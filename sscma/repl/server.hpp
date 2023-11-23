/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "sscma/repl/history.hpp"
#include "sscma/utility.hpp"

namespace sscma {

namespace repl {

class Server;  // pre-defined for friend class

}

namespace types {

typedef std::function<void(el_err_code_t, std::string)> repl_echo_cb_t;

typedef std::function<el_err_code_t(std::vector<std::string>)> repl_cmd_cb_t;

struct repl_cmd_t {
    template <typename Callable>
    repl_cmd_t(std::string cmd_, std::string desc_, std::string args_, Callable&& cmd_cb_)
        : cmd(cmd_), desc(desc_), args(args_), cmd_cb(std::forward<Callable>(cmd_cb_)), _argc(0) {
        if (args.size()) _argc = std::count(args.begin(), args.end(), ',') + 1;
    }

    repl_cmd_t(repl_cmd_t&& repl_cmd)
        : cmd(std::move(repl_cmd.cmd)),
          desc(std::move(repl_cmd.desc)),
          args(std::move(repl_cmd.args)),
          cmd_cb(std::move(repl_cmd.cmd_cb)),
          _argc(repl_cmd._argc) {}

    repl_cmd_t(const repl_cmd_t& repl_cmd)
        : cmd(repl_cmd.cmd), desc(repl_cmd.desc), args(repl_cmd.args), cmd_cb(repl_cmd.cmd_cb), _argc(repl_cmd._argc) {}

    repl_cmd_t& operator=(repl_cmd_t&& repl_cmd) {
        if (this != &repl_cmd) [[likely]] {
            cmd    = std::move(repl_cmd.cmd);
            desc   = std::move(repl_cmd.desc);
            args   = std::move(repl_cmd.args);
            cmd_cb = std::move(repl_cmd.cmd_cb);
            _argc  = repl_cmd._argc;
        }
        return *this;
    }

    repl_cmd_t& operator=(const repl_cmd_t& repl_cmd) {
        if (this != &repl_cmd) [[likely]] {
            cmd    = repl_cmd.cmd;
            desc   = repl_cmd.desc;
            args   = repl_cmd.args;
            cmd_cb = repl_cmd.cmd_cb;
            _argc  = repl_cmd._argc;
        }
        return *this;
    }

    ~repl_cmd_t() = default;

    std::string   cmd;
    std::string   desc;
    std::string   args;
    repl_cmd_cb_t cmd_cb;

    friend class sscma::repl::Server;

   private:
    uint8_t _argc;
};

}  // namespace types

}  // namespace sscma

namespace sscma::repl {

using namespace edgelab;

using namespace sscma::types;

class Server {
   public:
    Server() : _history(SSCMA_REPL_HISTORY_MAX), _cmd_list_lock(), _exec_lock(), _is_ctrl(false), _line_index(-1) {
        register_cmd("HELP?", "List available commands", "", [this](std::vector<std::string>) -> el_err_code_t {
            this->print_help();
            return EL_OK;
        });
    }

    ~Server() { deinit(); }

    Server(Server const&)            = delete;
    Server& operator=(Server const&) = delete;

    void init(repl_echo_cb_t echo_cb) {
        {
            const Guard<Mutex> guard(_cmd_list_lock);
            if (!echo_cb) [[unlikely]]
                _echo_cb = [](el_err_code_t, std::string msg) { el_printf("%s\n", msg.c_str()); };
            else
                _echo_cb = std::move(echo_cb);
        }

        m_echo_cb(EL_OK, "Welcome to EegeLab REPL.\n", "Type 'AT+HELP?' for command list.\n", "> ");
    }

    void deinit() {
        const Guard<Mutex> guard(_cmd_list_lock);

        _history.clear();
        _cmd_list.clear();

        _is_ctrl = false;
        _ctrl_line.clear();
        _line.clear();
        _line_index = -1;
    }

    bool has_cmd(const std::string& cmd) {
        const Guard<Mutex> guard(_cmd_list_lock);
        auto               it = std::find_if(
          _cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) { return c.cmd.compare(cmd) == 0; });
        return it != _cmd_list.end();
    }

    template <typename Command> el_err_code_t register_cmd(Command&& cmd) {
        const Guard<Mutex> guard(_cmd_list_lock);

        if (cmd.cmd.empty()) [[unlikely]]
            return EL_EINVAL;

        auto it = std::find_if(
          _cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) { return c.cmd.compare(cmd.cmd) == 0; });
        if (it != _cmd_list.end()) [[unlikely]] {                 // if cmd already registered, replace it
            const Guard<Mutex> guard(_exec_lock);                 // lock to avoid concurrency modification of cmd list
            *it = std::forward<Command>(cmd);                     // overloaded construct (copy or move)
        } else                                                    // else register an new cmd
            _cmd_list.emplace_front(std::forward<Command>(cmd));  // inplace construct (copy or move)

