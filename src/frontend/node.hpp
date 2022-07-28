// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "locations.hpp"

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
    std::variant<std::unique_ptr<AdditiveExpression>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Identifier>, std::unique_ptr<MultiplicativeExpression>,
                 std::unique_ptr<UnaryExpression>, std::unique_ptr<Number>, std::unique_ptr<String>,
                 std::unique_ptr<Subscript>, std::unique_ptr<Relational>,
                 std::unique_ptr<FunctionCall>, std::unique_ptr<GetAttribute>,
                 std::unique_ptr<Array>, std::unique_ptr<Dict>, std::unique_ptr<Ternary>>;

using ExpressionList = std::vector<ExpressionV>;

class Location {
  public:
    Location(const location & l)
        : column_start{l.begin.column}, column_end{l.end.column},
          line_start{l.begin.line}, line_end{l.end.line}, filename{*l.begin.filename} {};
    virtual ~Location() = default;
    Location(const Location &) = default;

    const int column_start;
    const int column_end;
    const int line_start;
    const int line_end;
    const std::string filename;
};

class Number {
  public:
    Number(const int64_t & number, const location & l) : value{number}, loc{l} {};
    Number(Number && n) noexcept : value{n.value}, loc{n.loc} {};
    Number(const Number &) = delete;
    ~Number() = default;

    std::string as_string() const;

    int64_t value;
    Location loc;
};

class Boolean {
  public:
    Boolean(const bool & b, const location & l) : value{b}, loc{l} {};
    Boolean(Boolean && b) noexcept : value{b.value}, loc{b.loc} {};
    Boolean(const Boolean &) = delete;
    ~Boolean() = default;

    std::string as_string() const;

    bool value;
    Location loc;
};

class String {
  public:
    String(std::string str, const bool & t, const bool & f, const location & l)
        : value{std::move(str)}, is_triple{t}, is_fstring{f}, loc{l} {};
    String(String && s) noexcept
        : value{std::move(s.value)}, is_triple{s.is_triple},
          is_fstring{s.is_fstring}, loc{s.loc} {};
    String(const String &) = delete;
    ~String() = default;

    std::string as_string() const;

    std::string value;
    bool is_triple;
    bool is_fstring;
    Location loc;
};

class Identifier {
  public:
    Identifier(std::string str, const location & l) : value{std::move(str)}, loc{l} {};
    Identifier(Identifier && s) noexcept : value{std::move(s.value)}, loc{s.loc} {};
    Identifier(const Identifier &) = delete;
    ~Identifier() = default;

    std::string as_string() const;

    std::string value;
    Location loc;
};

class Subscript {
  public:
    Subscript(ExpressionV && l, ExpressionV && r, location & lo)
        : lhs{std::move(l)}, rhs{std::move(r)}, loc{lo} {};
    Subscript(const Subscript &) = delete;
    Subscript(Subscript && a) noexcept
        : lhs{std::move(a.lhs)}, rhs{std::move(a.rhs)}, loc{a.loc} {};
    ~Subscript() = default;

    std::string as_string() const;

    ExpressionV lhs;
    ExpressionV rhs;
    Location loc;
};

enum class UnaryOp {
    NEG,
    NOT,
};

class UnaryExpression {
  public:
    UnaryExpression(const UnaryOp & o, ExpressionV && r, location & l)
        : op{o}, rhs{std::move(r)}, loc{l} {};
    UnaryExpression(UnaryExpression && a) noexcept : op{a.op}, rhs{std::move(a.rhs)}, loc{a.loc} {};
    UnaryExpression(const UnaryExpression &) = delete;
    ~UnaryExpression() = default;

    std::string as_string() const;

    UnaryOp op;
    ExpressionV rhs;
    Location loc;
};

enum class MulOp {
    MUL,
    DIV,
    MOD,
};

