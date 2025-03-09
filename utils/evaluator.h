#pragma once

#include "base_object.h"
#include "tokenizer.h"

#include <map>
#include <memory>
#include <regex>

enum class EvalCategory {
    Predicator,
    Comparator,
    Arithmetical,
    Logical,
    ArrayFunctor,
    Functor,
    Quote,
    None
};

const std::map<EvalCategory, std::regex> kEvalCategoriesRegexes{
    {EvalCategory::Predicator, std::regex{"[a-z]+\\?"}},
    {EvalCategory::Comparator, std::regex{"(=|<=|>=|<|>)"}},
    {EvalCategory::Arithmetical, std::regex{"(\\+|-|\\*|/)"}},
    {EvalCategory::Logical, std::regex{"(and|or|not)"}},
    {EvalCategory::ArrayFunctor,
     std::regex{"(min|max|cons|car|cdr|list|list-ref|list-tail)"}},
    {EvalCategory::Functor, std::regex{"(abs)"}},
    {EvalCategory::Quote, std::regex{"quote"}},
};

class Evaluator {
  public:
    virtual ~Evaluator() = default;

    virtual Object::NodeType Evaluate(Object::NodeType args) = 0;
};

class Arithmetical : public Evaluator {
  public:
    enum class ArithmeticalOperations { Plus, Minus, Multiply, Divide };

    Arithmetical(const std::string &symbol);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    ArithmeticalOperations type_;
};

class Predicator : public Evaluator {
  public:
    enum class PredicateTypes { Integer, Boolean, Pair, List, Null };

    Predicator(const std::string &type);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    PredicateTypes type_;
};

class Comparator : public Evaluator {
  public:
    enum class CompareType { EQ, LE, GE, LS, GR };

    Comparator(const std::string &type);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    CompareType type_;
};

class ArrayFunctor : public Evaluator {
  public:
    enum class ArrayFunction {
        Min,
        Max,
        Cons,
        Car,
        Cdr,
        List,
        List_Ref,
        List_Tail
    };

    ArrayFunctor(const std::string &type);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    ArrayFunction type_;
};

class Functor : public Evaluator {
  public:
    enum class Function { Abs };

    Functor(const std::string &type);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    Function type_;
};

class Logical : public Evaluator {
  public:
    enum class LogicalOperation { And, Or, Not };

    Logical(const std::string &type);

    virtual Object::NodeType Evaluate(Object::NodeType args) override;

  private:
    LogicalOperation type_;
};

class Quote : public Evaluator {
  public:
    Quote() = default;

    virtual Object::NodeType Evaluate(Object::NodeType args) override;
};

EvalCategory DefineCategory(const std::string &symbol);

std::unique_ptr<Evaluator> GetEvaluator(const Token &token);