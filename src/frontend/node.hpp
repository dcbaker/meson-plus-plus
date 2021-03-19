// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Frontend::AST {

class AdditiveExpression;
class Assignment;
class Boolean;
class Identifier;
class MultiplicativeExpression;
class Number;
class String;
class Subscript;
class UnaryExpression;
class Relational;
class FunctionCall;
class GetAttribute;
class Array;
class Dict;
class Ternary;

using ExpressionV =
    std::variant<std::unique_ptr<AdditiveExpression>, std::unique_ptr<Boolean>, std::unique_ptr<Identifier>,
                 std::unique_ptr<MultiplicativeExpression>, std::unique_ptr<UnaryExpression>, std::unique_ptr<Number>,
                 std::unique_ptr<String>, std::unique_ptr<Subscript>, std::unique_ptr<Relational>,
                 std::unique_ptr<FunctionCall>, std::unique_ptr<GetAttribute>, std::unique_ptr<Array>,
                 std::unique_ptr<Dict>, std::unique_ptr<Ternary>>;

using ExpressionList = std::vector<ExpressionV>;

class Number {
  public:
    Number(const int64_t & number) : value{number} {};
    Number(Number && n) noexcept : value{std::move(n.value)} {};
    Number(const Number &) = delete;
    ~Number(){};

    std::string as_string() const;

    int64_t value;
};

class Boolean {
  public:
    Boolean(const bool & b) : value{b} {};
    Boolean(Boolean && b) noexcept : value{std::move(b.value)} {};
    Boolean(const Boolean &) = delete;
    ~Boolean(){};

    std::string as_string() const;

    bool value;
};

class String {
  public:
    String(const std::string & str) : value{str} {};
    String(String && s) noexcept : value{std::move(s.value)} {};
    String(const String &) = delete;
    ~String(){};

    std::string as_string() const;

    std::string value;
};

class Identifier {
  public:
    Identifier(const std::string & str) : value{str} {};
    Identifier(Identifier && s) noexcept : value{std::move(s.value)} {};
    Identifier(const Identifier &) = delete;
    ~Identifier(){};

    std::string as_string() const;

    std::string value;
};

class Subscript {
  public:
    Subscript(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {};
    Subscript(const Subscript &) = delete;
    Subscript(Subscript && a) noexcept : lhs{std::move(a.lhs)}, rhs{std::move(a.rhs)} {};
    ~Subscript(){};

    std::string as_string() const;

    ExpressionV lhs;
    ExpressionV rhs;
};

enum class UnaryOp {
    NEG,
    NOT,
};

class UnaryExpression {
  public:
    UnaryExpression(const UnaryOp & o, ExpressionV && r) : op{o}, rhs{std::move(r)} {};
    UnaryExpression(UnaryExpression && a) noexcept : op{std::move(a.op)}, rhs{std::move(a.rhs)} {};
    UnaryExpression(const UnaryExpression &) = delete;
    ~UnaryExpression(){};

    std::string as_string() const;

    UnaryOp op;
    ExpressionV rhs;
};

enum class MulOp {
    MUL,
    DIV,
    MOD,
};

class MultiplicativeExpression {
  public:
    MultiplicativeExpression(ExpressionV && l, const MulOp & o, ExpressionV && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    MultiplicativeExpression(MultiplicativeExpression && a) noexcept
        : lhs{std::move(a.lhs)}, op{std::move(a.op)}, rhs{std::move(a.rhs)} {};
    MultiplicativeExpression(const MultiplicativeExpression &) = delete;
    ~MultiplicativeExpression(){};

    std::string as_string() const;

    ExpressionV lhs;
    MulOp op;
    ExpressionV rhs;
};

enum class AddOp {
    ADD,
    SUB,
};

class AdditiveExpression {
  public:
    AdditiveExpression(ExpressionV && l, const AddOp & o, ExpressionV && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    AdditiveExpression(AdditiveExpression && a) noexcept
        : lhs{std::move(a.lhs)}, op{std::move(a.op)}, rhs{std::move(a.rhs)} {};
    AdditiveExpression(const AdditiveExpression &) = delete;
    ~AdditiveExpression(){};

    std::string as_string() const;

    ExpressionV lhs;
    AddOp op;
    ExpressionV rhs;
};

enum class RelationalOp {
    LT,
    LE,
    EQ,
    NE,
    GE,
    GT,
    AND,
    OR,
    IN,
    NOT_IN,
};

static AST::RelationalOp to_relop(const std::string & s) {
    if (s == "<") {
        return AST::RelationalOp::LT;
    } else if (s == "<=") {
        return AST::RelationalOp::LE;
    } else if (s == "==") {
        return AST::RelationalOp::EQ;
    } else if (s == "!=") {
        return AST::RelationalOp::NE;
    } else if (s == ">=") {
        return AST::RelationalOp::GE;
    } else if (s == ">") {
        return AST::RelationalOp::GT;
    } else if (s == "and") {
        return AST::RelationalOp::AND;
    } else if (s == "or") {
        return AST::RelationalOp::OR;
    } else if (s == "in") {
        return AST::RelationalOp::IN;
    } else if (s == "not in") {
        return AST::RelationalOp::NOT_IN;
    }
    assert(false);
}

class Relational {
  public:
    Relational(ExpressionV && l, const std::string & o, ExpressionV && r)
        : lhs{std::move(l)}, op{to_relop(o)}, rhs{std::move(r)} {};
    Relational(Relational && a) noexcept : lhs{std::move(a.lhs)}, op{std::move(a.op)}, rhs{std::move(a.rhs)} {};
    Relational(const Relational &) = delete;
    ~Relational(){};

