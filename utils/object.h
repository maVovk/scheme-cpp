#pragma once

#include "base_object.h"
#include "error.h"
#include "evaluator.h"
#include "tokenizer.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Number : public Object {
  public:
    Number(const Token &token) : token_(token) {}

    bool IsBoolean() const;

    int64_t GetValue() const;

    bool GetBooleanValue() const;

    virtual Object::NodeType Call(Object::NodeType args) override;

  private:
    Token token_;
};

class Symbol : public Object {
  public:
    Symbol(const Token &token) : token_(token), eval_(GetEvaluator(token)) {}

    const std::string &GetName() const;

    virtual bool Callable() const override;

    virtual Object::NodeType Call(Object::NodeType args) override;

  private:
    Token token_;
    std::unique_ptr<Evaluator> eval_;
};

class Reserved : public Object {
  public:
    Reserved(const Token &token) : token_(token) {}

    TokenType GetType() const;

    virtual Object::NodeType Call(Object::NodeType args) override;

  private:
    Token token_;
};

class Null : public Object {
  private:
    virtual Object::NodeType Call(Object::NodeType) override {
        throw RuntimeError{"Null cannot be evaluated"};
    }
};

class Cell : public Object {
  public:
    void SetFirst(Object::NodeType other);
    void SetSecond(Object::NodeType other);

    Object::NodeType GetFirst() const;
    Object::NodeType GetSecond() const;

    virtual Object::NodeType Call(Object::NodeType args) override;

  private:
    Object::NodeType left_;
    Object::NodeType right_;
};

///////////////////////////////////////////////////////////////////////////////

// Runtime type checking and conversion.
// This can be helpful:
// https://en.cppreference.com/w/cpp/memory/shared_ptr/pointer_cast

template <class T> std::shared_ptr<T> As(const std::shared_ptr<Object> &obj) {
    auto p = dynamic_cast<T *>(obj.get());

    if (p) {
        return std::shared_ptr<T>{obj, p};
    }
    return std::shared_ptr<T>{};
}

template <class T> bool Is(const std::shared_ptr<Object> &obj) {
    auto p = dynamic_cast<T *>(obj.get());

    return p != nullptr;
}

void ToVector(Object::NodeType node, std::vector<Object::NodeType> &args);

Object::NodeType FromVector(size_t pos, std::vector<Object::NodeType> &args);