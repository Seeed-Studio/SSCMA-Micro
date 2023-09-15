#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
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
    Condition() : _node(nullptr), _eval_lock() {}

    ~Condition() { unset_condition(); }

    bool has_condition() {
        const Guard<Mutex> guard(_eval_lock);
        return _node ? true : false;
    }

    bool set_condition(const std::string& input) {
        const Guard<Mutex> guard(_eval_lock);

        Lexer    lexer(input);
        Mutables mutables;
        Parser   parser(lexer, mutables);

        _node = parser.parse();

        if (!_node) [[unlikely]]
            return false;

        for (const auto& tok : mutables) _mutable_map[tok.value] = nullptr;

        return true;
    }

    const mutable_map_t& get_mutable_map() {
        const Guard<Mutex> guard(_eval_lock);
        return _mutable_map;
    }

    void set_mutable_map(const mutable_map_t& map) {
        const Guard<Mutex> guard(_eval_lock);
        _mutable_map = map;
    }

    void set_true_cb(branch_cb_t cb) {
        const Guard<Mutex> guard(_eval_lock);
        _true_cb = cb;
    }

    void set_false_cb(branch_cb_t cb) {
        const Guard<Mutex> guard(_eval_lock);
        _false_cb = cb;
    }

    void set_exception_cb(branch_cb_t cb) {
        const Guard<Mutex> guard(_eval_lock);
        _exception_cb = cb;
    }

    void evalute() {
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
        } else {
            if (result.value) {
                if (_true_cb) [[likely]]
                    _true_cb();
            } else {
                if (_false_cb) [[likely]]
                    _false_cb();
            }
        }
    }

    void unset_condition() {
        const Guard<Mutex> guard(_eval_lock);

        if (_node) [[likely]] {
            delete _node;
            _node = nullptr;
        }
        _mutable_map.clear();
    }

   private:
    ASTNode* _node;

    Mutex _eval_lock;

    mutable_map_t _mutable_map;

    branch_cb_t _true_cb;
    branch_cb_t _false_cb;
    branch_cb_t _exception_cb;
};

}  // namespace sscma::interpreter