    std::string as_string() const;

    ExpressionV lhs;
    RelationalOp op;
    ExpressionV rhs;
};

// XXX: this isn't really true, it's really an identifier : expressionv
using KeywordPair = std::tuple<ExpressionV, ExpressionV>;
using KeywordList = std::vector<KeywordPair>;

class Arguments {
  public:
    Arguments() : positional{} {};
    Arguments(ExpressionList && v) : positional{std::move(v)}, keyword{} {};
    Arguments(KeywordList && k) : positional{}, keyword{std::move(k)} {};
    Arguments(ExpressionList && v, KeywordList && k) : positional{std::move(v)}, keyword{std::move(k)} {};
    Arguments(Arguments && a) noexcept : positional{std::move(a.positional)}, keyword{std::move(a.keyword)} {};
    Arguments(const Arguments &) = delete;
    ~Arguments(){};

    std::string as_string() const;

    ExpressionList positional;
    KeywordList keyword;
};

class FunctionCall {
  public:
    FunctionCall(ExpressionV && i, std::unique_ptr<Arguments> && a) : id{std::move(i)}, args{std::move(a)} {};
    FunctionCall(FunctionCall && a) noexcept : id{std::move(a.id)}, args{std::move(a.args)} {};
    FunctionCall(const FunctionCall &) = delete;
    ~FunctionCall(){};

    std::string as_string() const;

    ExpressionV id;
    std::unique_ptr<Arguments> args;
};

class GetAttribute {
  public:
    GetAttribute(ExpressionV && o, ExpressionV && i) : object{std::move(o)}, id{std::move(i)} {};
    GetAttribute(GetAttribute && a) noexcept : object{std::move(a.object)}, id{std::move(a.id)} {};
    GetAttribute(const GetAttribute &) = delete;
    ~GetAttribute(){};

    std::string as_string() const;

    ExpressionV object;
    ExpressionV id;
};

class Array {
  public:
    Array() : elements{} {};
    Array(ExpressionList && e) : elements{std::move(e)} {};
    Array(Array && a) noexcept : elements{std::move(a.elements)} {};
    Array(const Array &) = delete;
    ~Array(){};

    std::string as_string() const;

    ExpressionList elements;
};

class Dict {
  public:
    Dict() : elements{} {};
    Dict(KeywordList && l);
    Dict(Dict && a) : elements{std::move(a.elements)} {};
    Dict(const Dict &) = delete;
    ~Dict(){};

    std::string as_string() const;

    std::unordered_map<ExpressionV, ExpressionV> elements;
};

class Ternary {
  public:
    Ternary(ExpressionV && c, ExpressionV && l, ExpressionV && r)
        : condition{std::move(c)}, lhs{std::move(l)}, rhs{std::move(r)} {};
    Ternary(const Ternary &) = delete;
    ~Ternary(){};

    std::string as_string() const;

    ExpressionV condition;
    ExpressionV lhs;
    ExpressionV rhs;
};

class Statement {
  public:
    Statement(ExpressionV && e) : expr{std::move(e)} {};
    Statement(Statement && a) noexcept : expr{std::move(a.expr)} {};
    Statement(const Statement &) = delete;
    ~Statement(){};

    std::string as_string() const;

