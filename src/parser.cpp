#include "utils/parser.h"
#include "utils/base_object.h"
#include "utils/error.h"
#include "utils/evaluator.h"
#include "utils/object.h"
#include "utils/tokenizer.h"

#include <cstddef>
#include <iostream>
#include <memory>

Object::NodeType ConvertNull(Object::NodeType obj) {
    if (Is<Null>(obj)) {
        obj.reset();
        return nullptr;
    }

    if (Is<Cell>(obj)) {
        auto left = As<Cell>(obj)->GetFirst();
        auto right = As<Cell>(obj)->GetSecond();

        if (left) {
            As<Cell>(obj)->SetFirst(ConvertNull(left));
        } else {
            throw SyntaxError{"nullptr in AST"};
        }

        if (right) {
            As<Cell>(obj)->SetSecond(ConvertNull(right));
        } else {
            throw SyntaxError{"nullptr in AST"};
        }
    }

    return obj;
}

std::shared_ptr<Object> ReadToken(Tokenizer *tokenizer) {
    if (tokenizer->IsEnd()) {
        throw SyntaxError{"Empty token"};
    }

    Token token = tokenizer->GetToken();
    tokenizer->Next();

    switch (GetType(token)) {
    case TokenType::Quote:
        return std::make_shared<Symbol>(QuoteToken{});
    case TokenType::Constant:
    case TokenType::Boolean:
        return std::make_shared<Number>(token);
    case TokenType::Symbol:
        return std::make_shared<Symbol>(token);
    case TokenType::OpenBracket:
    case TokenType::CloseBracket:
    case TokenType::Dot:
        return std::make_shared<Reserved>(token);
    case TokenType::None:
        throw SyntaxError{"None token shouldn't be parsed"};
    }

    throw SyntaxError{"Unexpected token"};
}

std::shared_ptr<Object> Read(Tokenizer *tokenizer, bool first) {
    size_t input_counter = 0;
    std::shared_ptr<Object> root = std::make_shared<Cell>();
    auto root_cell = As<Cell>(root);

    if (!tokenizer->IsEnd()) {
        auto token = ReadToken(tokenizer);
        ++input_counter;

        if (Is<Reserved>(token)) {
            if (As<Reserved>(token)->GetType() == TokenType::OpenBracket) {
                if (!root_cell->GetFirst()) {
                    root_cell->SetFirst(ReadList(tokenizer));

                    if (!Is<Cell>(root_cell->GetFirst()) &&
                        !Is<Null>(root_cell->GetFirst())) {
                        throw SyntaxError{"Wrong list initialization"};
                    }
                } else if (!root_cell->GetSecond()) {
                    root_cell->SetSecond(ReadList(tokenizer));

                    if (!Is<Cell>(root_cell->GetFirst()) &&
                        !Is<Null>(root_cell->GetSecond())) {
                        throw SyntaxError{"Wrong list initialization"};
                    }
                } else {
                    throw SyntaxError{"Invalid list instruction"};
                }
            } else if (As<Reserved>(token)->GetType() ==
                       TokenType::CloseBracket) {
                if (!first) {
                    return root;
                }
            } else {
                throw SyntaxError{"Invalid dot usage"};
            }
        } else {
            root_cell->SetFirst(token);
        }
    }

    if (first) {
        if (!tokenizer->IsEnd()) {
            if (root_cell->GetSecond()) {
                throw SyntaxError{"Invalid list instruction"};
            }

            root_cell->SetSecond(Read(tokenizer, false));
        }

        if (!tokenizer->CheckBrackets()) {
            throw SyntaxError{"Unmatched brackets"};
        }
        if (input_counter == 0) {
            throw SyntaxError{"Empty input is prohibited"};
        }
    }

    if (!root_cell->GetSecond()) {
        if (Is<Symbol>(root_cell->GetFirst()) &&
            As<Symbol>(root_cell->GetFirst())->GetName() == "quote") {
            throw SyntaxError{"Single quote is banned"};
        }

        if (first) {
            root = ConvertNull(root_cell->GetFirst());
        } else {
            root = root_cell->GetFirst();
        }

        return root;
    }

    // NULL -> nullptr
    // if nullptr -> SyntaxError
    if (first) {
        root = ConvertNull(root);
    }

    return root;
}

std::shared_ptr<Object> ReadList(Tokenizer *tokenizer) {
    std::shared_ptr<Cell> node{};

    auto token = ReadToken(tokenizer);

    if (Is<Reserved>(token)) {
        auto token_type = As<Reserved>(token)->GetType();

        if (token_type == TokenType::OpenBracket) {
            if (!node) {
                node = std::make_shared<Cell>();
                node->SetFirst(ReadList(tokenizer));
            }

            node->SetSecond(ReadList(tokenizer));
            return node;
        } else if (token_type == TokenType::CloseBracket) {
            if (!node) {
                return std::make_shared<Null>(); // NULL -> () empty list
            } else if (!node->GetSecond()) {
                node->SetSecond(nullptr);
            }

            return node;
        } else if (token_type == TokenType::Dot) {
            auto second_element = Read(tokenizer, false);
            auto closed_bracket = ReadToken(tokenizer);

            // check if next is number?
            if (!second_element) {
                throw SyntaxError{"Incorrect pair usage"};
            }
            if (!Is<Reserved>(closed_bracket) ||
                As<Reserved>(closed_bracket)->GetType() !=
                    TokenType::CloseBracket) {
                throw SyntaxError{"No closing bracket in the end of pair"};
            }

            return second_element;
        }
    } else if (Is<Symbol>(token) && As<Symbol>(token)->GetName() == "quote") {
        std::shared_ptr<Cell> quote_obj = std::make_shared<Cell>();

        quote_obj->SetFirst(token);
        quote_obj->SetSecond(Read(tokenizer, false));

        node = std::make_shared<Cell>();

        node->SetFirst(quote_obj);
        node->SetSecond(ReadList(tokenizer));

        return node;
    } else {
        node = std::make_shared<Cell>();

        node->SetFirst(token);
        node->SetSecond(ReadList(tokenizer));

        return node;
    }

    throw SyntaxError{"ReadList error"};
}