class MultiplicativeExpression {
  public:
    MultiplicativeExpression(ExpressionV && l, const MulOp & o, ExpressionV && r, location & lo)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)}, loc{lo} {};
    MultiplicativeExpression(MultiplicativeExpression && a) noexcept
        : lhs{std::move(a.lhs)}, op{a.op}, rhs{std::move(a.rhs)}, loc{a.loc} {};
    MultiplicativeExpression(const MultiplicativeExpression &) = delete;
    ~MultiplicativeExpression() = default;

    std::string as_string() const;

    ExpressionV lhs;
    MulOp op;
    ExpressionV rhs;
    Location loc;
};

enum class AddOp {
    ADD,
    SUB,
};

class AdditiveExpression {
  public:
    AdditiveExpression(ExpressionV && l, const AddOp & o, ExpressionV && r, location & lo)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)}, loc{lo} {};
    AdditiveExpression(AdditiveExpression && a) noexcept
        : lhs{std::move(a.lhs)}, op{a.op}, rhs{std::move(a.rhs)}, loc{a.loc} {};
    AdditiveExpression(const AdditiveExpression &) = delete;
    ~AdditiveExpression() = default;

    std::string as_string() const;

    ExpressionV lhs;
    AddOp op;
    ExpressionV rhs;
    Location loc;
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

// TODO: move this into the parser cpp
static AST::RelationalOp to_relop(const std::string & s) {
    if (s == "<")
        return AST::RelationalOp::LT;
    if (s == "<=")
        return AST::RelationalOp::LE;
    if (s == "==")
        return AST::RelationalOp::EQ;
    if (s == "!=")
        return AST::RelationalOp::NE;
    if (s == ">=")
        return AST::RelationalOp::GE;
    if (s == ">")
        return AST::RelationalOp::GT;
    if (s == "and")
        return AST::RelationalOp::AND;
    if (s == "or")
        return AST::RelationalOp::OR;
    if (s == "in")
        return AST::RelationalOp::IN;
    if (s == "not in")
        return AST::RelationalOp::NOT_IN;

    assert(false);
}

class Relational {
  public:
    Relational(ExpressionV && l, const std::string & o, ExpressionV && r, location & lo)
        : lhs{std::move(l)}, op{to_relop(o)}, rhs{std::move(r)}, loc{lo} {};
    Relational(Relational && a) noexcept
        : lhs{std::move(a.lhs)}, op{a.op}, rhs{std::move(a.rhs)}, loc{a.loc} {};
    Relational(const Relational &) = delete;
    ~Relational() = default;

    std::string as_string() const;

    ExpressionV lhs;
    RelationalOp op;
    ExpressionV rhs;
    Location loc;
};

// XXX: this isn't really true, it's really an identifier : expressionv
using KeywordPair = std::tuple<ExpressionV, ExpressionV>;
using KeywordList = std::vector<KeywordPair>;

class Arguments {
  public:
    Arguments(location & l) : loc{l} {};
    Arguments(ExpressionList && v, location & l) : positional{std::move(v)}, loc{l} {};
    Arguments(KeywordList && k, location & l) : keyword{std::move(k)}, loc{l} {};
    Arguments(ExpressionList && v, KeywordList && k, location & l)
        : positional{std::move(v)}, keyword{std::move(k)}, loc{l} {};
    Arguments(Arguments && a) noexcept
        : positional{std::move(a.positional)}, keyword{std::move(a.keyword)}, loc{a.loc} {};
    Arguments(const Arguments &) = delete;
    ~Arguments() = default;

    std::string as_string() const;

    ExpressionList positional;
    KeywordList keyword;
    Location loc;
};

class FunctionCall {
  public:
    FunctionCall(ExpressionV && i, std::unique_ptr<Arguments> && a, location & l)
        : held{std::move(i)}, args{std::move(a)}, loc{l} {};
    FunctionCall(FunctionCall && a) noexcept
        : held{std::move(a.held)}, args{std::move(a.args)}, loc{a.loc} {};
    FunctionCall(const FunctionCall &) = delete;
    ~FunctionCall() = default;

    std::string as_string() const;

    ExpressionV held;
    std::unique_ptr<Arguments> args;
    Location loc;
};

