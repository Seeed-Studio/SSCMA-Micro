#pragma once

#include <cstdint>
#include <stack>
#include <string>
#include <utility>

#include "sscma/interpreter/types.hpp"
#include "sscma/interpreter/utility.hpp"

namespace sscma::interpreter {

using namespace sscma::interpreter::types;
using namespace sscma::interpreter::utility;

class Lexer {
   public:
    explicit Lexer(const std::string& input) noexcept : _input(input), _length(input.size() - 1), _index(0) {
        _current_char = _input.at(_index);
    }

    ~Lexer() = default;

    Token get_next_token() noexcept {
        do {
            if (std::isalpha(_current_char)) {
                std::string identifier{get_identifier()};
                if (is_lparn(_current_char)) {
                    identifier += get_function_parms();
                    return Token{.type = TokenType::FUNCTION, .value = std::move(identifier)};
                }
                return Token{.type = TokenType::IDENTIFIER, .value = std::move(identifier)};
            }

            if (std::isdigit(_current_char)) {
                return Token{.type = TokenType::CONSTANT, .value = get_constant()};
            }

            if (is_lparn(_current_char)) {
                size_t head = _index;
                advance();
                return Token{.type = TokenType::LPARN, .value = _input.substr(head, 1)};
            }

            if (is_rparn(_current_char)) {
                size_t head = _index;
                advance();
                return Token{.type = TokenType::RPARN, .value = _input.substr(head, 1)};
            }

            if (is_arithmetic_operator(_current_char)) {
                size_t head = _index;
                advance();
                return Token{.type = TokenType::OPERATOR, .value = _input.substr(head, 1)};
            }

            if (is_logical_operator(_current_char)) {
                size_t head = _index;
                if (advance() && is_logical_operator(_current_char)) {
                    advance();
                    return Token{.type = TokenType::OPERATOR, .value = _input.substr(head, _index - head)};
                }
                break;
            }

            if (is_relational_operator_pred(_current_char)) {
                size_t head = _index;
                if (advance() && is_relational_operator_term(_current_char)) advance();
                return Token{.type = TokenType::OPERATOR, .value = _input.substr(head, _index - head)};
            }

        } while (advance());

        return Token{.type = TokenType::TERM, .value = std::string{}};
    }

   protected:
    inline bool advance() noexcept {
        if (_index++ < _length) [[likely]] {
            _current_char = _input.at(_index);
            return true;
        } else {
            _current_char = '\0';
            return false;
        }
    }

    inline std::string get_constant() noexcept {
        size_t head = _index;

        while (advance() && is_constant(_current_char))
            ;

        return _input.substr(head, _index - head);
    }

    inline std::string get_identifier() noexcept {
        size_t head = _index;

        while (advance() && is_identifier(_current_char))
            ;

        return _input.substr(head, _index - head);
    }

    inline std::string get_function_parms() noexcept {
        size_t head = _index;

        std::stack<char> stk;
        stk.push(rparn);

        while (advance() && stk.size()) {
            if (_current_char == stk.top())
                stk.pop();
            else if (is_lparn(_current_char))
                stk.push(rparn);
        }

        return _input.substr(head, _index - head);
    }

   private:
    const std::string& _input;
    size_t             _length;
    size_t             _index;
    char               _current_char;
};

}  // namespace sscma::interpreter