        return EL_OK;
    }

    template <typename Callable>
    el_err_code_t register_cmd(const char* cmd, const char* desc, const char* args, Callable&& cmd_cb) {
        repl_cmd_t cmd_t{cmd, desc, args, std::forward<Callable>(cmd_cb)};
        return register_cmd(std::move(cmd_t));
    }

    // unregister multiple commands
    template <typename... Args> void unregister_cmd(Args&&... args) {
        const Guard<Mutex> guard(_cmd_list_lock);
        ((m_unregister_cmd(std::forward<Args>(args))), ...);
    }

    std::forward_list<repl_cmd_t> get_registered_cmds() const {
        const Guard<Mutex> guard(_cmd_list_lock);
        return _cmd_list;
    }

    void print_help() {
        const Guard<Mutex> guard(_cmd_list_lock);

        _echo_cb(EL_OK, "Command list:\n");
        for (const auto& cmd : _cmd_list) {
            if (cmd.args.size())  // if cmd has args
                m_echo_cb(EL_OK, "  AT+", cmd.cmd, "=<", cmd.args, ">\n");
            else
                m_echo_cb(EL_OK, "  AT+", cmd.cmd, "\n");
            m_echo_cb(EL_OK, "    ", cmd.desc, "\n");
        }
    }

    // use exec_non_lock since no recursive mutex implemented
    el_err_code_t exec_non_lock(std::string line) {
        auto it = std::remove_if(line.begin(), line.end(), [](char c) { return !std::isprint(c); });
        line.erase(it, line.end());
        return m_exec_cmd(line);
    }

    el_err_code_t exec(std::string line) {
        const Guard<Mutex> guard(_exec_lock);
        return exec_non_lock(std::move(line));
    }

    void loop(const std::string& line) {
        std::for_each(line.begin(), line.end(), [this](char c) { this->loop(c); });
    }

    void loop(char c) {
        if (_is_ctrl) {
            _ctrl_line.push_back(c);                  // append current char to ctrl line
            if (std::isalpha(c) || (c == '~')) {      // if current char is a letter or '~'
                if (_ctrl_line.compare("[A") == 0) {  // if press up arrow
                    _history.prev(_line);
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[K");
                } else if (_ctrl_line.compare("[B") == 0) {  // if press down arrow
                    _history.next(_line);
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[K");
                } else if (_ctrl_line.compare("[C") == 0) {  // if press right arrow
                    if (_line_index < static_cast<int>(_line.size()) - 1) {
                        ++_line_index;
                        m_echo_cb(EL_OK, "\033", _ctrl_line);
                    }
                } else if (_ctrl_line.compare("[D") == 0) {  // if press left arrow
                    if (_line_index >= 0) {
                        --_line_index;
                        m_echo_cb(EL_OK, "\033", _ctrl_line);
                    }
                } else if (_ctrl_line.compare("[H") == 0) {  // if press home
                    _line_index = 0;
                    m_echo_cb(EL_OK, "\r\033[K> ", _line, "\033[", std::to_string(_line_index + 3), "G");
                } else if (_ctrl_line.compare("[F") == 0) {  // if press end
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r\033[K> ", _line, "\033[", std::to_string(_line_index + 4), "G");
                } else if (_ctrl_line.compare("[3~") == 0) {  // if press delete
                    if (_line_index < static_cast<int>(_line.size()) - 1) {
                        if (!_line.empty() && _line_index >= 0) {
                            _line.erase(_line_index + 1, 1);
                            --_line_index;
                            m_echo_cb(EL_OK, "\r> ", _line, "\033[K\033[", std::to_string(_line_index + 4), "G");
                        }
                    }
                } else
                    m_echo_cb(EL_OK, "\033", _ctrl_line);  // echo ctrl line

                _ctrl_line.clear();
                _is_ctrl = false;
            }

            return;
        }

        switch (c) {
        case '\n':  // newline
        case '\r':  // enter
            _echo_cb(EL_OK, "\r\n");
            if (!_line.empty()) {
                if (m_exec_cmd(_line) == EL_OK) [[likely]] {
                    _history.add(_line);
                }
                _line.clear();
                _line_index = -1;
            }
            _history.reset();
            _echo_cb(EL_OK, "\r> ");
            break;

        case '\b':  // backspace
        case 0x7F:  // DEL
            if (!_line.empty() && _line_index >= 0) {
                _line.erase(_line_index, 1);
                --_line_index;
                m_echo_cb(EL_OK, "\r> ", _line, "\033[K\033[", std::to_string(_line_index + 4), "G");
            }
            break;

        case 0x1B:  // ESC
            _is_ctrl = true;
            break;

        default:
            if (std::isprint(c)) {                                      // if current char is printable
                _line.insert(++_line_index, 1, c);                      // insert the char to line
                if (_line_index == static_cast<int>(_line.size()) - 1)  // if current char is the last char of line
                    m_echo_cb(EL_OK, std::to_string(c));                // echo the char
                else                                                    // else move cursor on the line
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[", std::to_string(_line_index + 4), "G");
            }
        }
    }

   protected:
    void m_unregister_cmd(std::string cmd) {
        const Guard<Mutex> guard(_exec_lock);
        _cmd_list.remove_if([&](const repl_cmd_t& c) { return c.cmd.compare(cmd) == 0; });
    }

    el_err_code_t m_exec_cmd(const std::string& cmd) {
        el_err_code_t ret = EL_EINVAL;
        std::string   cmd_name;
        std::string   cmd_args;

        // find first '=' in AT command (<cmd_name>=<cmd_args>)
        size_t pos = cmd.find_first_of("=");
        if (pos != std::string::npos) {
            cmd_name = cmd.substr(0, pos);
            cmd_args = cmd.substr(pos + 1, cmd.size());
        } else
            cmd_name = cmd;

        std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::toupper);

        // check if cmd_name is valid (starts with "AT+")
        if (cmd_name.rfind("AT+", 0) != 0) {
            m_echo_cb(EL_EINVAL, "Unknown command: ", cmd, "\n");
            return EL_EINVAL;
        }
        cmd_name = cmd_name.substr(3, cmd_name.size());  // remove "AT+" command prefix

        // find command tag (everything after last '@'), then remove the tag to get the real command name
        size_t cmd_body_pos    = cmd_name.rfind('@');
        cmd_body_pos           = cmd_body_pos != std::string::npos ? cmd_body_pos + 1 : 0;
        std::string target_cmd = cmd_name.substr(cmd_body_pos, cmd_name.size());

        // find command in cmd list
        _cmd_list_lock.lock();
        auto it = std::find_if(
          _cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) { return c.cmd.compare(target_cmd) == 0; });
        if (it == _cmd_list.end()) [[unlikely]] {
            m_echo_cb(EL_EINVAL, "Unknown command: ", cmd, "\n");
            _cmd_list_lock.unlock();
            return ret;
        }
        _cmd_list_lock.unlock();

        // TODO: use dispatch queue to avoid concurrency modification of cmd list
        // maybe unsafe if executing some cmd callback while the command got unregistered or changed

        if (!it->cmd_cb) [[unlikely]]
            return ret;

        // tokenize callback args in callback function exisits
        std::vector<std::string> argv;
        argv.push_back(cmd_name);
        std::stack<char> stk;
        size_t           index = 0;
        size_t           size  = cmd_args.size();
        while (index < size && argv.size() < it->_argc + 1u) {  // while not reach end of cmd_args and not enough args
            char c = cmd_args.at(index);
            if (c == '\'' || c == '"') [[unlikely]] {  // if current char is a quote
                stk.push(c);                           // push the quote to stack
                std::string arg;
                while (++index < size && stk.size()) {  // while not reach end of cmd_args and stack is not empty
                    c = cmd_args.at(index);             // get next char
                    if (c == stk.top())                 // if the char matches the top of stack, pop the stack
                        stk.pop();
                    else if (c != '\\') [[likely]]  // if the char is not a backslash, append it to arg
                        arg += c;
                    else if (++index < size) [[likely]]  // if the char is a backslash, get next char, append it to arg
                        arg += cmd_args.at(index);
                }
                argv.push_back(std::move(arg));
            } else if (c == '-' || std::isdigit(c)) {  //if current char is a digit or a minus sign
                size_t prev = index;
                while (++index < size)                                // while not reach end of cmd_args
                    if (!std::isdigit(cmd_args.at(index))) break;     // if current char is not a digit, break
                argv.push_back(cmd_args.substr(prev, index - prev));  // append the number string to argv
            } else
                ++index;  // if current char is not a quote or a digit, skip it
        }
        argv.shrink_to_fit();

        if (argv.size() != it->_argc + 1u) [[unlikely]] {
            m_echo_cb(EL_EINVAL, "Command ", cmd_name, " got wrong arguements.\n");
            return ret;
        }

        ret = it->cmd_cb(std::move(argv));  // execute callback function
        if (ret != EL_OK) [[unlikely]]
            m_echo_cb(EL_EINVAL, "Command ", cmd_name, " failed.\n");

        return ret;
    }

    template <typename... Args> inline void m_echo_cb(el_err_code_t ret, Args&&... args) {
        using namespace sscma::utility::string_concat;
        std::string ss{concat_strings(std::forward<Args>(args)...)};
        _echo_cb(ret, std::move(ss));
    }

   private:
    History _history;

    Mutex                         _cmd_list_lock;
    std::forward_list<repl_cmd_t> _cmd_list;

    Mutex _exec_lock;

    repl_echo_cb_t _echo_cb;

    bool        _is_ctrl;
    std::string _ctrl_line;
    std::string _line;
    int         _line_index;
};

}  // namespace sscma::repl