class GetAttribute {
  public:
    GetAttribute(ExpressionV && o, ExpressionV && i, location & l)
        : holder{std::move(o)}, held{std::move(i)}, loc{l} {};
    GetAttribute(GetAttribute && a) noexcept
        : holder{std::move(a.holder)}, held{std::move(a.held)}, loc{a.loc} {};
    GetAttribute(const GetAttribute &) = delete;
    ~GetAttribute() = default;

    std::string as_string() const;

    /// Object holding the attribute
    ExpressionV holder;
    /// The attribute to get (really, the method)
    ExpressionV held;
    Location loc;
};

class Array {
  public:
    Array(location & l) : loc{l} {};
    Array(ExpressionList && e, location & l) : elements{std::move(e)}, loc{l} {};
    Array(Array && a) noexcept : elements{std::move(a.elements)}, loc{a.loc} {};
    Array(const Array &) = delete;
    ~Array() = default;

    std::string as_string() const;

    ExpressionList elements;
    Location loc;
};

class Dict {
  public:
    Dict(location & l) : loc{l} {};
    Dict(KeywordList && l, location & lo);
    Dict(Dict && a) noexcept : elements{std::move(a.elements)}, loc{a.loc} {};
    Dict(const Dict &) = delete;
    ~Dict() = default;

    std::string as_string() const;

    std::unordered_map<ExpressionV, ExpressionV> elements;
    Location loc;
};

class Ternary {
  public:
    Ternary(ExpressionV && c, ExpressionV && l, ExpressionV && r, location & lo)
        : condition{std::move(c)}, lhs{std::move(l)}, rhs{std::move(r)}, loc{lo} {};
    Ternary(Ternary && t) noexcept
        : condition{std::move(t.condition)}, lhs{std::move(t.lhs)}, rhs{std::move(t.rhs)},
          loc{t.loc} {};
    Ternary(const Ternary &) = delete;
    ~Ternary() = default;

    std::string as_string() const;

    ExpressionV condition;
    ExpressionV lhs;
    ExpressionV rhs;
    Location loc;
};

class Statement {
  public:
    Statement(ExpressionV && e) : expr{std::move(e)} {};
    Statement(Statement && a) noexcept : expr{std::move(a.expr)} {};
    Statement(const Statement &) = delete;
    ~Statement() = default;

    std::string as_string() const;

    ExpressionV expr;
};

enum class AssignOp {
    EQUAL,
    ADD_EQUAL,
    SUB_EQUAL,
    MUL_EQUAL,
    DIV_EQUAL,
    MOD_EQUAL,
};

class Assignment {
  public:
    Assignment(ExpressionV && l, AssignOp & o, ExpressionV && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    Assignment(Assignment && a) noexcept
        : lhs{std::move(a.lhs)}, op{a.op}, rhs{std::move(a.rhs)} {};
    Assignment(const Assignment &) = delete;
    ~Assignment() = default;

    std::string as_string() const;

    ExpressionV lhs;
    AssignOp op;
    ExpressionV rhs;
};

class Break {
  public:
    Break() = default;
    ~Break() = default;
    Break(const Break &) = delete;

    std::string as_string() const;
};

class Continue {
  public:
    Continue() = default;
    ~Continue() = default;
    Continue(const Break &) = delete;

    std::string as_string() const;
};

class IfStatement;
class ForeachStatement;

using StatementV = std::variant<std::unique_ptr<Statement>, std::unique_ptr<Assignment>,
                                std::unique_ptr<IfStatement>, std::unique_ptr<ForeachStatement>,
                                std::unique_ptr<Break>, std::unique_ptr<Continue>>;

class CodeBlock {
  public:
    CodeBlock() = default;
    CodeBlock(StatementV && stmt) { statements.emplace_back(std::move(stmt)); };
    CodeBlock(CodeBlock && b) noexcept : statements{std::move(b.statements)} {};
    CodeBlock(const CodeBlock &) = delete;
    ~CodeBlock() = default;

