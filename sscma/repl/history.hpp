#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/types.hpp"

namespace sscma::repl {

class History {
   public:
    History(int max_size = REPL_HISTORY_MAX) : _max_size(max_size) {}

    ~History() = default;

    bool add(const std::string& line) {
        if (line.empty()) [[unlikely]]
            return true;

        if (!_history.empty() && _history.back().compare(line) == 0) return true;

        auto it =
          std::find_if(_history.begin(), _history.end(), [&](const std::string& l) { return l.compare(line) == 0; });
        if (it != _history.end()) [[likely]]
            _history.erase(it);

        while (_history.size() >= _max_size) _history.pop_front();

        _history.push_back(line);
        _history.shrink_to_fit();
        _history_index = _history.size() - 1;

        return true;
    }

    bool add(const char* line) { return add(std::string(line)); }

    bool get(std::string& line, int index) {
        if (index < 0 || index >= _history.size()) [[unlikely]]
            return false;

        line = _history[index];

        return true;
    }

    bool next(std::string& line) {
        if (_history.empty()) [[unlikely]]
            return false;

        if (_history_index < 0) [[unlikely]]
            _history_index = _history.size() - 1;

        if (_history_index < (_history.size() - 1)) [[likely]]
            ++_history_index;

        line = _history[_history_index];

        return true;
    }

    bool prev(std::string& line) {
        if (_history.empty()) [[unlikely]]
            return false;

        if (_history_index >= 0) [[likely]]
            line = _history[_history_index--];

        return true;
    }

    bool reset() {
        bool is_tail   = _history_index == (_history.size() - 1);
        _history_index = _history.size() - 1;

        return is_tail;
    }

    void clear() {
        _history.clear();
        _history.shrink_to_fit();
        _history_index = -1;
    }

    size_t size() const { return _history.size(); }

   private:
    std::deque<std::string> _history;
    int                     _history_index;
    int                     _max_size;
};

}  // namespace sscma::repl
