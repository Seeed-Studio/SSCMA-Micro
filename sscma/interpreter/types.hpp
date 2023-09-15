#pragma once

#include <functional>
#include <string>
#include <vector>

namespace sscma::interpreter::types {

static const char lparn = '(';
static const char rparn = ')';

enum class TokenType { IDENTIFIER, CONSTANT, OPERATOR, LPARN, RPARN, FUNCTION, TERM };

struct Token {
    TokenType   type;
    std::string value;
};

enum class NodeType { IDENTIFIER, FUNCTION_CALL };

enum class EvalStatus { OK, EXCEPTION };

struct Result {
    EvalStatus status;
    int        value;
};

typedef std::vector<Token> Mutables;

typedef std::function<Result(NodeType, const std::string&)> EvalCallback;

class ASTNode {
   public:
    virtual ~ASTNode() = default;

    virtual Result evaluate(EvalCallback callback) const = 0;
};

}  // namespace sscma::interpreter::types