    ExpressionV expr;
};

class Assignment {
  public:
    Assignment(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {};
    Assignment(Assignment && a) noexcept : lhs{std::move(a.lhs)}, rhs{std::move(a.rhs)} {};
    Assignment(const Assignment &) = delete;
    ~Assignment(){};

    std::string as_string() const;

    ExpressionV lhs;
    ExpressionV rhs;
};

class IfStatement;
class ForeachStatement;

using StatementV = std::variant<std::unique_ptr<Statement>, std::unique_ptr<Assignment>, std::unique_ptr<IfStatement>,
                                std::unique_ptr<ForeachStatement>>;

class CodeBlock {
  public:
    CodeBlock() : statements{} {};
    CodeBlock(StatementV && stmt) : statements{} {
        statements.emplace_back(std::move(stmt));
    };
    CodeBlock(CodeBlock && b) noexcept : statements{std::move(b.statements)} {};
    CodeBlock(const CodeBlock &) = delete;
    ~CodeBlock(){};

    CodeBlock & operator=(CodeBlock &&) = default;

    std::string as_string() const;

    // XXX: this should probably be a statement list
    std::vector<StatementV> statements;
};

class IfBlock {
  public:
    IfBlock(){};
    IfBlock(ExpressionV && cond) : condition{std::move(cond)}, block{} {};
    IfBlock(ExpressionV && cond, std::unique_ptr<CodeBlock> && b) : condition{std::move(cond)}, block{std::move(b)} {};
    IfBlock(IfBlock && i) noexcept : condition{std::move(i.condition)}, block{std::move(i.block)} {};
    IfBlock(const IfBlock &) = delete;
    ~IfBlock(){};

    IfBlock & operator=(IfBlock &&) = default;

    ExpressionV condition;
    std::unique_ptr<CodeBlock> block;
};

class ElifBlock {
  public:
    ElifBlock() : condition{}, block{} {};
    ElifBlock(ExpressionV && cond, std::unique_ptr<CodeBlock> && b)
        : condition{std::move(cond)}, block{std::move(b)} {};
    ElifBlock(ElifBlock && e) noexcept : condition{std::move(e.condition)}, block{std::move(e.block)} {};
    ElifBlock(const ElifBlock &) = delete;
    ~ElifBlock(){};

    ElifBlock & operator=(ElifBlock &&) = default;

    ExpressionV condition;
    std::unique_ptr<CodeBlock> block;
};

class ElseBlock {
  public:
    ElseBlock() : block{} {};
    ElseBlock(std::unique_ptr<CodeBlock> && b) : block{std::move(b)} {};
    ElseBlock(ElseBlock && e) noexcept : block{std::move(e.block)} {};
    ElseBlock(const ElseBlock &) = delete;
    ~ElseBlock(){};

    ElseBlock & operator=(ElseBlock &&) = default;

    std::unique_ptr<CodeBlock> block;
};

class IfStatement {
  public:
    IfStatement(IfBlock && ib) : ifblock{std::move(ib)}, efblock{}, eblock{} {};
    IfStatement(IfBlock && ib, ElseBlock && eb) : ifblock{std::move(ib)}, efblock{}, eblock{std::move(eb)} {};
    IfStatement(IfBlock && ib, std::vector<ElifBlock> && ef)
        : ifblock{std::move(ib)}, efblock{std::move(ef)}, eblock{} {};
    IfStatement(IfBlock && ib, std::vector<ElifBlock> && ef, ElseBlock && eb)
        : ifblock{std::move(ib)}, efblock{std::move(ef)}, eblock{std::move(eb)} {};
    IfStatement(IfStatement && i)
        : ifblock{std::move(i.ifblock)}, efblock{std::move(i.efblock)}, eblock{std::move(i.eblock)} {};
    IfStatement(const IfStatement &) = delete;
    ~IfStatement(){};

    std::string as_string() const;

    IfBlock ifblock;
    std::vector<ElifBlock> efblock;
    ElseBlock eblock;
};

class ForeachStatement {
  public:
    ForeachStatement(Identifier && i, ExpressionV && e, std::unique_ptr<CodeBlock> && b)
        : id{std::move(i)}, expr{std::move(e)}, block{std::move(b)} {};
    ForeachStatement(ForeachStatement && f) noexcept
        : id{std::move(f.id)}, expr{std::move(f.expr)}, block{std::move(f.block)} {};
    ForeachStatement(const ForeachStatement &) = delete;
    ~ForeachStatement(){};

    std::string as_string() const;

    Identifier id;
    ExpressionV expr;
    std::unique_ptr<CodeBlock> block;
};

} // namespace Frontend::AST
