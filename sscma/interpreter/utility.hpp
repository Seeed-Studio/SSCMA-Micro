#pragma once

#include <locale>

#include "sscma/interpreter/types.hpp"

namespace sscma::interpreter::utility {

using namespace sscma::interpreter::types;

bool is_identifier(char c) { return std::isalnum(c) || c == '_'; }

bool is_constant(char c) { return std::isdigit(c); }

bool is_arithmetic_operator(char c) { return c == '*' || c == '/' || c == '+' || c == '-'; }

bool is_relational_operator_pred(char c) { return c == '<' || c == '>' || c == '=' || c == '!'; }

bool is_relational_operator_term(char c) { return c == '='; }

bool is_logical_operator(char c) { return c == '&' || c == '|'; }

bool is_lparn(char c) { return c == lparn; }

bool is_rparn(char c) { return c == rparn; }

bool is_comma(char c) { return c == ','; }

}  // namespace sscma::interpreter::utility