    CodeBlock & operator=(CodeBlock &&) = default;

    std::string as_string() const;

    // XXX: this should probably be a statement list
    std::vector<StatementV> statements;
};

class IfBlock {
  public:
    IfBlock() = default;
    IfBlock(ExpressionV && cond) : condition{std::move(cond)} {};
    IfBlock(ExpressionV && cond, std::unique_ptr<CodeBlock> && b)
        : condition{std::move(cond)}, block{std::move(b)} {};
    IfBlock(IfBlock && i) noexcept
        : condition{std::move(i.condition)}, block{std::move(i.block)} {};
    IfBlock(const IfBlock &) = delete;
    ~IfBlock() = default;

    IfBlock & operator=(IfBlock &&) = default;

    ExpressionV condition;
    std::unique_ptr<CodeBlock> block;
};

class ElifBlock {
  public:
    ElifBlock() = default;
    ElifBlock(ExpressionV && cond, std::unique_ptr<CodeBlock> && b)
        : condition{std::move(cond)}, block{std::move(b)} {};
    ElifBlock(ElifBlock && e) noexcept
        : condition{std::move(e.condition)}, block{std::move(e.block)} {};
    ElifBlock(const ElifBlock &) = delete;
    ~ElifBlock() = default;

    ElifBlock & operator=(ElifBlock &&) = default;

    ExpressionV condition;
    std::unique_ptr<CodeBlock> block;
};

class ElseBlock {
  public:
    ElseBlock() : block{nullptr} {};
    ElseBlock(std::unique_ptr<CodeBlock> && b) : block{std::move(b)} {};
    ElseBlock(ElseBlock && e) noexcept : block{std::move(e.block)} {};
    ElseBlock(const ElseBlock &) = delete;
    ~ElseBlock() = default;

    ElseBlock & operator=(ElseBlock &&) = default;

    std::unique_ptr<CodeBlock> block;
};

class IfStatement {
  public:
    IfStatement(IfBlock && ib) : ifblock{std::move(ib)} {};
    IfStatement(IfBlock && ib, ElseBlock && eb) : ifblock{std::move(ib)}, eblock{std::move(eb)} {};
    IfStatement(IfBlock && ib, std::vector<ElifBlock> && ef)
        : ifblock{std::move(ib)}, efblock{std::move(ef)} {};
    IfStatement(IfBlock && ib, std::vector<ElifBlock> && ef, ElseBlock && eb)
        : ifblock{std::move(ib)}, efblock{std::move(ef)}, eblock{std::move(eb)} {};
    IfStatement(IfStatement && i) noexcept
        : ifblock{std::move(i.ifblock)}, efblock{std::move(i.efblock)}, eblock{
                                                                            std::move(i.eblock)} {};
    IfStatement(const IfStatement &) = delete;
    ~IfStatement() = default;

    std::string as_string() const;

    IfBlock ifblock;
    std::vector<ElifBlock> efblock;
    ElseBlock eblock;
};

class ForeachStatement {
  public:
    ForeachStatement(Identifier && i, ExpressionV && e, std::unique_ptr<CodeBlock> && b)
        : id{std::move(i)}, id2{std::nullopt}, expr{std::move(e)}, block{std::move(b)} {};
    ForeachStatement(Identifier && i, Identifier && j, ExpressionV && e,
                     std::unique_ptr<CodeBlock> && b)
        : id{std::move(i)}, id2{std::move(j)}, expr{std::move(e)}, block{std::move(b)} {};
    ForeachStatement(ForeachStatement && f) noexcept
        : id{std::move(f.id)}, id2{std::move(f.id2)}, expr{std::move(f.expr)}, block{std::move(
                                                                                   f.block)} {};
    ForeachStatement(const ForeachStatement &) = delete;
    ~ForeachStatement() = default;

    std::string as_string() const;

    Identifier id;
    // Used only in dictionary iteration
    std::optional<Identifier> id2;
    ExpressionV expr;
    std::unique_ptr<CodeBlock> block;
};

} // namespace Frontend::AST
