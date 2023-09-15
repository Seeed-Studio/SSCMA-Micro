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
#include <forward_list>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "sscma/repl/history.hpp"

namespace sscma {

namespace repl {

class Server;

}

namespace types {

typedef std::function<void(el_err_code_t, const std::string&)> repl_echo_cb_t;

typedef std::function<el_err_code_t(std::vector<std::string>)> repl_cmd_cb_t;

struct repl_cmd_t {
    repl_cmd_t(std::string cmd_, std::string desc_, std::string args_, repl_cmd_cb_t cmd_cb_)
        : cmd(cmd_), desc(desc_), args(args_), cmd_cb(cmd_cb_), _argc(0) {
        if (args.size()) _argc = std::count(args.begin(), args.end(), ',') + 1;
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

class Server {
   public:
    Server() : _cmd_list_lock(), _exec_lock(), _is_ctrl(false), _line_index(-1) {
        register_cmd("HELP", "List available commands", "", [this](std::vector<std::string>) -> el_err_code_t {
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
            _echo_cb = echo_cb;
        }

        m_echo_cb(EL_OK, "Welcome to EegeLab REPL.\n", "Type 'AT+HELP' for command list.\n", "> ");
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

        auto it = std::find_if(
          _cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) { return c.cmd.compare(cmd) == 0; });

        return it != _cmd_list.end();
    }

    el_err_code_t register_cmd(const repl_cmd_t& cmd) {
        const Guard<Mutex> guard(_cmd_list_lock);

        if (cmd.cmd.empty()) [[unlikely]]
            return EL_EINVAL;

        auto it = std::find_if(
          _cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) { return c.cmd.compare(cmd.cmd) == 0; });
        if (it != _cmd_list.end()) [[unlikely]]
            *it = cmd;
        else
            _cmd_list.emplace_front(std::move(cmd));

        return EL_OK;
    }

    el_err_code_t register_cmd(const char* cmd, const char* desc, const char* arg, repl_cmd_cb_t cmd_cb) {
        repl_cmd_t cmd_t(cmd, desc, arg, cmd_cb);

        return register_cmd(std::move(cmd_t));
    }

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
            if (cmd.args.size())
                m_echo_cb(EL_OK, "  AT+", cmd.cmd, "=<", cmd.args, ">\n");
            else
                m_echo_cb(EL_OK, "  AT+", cmd.cmd, "\n");
            m_echo_cb(EL_OK, "    ", cmd.desc, "\n");
        }
    }

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
            _ctrl_line.push_back(c);
            if (std::isalpha(c) || (c == '~')) {
                if (_ctrl_line.compare("[A") == 0) {
                    _history.prev(_line);
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[K");
                } else if (_ctrl_line.compare("[B") == 0) {
                    _history.next(_line);
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[K");
                } else if (_ctrl_line.compare("[C") == 0) {
                    if (_line_index < _line.size() - 1) {
                        ++_line_index;
                        m_echo_cb(EL_OK, "\033", _ctrl_line);
                    }
                } else if (_ctrl_line.compare("[D") == 0) {
                    if (_line_index >= 0) {
                        --_line_index;
                        m_echo_cb(EL_OK, "\033", _ctrl_line);
                    }
                } else if (_ctrl_line.compare("[H") == 0) {
                    _line_index = 0;
                    m_echo_cb(EL_OK, "\r\033[K> ", _line, "\033[", _line_index + 3, "G");
                } else if (_ctrl_line.compare("[F") == 0) {
                    _line_index = _line.size() - 1;
                    m_echo_cb(EL_OK, "\r\033[K> ", _line, "\033[", _line_index + 4, "G");
                } else if (_ctrl_line.compare("[3~") == 0) {
                    if (_line_index < (_line.size() - 1)) {
                        if (!_line.empty() && _line_index >= 0) {
                            _line.erase(_line_index + 1, 1);
                            --_line_index;
                            m_echo_cb(EL_OK, "\r> ", _line, "\033[K\033[", _line_index + 4, "G");
                        }
                    }
                } else
                    m_echo_cb(EL_OK, "\033", _ctrl_line);

                _ctrl_line.clear();
                _is_ctrl = false;
            }

            return;
        }

        switch (c) {
        case '\n':
        case '\r':
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

        case '\b':
        case 0x7F:
            if (!_line.empty() && _line_index >= 0) {
                _line.erase(_line_index, 1);
                --_line_index;
                m_echo_cb(EL_OK, "\r> ", _line, "\033[K\033[", _line_index + 4, "G");
            }
            break;

        case 0x1B:
            _is_ctrl = true;
            break;

        default:
            if (std::isprint(c)) {
                _line.insert(++_line_index, 1, c);
                if (_line_index == (_line.size() - 1))
                    m_echo_cb(EL_OK, c);
                else
                    m_echo_cb(EL_OK, "\r> ", _line, "\033[", _line_index + 4, "G");
            }
        }
    }

