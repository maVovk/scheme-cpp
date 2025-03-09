#include "utils/object.h"
#include "utils/base_object.h"
#include "utils/error.h"
#include "utils/tokenizer.h"

#include <cstddef>
#include <iostream>
#include <ostream>

// #define DEBUG

std::ostream &operator<<(std::ostream &out, std::shared_ptr<Object> obj) {
#ifdef DEBUG
    if (!obj) {
        out << "nullptr";
    }
#endif
#ifndef DEBUG
    if (!obj) {
        out << "()";
    }
#endif

    if (Is<Number>(obj)) {
#ifdef DEBUG
        if (As<Number>(obj)->IsBoolean()) {
            out << "bool{" << As<Number>(obj)->GetBooleanValue() << "}";
        } else {
            out << "num{" << As<Number>(obj)->GetValue() << "}";
        }
#endif

#ifndef DEBUG
        auto num = As<Number>(obj);

        if (num->IsBoolean()) {
            out << (num->GetBooleanValue() ? "#t" : "#f");
        } else {
            out << num->GetValue();
        }
#endif
    }
    if (Is<Symbol>(obj)) {
#ifdef DEBUG
        out << "symb{" << As<Symbol>(obj)->GetName() << "}";
#endif

#ifndef DEBUG
        out << As<Symbol>(obj)->GetName();
#endif
    }
    if (Is<Cell>(obj)) {
#ifdef DEBUG
        out << "cell{" << As<Cell>(obj)->GetFirst() << "; "
            << As<Cell>(obj)->GetSecond() << "}";
#endif

#ifndef DEBUG
        auto first = As<Cell>(obj)->GetFirst();
        auto second = As<Cell>(obj)->GetSecond();

        out << first;

        if (second) {
            if (Is<Number>(second)) {
                out << " .";
            }

            out << " " << second;
        }
#endif
    }
#ifdef DEBUG
    if (Is<Null>(obj)) {
        out << "NULL; ";
    }
#endif

    return out;
}

bool Number::IsBoolean() const {
    return std::get_if<BooleanToken>(&token_) != nullptr;
}

int64_t Number::GetValue() const {
    if (const ConstantToken *p = std::get_if<ConstantToken>(&token_)) {
        return p->value;
    }

    throw RuntimeError{"Number object doen't hold ConstantToken"};
}

bool Number::GetBooleanValue() const {
    if (const BooleanToken *p = std::get_if<BooleanToken>(&token_)) {
        return p->value;
    }

    throw RuntimeError{"Number object doen't hold BooleanToken"};
}

Object::NodeType Number::Call(Object::NodeType) {
    throw RuntimeError{"Number is not callable"};
}

const std::string &Symbol::GetName() const {
    if (const SymbolToken *p = std::get_if<SymbolToken>(&token_)) {
        return p->name;
    }
    if (const QuoteToken *p = std::get_if<QuoteToken>(&token_)) {
        return p->kName;
    }

    throw RuntimeError{"Symbol object doen't hold SymbolToken"};
}

bool Symbol::Callable() const { return eval_ != nullptr; }

Object::NodeType Symbol::Call(Object::NodeType args) {
    if (!eval_) {
        return shared_from_this();
    }

    return eval_->Evaluate(args);
}

TokenType Reserved::GetType() const { return ::GetType(token_); }

Object::NodeType Reserved::Call(Object::NodeType) {
    throw SyntaxError{"Reserved symbol cannot be evaluated"};
}

void Cell::SetFirst(Object::NodeType other) { left_ = other; }

void Cell::SetSecond(Object::NodeType other) { right_ = other; }

Object::NodeType Cell::GetFirst() const { return left_; }

Object::NodeType Cell::GetSecond() const { return right_; }

Object::NodeType Cell::Call(Object::NodeType) {
    if (!left_) {
        throw RuntimeError{"Null is not callable"};
    }

    if (Is<Cell>(left_) && Is<Symbol>(As<Cell>(left_)->GetFirst()) &&
        As<Symbol>(As<Cell>(left_)->GetFirst())->GetName() == "quote" &&
        right_) {

        left_ = left_->Call(nullptr);

        if (!left_) {
            throw RuntimeError{"Not callable object"};
        }

        if (right_) {
            return left_->Call(right_);
        }

        return left_;
    }

    return left_->Call(right_);
}

void ToVector(Object::NodeType node, std::vector<Object::NodeType> &args) {
    if (Is<Cell>(node)) {
        if (Is<Symbol>(As<Cell>(node)->GetFirst()) &&
            As<Symbol>(As<Cell>(node)->GetFirst())->GetName() == "quote") {
            args.push_back(node);
            return;
        }

        args.push_back(As<Cell>(node)->GetFirst());

        if (!As<Cell>(node)->GetSecond()) {
            args.push_back(nullptr);
        } else {
            ToVector(As<Cell>(node)->GetSecond(), args);
        }
    } else if (node) {
        args.push_back(node);
    }
}

Object::NodeType FromVector(size_t pos, std::vector<Object::NodeType> &args) {
    if (pos + 1 == args.size()) {
        return args[pos];
    }

    std::shared_ptr<Cell> obj = std::make_shared<Cell>();
    obj->SetFirst(args[pos]);
    obj->SetSecond(FromVector(pos + 1, args));

    return obj;
}