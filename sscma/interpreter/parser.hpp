#pragma once

#include <stack>

#include "sscma/interpreter/lexer.hpp"
#include "sscma/interpreter/nodes.hpp"
#include "sscma/interpreter/types.hpp"

namespace sscma::interpreter {

using namespace sscma::interpreter::types;

class Parser {
   public:
    Parser(Lexer& lexer, Mutables& mut) : _lexer(lexer), _current_token(lexer.get_next_token()), _mut(mut) {}

    ~Parser() = default;

    ASTNode* parse() {
        while (_current_token.type != TokenType::TERM && _node_stack.size() < 3 && parse_expression()) {
            _current_token = _lexer.get_next_token();
        }

        if (_node_stack.size() != 1) {
            while (_node_stack.size()) {
                delete _node_stack.top();
                _node_stack.pop();
            }
            return nullptr;
        }

        return _node_stack.top();
    }

   protected:
    bool parse_expression() {
        if (_current_token.type == TokenType::IDENTIFIER) {
            _mut.push_back(_current_token);
            _node_stack.push(new IdentifierNode(_current_token.value));
            return true;
        }

        if (_current_token.type == TokenType::CONSTANT) {
            _node_stack.push(new ConstantNode(_current_token.value));
            return true;
        }

        if (_current_token.type == TokenType::FUNCTION) {
            _mut.push_back(_current_token);
            _node_stack.push(new FunctionCallNode(_current_token.value));
            return true;
        }

        if (_current_token.type == TokenType::OPERATOR) {
            Token operator_token = _current_token;
            if (_node_stack.size() == 1) {
                _current_token = _lexer.get_next_token();
                if (!parse_expression()) [[unlikely]]
                    return false;
            }

            if (_node_stack.size() != 2) return false;

            ASTNode* right = _node_stack.top();
            _node_stack.pop();
            ASTNode* left = _node_stack.top();
            _node_stack.pop();

            _node_stack.push(new BinaryOperatorNode(operator_token.value, left, right));

            return true;
        }

        if (_current_token.type == TokenType::LPARN) {
            _node_stack.push(Parser(_lexer, _mut).parse());
            return true;
        }

        if (_current_token.type == TokenType::RPARN) {
            return false;
        }

        // skip error
        return false;
    }

   private:
    Lexer&               _lexer;
    Token                _current_token;
    Mutables&          _mut;
    std::stack<ASTNode*> _node_stack;
};

}  // namespace sscma::interpreter
