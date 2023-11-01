#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "core/utils/el_hash.h"
#include "sscma/interpreter/lexer.hpp"
#include "sscma/interpreter/parser.hpp"
#include "sscma/interpreter/types.hpp"
#include "sscma/types.hpp"

namespace sscma::interpreter {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::interpreter;
using namespace sscma::interpreter::types;

class Condition {
   public:
    Condition() : _node(nullptr), _exp_hash(0xffff), _eval_lock() {}

    ~Condition() { m_unset_condition(); }

    inline bool has_condition() {
        const Guard<Mutex> guard(_eval_lock);
        return _node ? true : false;
    }

    bool set_condition(const std::string& input) {
        const Guard<Mutex> guard(_eval_lock);

        if (!input.size()) [[unlikely]] {
            m_unset_condition();
            _exp_hash = 0xffff;
            return true;
        }

        uint16_t exp_hash = el_crc16_maxim(reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
        if (exp_hash == _exp_hash) [[unlikely]]
            return true;

        Lexer    lexer(input);
        Mutables mutables;
        Parser   parser(lexer, mutables);

        auto* node = parser.parse();
        if (!node) [[unlikely]]
            return false;

        m_unset_condition();
        _node     = node;
        _exp_hash = exp_hash;

        for (const auto& tok : mutables) _mutable_map[tok.value] = nullptr;

        return true;
    }

    inline uint16_t get_condition_hash() {
        const Guard<Mutex> guard(_eval_lock);
        return _exp_hash;
    }

    inline const mutable_map_t& get_mutable_map() {
        const Guard<Mutex> guard(_eval_lock);
        return _mutable_map;
    }

    inline void set_mutable_map(const mutable_map_t& map) {
        const Guard<Mutex> guard(_eval_lock);
        _mutable_map = map;
    }

    inline void set_exception_cb(branch_cb_t cb) {
        const Guard<Mutex> guard(_eval_lock);
        _exception_cb = cb;
    }

    inline void evalute() {
        const Guard<Mutex> guard(_eval_lock);

        if (!_node) [[unlikely]]
            return;

        auto result = _node->evaluate([this](NodeType, const std::string& name) {
            auto it = this->_mutable_map.find(name);
            if (it != this->_mutable_map.end() && it->second) [[likely]]
                return Result{.status = EvalStatus::OK, .value = it->second()};
            return Result{.status = EvalStatus::EXCEPTION, .value = 0};
        });

        if (result.status != EvalStatus::OK) [[unlikely]] {
            if (_exception_cb) [[likely]]
                _exception_cb();
        }
    }

   protected:
    inline void m_unset_condition() {
        if (_node) [[likely]] {
            delete _node;
            _node = nullptr;
        }
        _mutable_map.clear();
    }

   private:
    ASTNode*      _node;
    uint16_t      _exp_hash;
    Mutex         _eval_lock;
    mutable_map_t _mutable_map;
    branch_cb_t   _exception_cb;
};

}  // namespace sscma::interpreter