   protected:
    void m_unregister_cmd(const std::string& cmd) {
        _cmd_list.remove_if([&](const repl_cmd_t& c) { return c.cmd.compare(cmd) == 0; });
    }

    el_err_code_t m_exec_cmd(const std::string& cmd) {
        el_err_code_t ret = EL_EINVAL;
        std::string   cmd_name;
        std::string   cmd_args;

        size_t pos = cmd.find_first_of("=");
        if (pos != std::string::npos) {
            cmd_name = cmd.substr(0, pos);
            cmd_args = cmd.substr(pos + 1);
        } else
            cmd_name = cmd;

        std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::toupper);

        if (cmd_name.rfind("AT+", 0) != 0) {
            m_echo_cb(EL_EINVAL, "Unknown command: ", cmd, "\n");
            return EL_EINVAL;
        }

        cmd_name = cmd_name.substr(3);

        _cmd_list_lock.lock();
        auto it = std::find_if(_cmd_list.begin(), _cmd_list.end(), [&](const repl_cmd_t& c) {
            size_t cmd_body_pos = cmd_name.rfind("@");
            return c.cmd.compare(cmd_name.substr(cmd_body_pos != std::string::npos ? cmd_body_pos + 1 : 0)) == 0;
        });
        if (it == _cmd_list.end()) [[unlikely]] {
            m_echo_cb(EL_EINVAL, "Unknown command: ", cmd, "\n");
            _cmd_list_lock.unlock();
            return ret;
        }
        repl_cmd_t cmd_copy = *it;
        _cmd_list_lock.unlock();

        if (!cmd_copy.cmd_cb) [[unlikely]]
            return ret;

        // tokenize
        std::vector<std::string> argv;
        argv.push_back(cmd_name);
        std::stack<char> stk;
        size_t           index = 0;
        size_t           size  = cmd_args.size();
        while (index < size && argv.size() < (cmd_copy._argc + 1)) {
            char c = cmd_args.at(index);
            if (c == '\'' || c == '"') [[unlikely]] {
                stk.push(c);
                std::string arg;
                while (++index < size && stk.size()) {
                    c = cmd_args.at(index);
                    if (c == stk.top())
                        stk.pop();
                    else if (c != '\\') [[likely]]
                        arg += c;
                    else if (++index < size) [[likely]]
                        arg += cmd_args.at(index);
                }
                argv.push_back(std::move(arg));
            } else if (c == '-' || std::isdigit(c)) {
                size_t prev = index;
                while (++index < size)
                    if (!std::isdigit(cmd_args.at(index))) break;
                argv.push_back(cmd_args.substr(prev, index - prev));
            } else
                ++index;
        }
        argv.shrink_to_fit();

        if (argv.size() != (cmd_copy._argc + 1)) [[unlikely]] {
            m_echo_cb(EL_EINVAL, "Command ", cmd_name, " got wrong arguements.\n");
            return ret;
        }

        ret = cmd_copy.cmd_cb(std::move(argv));
        if (ret != EL_OK) [[unlikely]]
            m_echo_cb(EL_EINVAL, "Command ", cmd_name, " failed.\n");

        return ret;
    }

    template <typename... Args> inline void m_echo_cb(el_err_code_t ret, Args&&... args) {
        auto os{std::ostringstream(std::ios_base::ate)};
        ((os << (std::forward<Args>(args))), ...);
        _echo_cb(ret, os.str());
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
