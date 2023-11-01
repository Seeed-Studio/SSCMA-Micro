#pragma once

#include <string>

#include "sscma/interpreter/types.hpp"

namespace sscma::interpreter {

using namespace sscma::interpreter::types;

class IdentifierNode : public ASTNode {
   public:
    IdentifierNode(const std::string& identifier_name) : _identifier_name(identifier_name) {}

    ~IdentifierNode() = default;

    inline Result evaluate(EvalCallback callback) const override { return callback(NodeType::IDENTIFIER, _identifier_name); }

   private:
    std::string _identifier_name;
};

class ConstantNode : public ASTNode {
   public:
    ConstantNode(const std::string& value) : _value(std::atoi(value.c_str())) {}

    ~ConstantNode() = default;

    inline Result evaluate(EvalCallback) const override { return Result{.status = EvalStatus::OK, .value = _value}; }

   private:
    int _value;
};

class OperatorNode : public ASTNode {
   public:
    OperatorNode(const std::string& operator_name, ASTNode* left, ASTNode* right)
        : _operator_name(operator_name), _left(left), _right(right) {}

    ~OperatorNode() {
        if (_left) delete _left;
        if (_right) delete _right;
    }

    Result evaluate(EvalCallback callback) const override {
        EvalStatus eval_status{EvalStatus::OK};
        Result     result{.status = EvalStatus::EXCEPTION, .value = 0};

        auto left_e = [&]() {
            Result left{_left->evaluate(callback)};
            eval_status = eval_status != EvalStatus::OK ? eval_status : left.status;
            return left.value;
        };

        auto right_e = [&]() {
            Result right{_right->evaluate(callback)};
            eval_status = eval_status != EvalStatus::OK ? eval_status : right.status;
            return right.value;
        };

        if (_operator_name == "&&") {
            result.value  = left_e() && right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "||") {
            result.value  = left_e() || right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "+") {
            result.value  = left_e() + right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "-") {
            result.value  = left_e() - right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "*") {
            result.value  = left_e() * right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "/") {
            auto rv{right_e()};
            if (rv) [[likely]] {
                result.value  = left_e() / rv;
                result.status = eval_status;
            } else
                result.status = EvalStatus::EXCEPTION;
            return result;
        }

        if (_operator_name == ">") {
            result.value  = left_e() > right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "<") {
            result.value  = left_e() < right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == ">=") {
            result.value  = left_e() >= right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "<=") {
            result.value  = left_e() <= right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "==") {
            result.value  = left_e() == right_e();
            result.status = eval_status;
            return result;
        }

        if (_operator_name == "!=") {
            result.value  = left_e() != right_e();
            result.status = eval_status;
            return result;
        }

        return result;
    }

   private:
    std::string _operator_name;
    ASTNode*    _left;
    ASTNode*    _right;
};

class FunctionCallNode : public ASTNode {
   public:
    FunctionCallNode(const std::string& function_call) : _function_call(function_call) {}

    ~FunctionCallNode() = default;

    Result evaluate(EvalCallback callback) const override { return callback(NodeType::FUNCTION_CALL, _function_call); }

   private:
    std::string _function_call;
};

}  // namespace sscma::interpreter
