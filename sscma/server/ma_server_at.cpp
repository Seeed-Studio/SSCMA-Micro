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

ATServer::ATServer(Codec* codec) : m_codec(*codec) {

    m_device  = Device::getInstance();
    m_storage = m_device->getStorage();
    m_transports.assign(m_device->getTransports().begin(), m_device->getTransports().end());

    m_executor = new Executor(MA_SEVER_AT_EXECUTOR_STACK_SIZE, MA_SEVER_AT_EXECUTOR_TASK_PRIO);
    MA_ASSERT(m_executor);
    m_thread = new Thread("ATServer", ATServer::threadEntryStub);
    MA_ASSERT(m_thread);

    memset(&m_wifiConfig, 0, sizeof(m_wifiConfig));
    memset(&m_mqttConfig, 0, sizeof(m_mqttConfig));
    memset(&m_mqttTopicConfig, 0, sizeof(m_mqttTopicConfig));
}

ATServer::ATServer(Codec& codec) : m_codec(codec) {

    m_device  = Device::getInstance();
    m_storage = m_device->getStorage();
    m_transports.assign(m_device->getTransports().begin(), m_device->getTransports().end());

    m_executor = new Executor(MA_SEVER_AT_EXECUTOR_STACK_SIZE, MA_SEVER_AT_EXECUTOR_TASK_PRIO);
    MA_ASSERT(m_executor);
    m_thread = new Thread("ATServer", ATServer::threadEntryStub);
    MA_ASSERT(m_thread);

    memset(&m_wifiConfig, 0, sizeof(m_wifiConfig));
    memset(&m_mqttConfig, 0, sizeof(m_mqttConfig));
    memset(&m_mqttTopicConfig, 0, sizeof(m_mqttTopicConfig));
}

void ATServer::threadEntryStub(void* arg) {
    ATServer* server = static_cast<ATServer*>(arg);
    server->threadEntry();
}

void ATServer::threadEntry() {
    char* buf = reinterpret_cast<char*>(malloc(MA_SEVER_AT_CMD_MAX_LENGTH + 1));
    std::memset(buf, 0, MA_SEVER_AT_CMD_MAX_LENGTH + 1);
    while (true) {
        for (auto& tansport : m_transports) {
            if (*tansport) {
                int len = tansport->receiveUtil(buf, MA_SEVER_AT_CMD_MAX_LENGTH, 0x0D);
                if (len > 1) {
                    execute(buf, tansport);
                }
                len = tansport->receiveUtil(buf, MA_SEVER_AT_CMD_MAX_LENGTH, 0x0A);
                if (len > 1) {
                    execute(buf, tansport);
                }
            }
        }
        Tick::sleep(Tick::fromMicroseconds(20));
    }
}

ma_err_t ATServer::init() {

    ma_err_t err = MA_OK;

    Executor* executor = m_executor;


    m_storage->get(MA_STORAGE_KEY_MQTT_HOST, m_mqttConfig.host, "");

    if (m_mqttConfig.host[0] != '\0') {

        m_storage->get(MA_STORAGE_KEY_MQTT_PORT, m_mqttConfig.port, 1883);
        m_storage->get(MA_STORAGE_KEY_MQTT_CLIENTID, m_mqttConfig.client_id, "");
        m_storage->get(MA_STORAGE_KEY_MQTT_USER, m_mqttConfig.username, "");
        m_storage->get(MA_STORAGE_KEY_MQTT_PWD, m_mqttConfig.password, "");

        if (m_mqttConfig.client_id[0] == '\0') {
            snprintf(m_mqttConfig.client_id,
                     MA_MQTT_MAX_CLIENT_ID_LENGTH,
                     MA_MQTT_CLIENTID_FMT,
                     m_device->name().c_str(),
                     m_device->id().c_str());
            m_storage->set(MA_STORAGE_KEY_MQTT_CLIENTID, m_mqttConfig.client_id);
        }

        snprintf(m_mqttTopicConfig.pub_topic,
                 MA_MQTT_MAX_TOPIC_LENGTH,
                 MA_MQTT_TOPIC_FMT "/tx",
                 MA_AT_API_VERSION,
                 m_mqttConfig.client_id);
        snprintf(m_mqttTopicConfig.sub_topic,
                 MA_MQTT_MAX_TOPIC_LENGTH,
                 MA_MQTT_TOPIC_FMT "/rx",
                 MA_AT_API_VERSION,
                 m_mqttConfig.client_id);

#if MA_USE_TRANSPORT_MQTT
        MQTT* transport = new MQTT(
            m_mqttConfig.client_id, m_mqttTopicConfig.pub_topic, m_mqttTopicConfig.sub_topic);
        MA_LOGD(TAG, "MQTT Transport: %p", transport);
        transport->connect(
            m_mqttConfig.host, m_mqttConfig.port, m_mqttConfig.username, m_mqttConfig.password);

        m_transports.push_front(transport);
#endif
    }


    MA_LOGD(TAG, "MQTT Host: %s:%d", m_mqttConfig.host, m_mqttConfig.port);
    MA_LOGD(TAG, "MQTT Client ID: %s", m_mqttConfig.client_id);
    MA_LOGD(TAG, "MQTT User: %s", m_mqttConfig.username);
    MA_LOGD(TAG, "MQTT Pwd: %s", m_mqttConfig.password);
    MA_LOGD(TAG, "MQTT Pub Topic: %s", m_mqttTopicConfig.pub_topic);
    MA_LOGD(TAG, "MQTT Sub Topic: %s", m_mqttTopicConfig.sub_topic);

    m_storage->get(MA_STORAGE_KEY_WIFI_SSID, m_wifiConfig.ssid, "");
    m_storage->get(MA_STORAGE_KEY_WIFI_BSSID, m_wifiConfig.bssid, "");
    m_storage->get(MA_STORAGE_KEY_WIFI_PWD, m_wifiConfig.password, "");
    m_storage->get(MA_STORAGE_KEY_WIFI_SECURITY, m_wifiConfig.security, SEC_AUTO);

    MA_LOGD(TAG, "Wifi SSID: %s", m_wifiConfig.ssid);
    MA_LOGD(TAG, "Wifi BSSID: %s", m_wifiConfig.bssid);
    MA_LOGD(TAG, "Wifi Pwd: %s", m_wifiConfig.password);
    MA_LOGD(TAG, "Wifi Security: %d", m_wifiConfig.security);


    this->addService(
        "ID?",
        "Get device ID",
        "",
        [executor](std::vector<std::string> args, Transport& transport, Codec& codec) {
            executor->add_task(
                [cmd = std::move(args[0]), &transport, &codec](const std::atomic<bool>&) {
                    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd);
                    codec.write("id", Device::getInstance()->id());
                    codec.end();
                    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
                });
            return MA_OK;
        });
    return err;
}

ma_err_t ATServer::start() {

    m_thread->start(this);

    return MA_OK;
}

ma_err_t ATServer::stop() {
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