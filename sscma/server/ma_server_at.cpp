#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <stack>

#include "ma_server_at.h"

namespace ma {

const char* TAG = "ma::server::ATServer";

ATService::ATService(const std::string& name,
                     const std::string& desc,
                     const std::string& args,
                     ATServiceCallback cb)
    : name(name), desc(desc), args(args), cb(cb) {}


ma_err_t ATServer::addService(ATService& service) {
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        if (it->name == service.name) {
            return MA_EEXIST;
        }
    }
    m_services.push_back(service);
    return MA_OK;
}

ma_err_t ATServer::addService(const std::string& name,
                              const std::string& desc,
                              const std::string& args,
                              ATServiceCallback cb) {
    ATService service(name, desc, args, cb);
    return addService(service);
}

ma_err_t ATServer::execute(std::string line, Transport& transport) {

    ma_err_t ret = MA_OK;
    std::string name;
    std::string args;

    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return !std::isprint(c); }),
               line.end());

    // find first '=' in AT command (<name>=<args>)
    size_t pos = line.find_first_of("=");
    if (pos != std::string::npos) {
        name = line.substr(0, pos);
        args = line.substr(pos + 1, line.size());
    } else {
        name = line;
    }

    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    // check if name is valid (starts with "AT+")
    if (name.rfind("AT+", 0) != 0) {
        m_codec.begin(MA_REPLY_EVENT, MA_EINVAL, "AT", "Uknown command: " + name);
        m_codec.end();
        transport.send(reinterpret_cast<const char*>(m_codec.data()), m_codec.size());
        return MA_EINVAL;
    }

    name = name.substr(3, name.size());  // remove "AT+" command prefix

    // find command tag (everything after last '@'), then remove the tag to get the AT command
    // name
    size_t cmd_body_pos    = name.rfind('@');
    cmd_body_pos           = cmd_body_pos != std::string::npos ? cmd_body_pos + 1 : 0;
    std::string target_cmd = name.substr(cmd_body_pos, name.size());

    // find command in cmd list
    auto it = std::find_if(m_services.begin(), m_services.end(), [&](const ATService& c) {
        return c.name.compare(target_cmd) == 0;
    });

    if (it == m_services.end()) [[unlikely]] {
        m_codec.begin(MA_REPLY_EVENT, MA_EINVAL, "AT", "Uknown command: " + name);
        m_codec.end();
        transport.send(reinterpret_cast<const char*>(m_codec.data()), m_codec.size());
        return MA_EINVAL;
    }

    // split args by ','
    std::vector<std::string> argv;
    argv.push_back(name);
    std::stack<char> stk;
    size_t index = 0;
    size_t size  = args.size();

    while (index < size) {  // while not reach end of args and not enough args
        char c = args.at(index);
        if (c == '\'' || c == '"') [[unlikely]] {  // if current char is a quote
            stk.push(c);                           // push the quote to stack
            std::string arg;
            while (++index < size &&
                   stk.size()) {     // while not reach end of args and stack is not empty
                c = args.at(index);  // get next char
                if (c == stk.top())  // if the char matches the top of stack, pop the stack
                    stk.pop();
                else if (c != '\\') [[likely]]  // if the char is not a backslash, append it to
                    arg += c;
                else if (++index < size)
                    [[likely]]  // if the char is a backslash, get next char, append it to arg
                    arg += args.at(index);
            }
            argv.push_back(std::move(arg));
        } else if (c == '-' || std::isdigit(c)) {  // if current char is a digit or a minus sign
            size_t prev = index;
            while (++index < size)  // while not reach end of args
                if (!std::isdigit(args.at(index)))
                    break;  // if current char is not a digit, break
            argv.push_back(args.substr(prev, index - prev));  // append the number string to argv
        } else
            ++index;  // if current char is not a quote or a digit, skip it
    }
    argv.shrink_to_fit();

    ret = it->cb(std::move(argv), transport, m_codec);

    return ret;
}

ma_err_t ATServer::execute(std::string line, Transport* transport) {
    return execute(line, *transport);
}

}  // namespace ma