#pragma once

#include <string>

#include "sscma/interpreter/types.hpp"

namespace sscma::interpreter {

using namespace sscma::interpreter::types;

class IdentifierNode : public ASTNode {
   public:
    IdentifierNode(const std::string& identifier_name) : _identifier_name(identifier_name) {}

    ~IdentifierNode() = default;

    Result evaluate(EvalCallback callback) const override { return callback(NodeType::IDENTIFIER, _identifier_name); }

   private:
    std::string _identifier_name;
};

class ConstantNode : public ASTNode {
   public:
    ConstantNode(const std::string& value) : _value(std::atoi(value.c_str())) {}

    ~ConstantNode() = default;

    Result evaluate(EvalCallback) const override { return Result{.status = EvalStatus::OK, .value = _value}; }

   private:
    int _value;
};

class BinaryOperatorNode : public ASTNode {
   public:
    BinaryOperatorNode(const std::string& operator_name, ASTNode* left, ASTNode* right)
        : _operator_name(operator_name), _left(left), _right(right) {}

    ~BinaryOperatorNode() {
        if (_left) delete _left;
        if (_right) delete _right;
    }

    Result evaluate(EvalCallback callback) const override {
        Result left = _left->evaluate(callback);
        if (left.status != EvalStatus::OK) [[unlikely]]
            return left;

        Result right = _right->evaluate(callback);
        if (right.status != EvalStatus::OK) [[unlikely]]
            return right;

        if (_operator_name == "*") return Result{.status = EvalStatus::OK, .value = left.value * right.value};

        if (_operator_name == "/")
            return Result{.status = right.value ? EvalStatus::OK : EvalStatus::EXCEPTION,
                          .value  = left.value / right.value};

        if (_operator_name == "+") return Result{.status = EvalStatus::OK, .value = left.value + right.value};

        if (_operator_name == "-") return Result{.status = EvalStatus::OK, .value = left.value - right.value};

        if (_operator_name == ">") return Result{.status = EvalStatus::OK, .value = left.value > right.value};

        if (_operator_name == "<") return Result{.status = EvalStatus::OK, .value = left.value < right.value};

        if (_operator_name == ">=") return Result{.status = EvalStatus::OK, .value = left.value >= right.value};

        if (_operator_name == "<=") return Result{.status = EvalStatus::OK, .value = left.value <= right.value};

        if (_operator_name == "==") return Result{.status = EvalStatus::OK, .value = left.value == right.value};

        if (_operator_name == "!=") return Result{.status = EvalStatus::OK, .value = left.value != right.value};

        if (_operator_name == "&&") return Result{.status = EvalStatus::OK, .value = left.value && right.value};

        if (_operator_name == "||") return Result{.status = EvalStatus::OK, .value = left.value || right.value};

        return Result{.status = EvalStatus::EXCEPTION, .value = 0};
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
