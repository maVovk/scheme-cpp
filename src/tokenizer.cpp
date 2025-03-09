#include "utils/tokenizer.h"
#include "utils/error.h"

#include <cctype>
#include <string>

TokenType DefineType(const std::string &str) {
    for (const auto &[type, regex] : kTokenRegexes) {
        if (std::regex_match(str, regex)) {
            return type;
        }
    }

    return TokenType::None;
}

TokenType GetType(const Token &token) {
    if (std::holds_alternative<ConstantToken>(token)) {
        return TokenType::Constant;
    }
    if (std::holds_alternative<BracketToken>(token)) {
        return std::get<BracketToken>(token) == BracketToken::OPEN
                   ? TokenType::OpenBracket
                   : TokenType::CloseBracket;
    }
    if (std::holds_alternative<SymbolToken>(token)) {
        return TokenType::Symbol;
    }
    if (std::holds_alternative<QuoteToken>(token)) {
        return TokenType::Quote;
    }
    if (std::holds_alternative<DotToken>(token)) {
        return TokenType::Dot;
    }
    if (std::holds_alternative<BooleanToken>(token)) {
        return TokenType::Boolean;
    }

    return TokenType::None;
}

Token *CreateToken(TokenType type, const std::string &str) {
    switch (type) {
    case TokenType::Symbol:
        return new Token{SymbolToken{str}};
    case TokenType::Quote:
        return new Token{QuoteToken{}};
    case TokenType::Dot:
        return new Token{DotToken{}};
    case TokenType::OpenBracket:
        return new Token{BracketToken::OPEN};
    case TokenType::CloseBracket:
        return new Token{BracketToken::CLOSE};
    case TokenType::Constant:
        return new Token{ConstantToken{std::stoi(str)}};
    case TokenType::Boolean:
        return new Token{BooleanToken{str == "#t"}};
    case TokenType::None:
        throw SyntaxError{"Unkown token"};
    }

    throw SyntaxError{"Unkown token"};
}

Tokenizer::Tokenizer(std::istream *in) : input_stream_(in) { Next(); };

bool SymbolToken::operator==(const SymbolToken &other) const {
    return name == other.name;
}

const std::string QuoteToken::kName = "quote";

bool QuoteToken::operator==(const QuoteToken &) const { return true; }

bool DotToken::operator==(const DotToken &) const { return true; }

bool ConstantToken::operator==(const ConstantToken &other) const {
    return value == other.value;
}

bool BooleanToken::operator==(const BooleanToken &other) const {
    return value == other.value;
}

void Tokenizer::Update(std::istream *in) {
    input_stream_ = in;
    Reset();
    Next();
}

bool Tokenizer::IsEnd() { return !current_token_.get(); }

void Tokenizer::Next() {
    TokenType current_token_type;
    std::string current_token_string;

    while (true) {
        auto symb = input_stream_->peek();
        current_token_string.push_back(symb);

        TokenType defined_type = DefineType(current_token_string);

        if (symb != std::char_traits<char>::eof() &&
            defined_type != TokenType::None) {
            input_stream_->get();
            current_token_type = defined_type;
            continue;
        }

        current_token_string.pop_back();

        if (current_token_string.empty()) {
            if (symb == std::char_traits<char>::eof()) {
                current_token_.reset(nullptr);
                return;
            }

            input_stream_->get();
            continue;
        }

        if (current_token_type == TokenType::OpenBracket) {
            ++opened_;
        }
        if (current_token_type == TokenType::CloseBracket) {
            --opened_;
        }

        current_token_.reset(
            CreateToken(current_token_type, current_token_string));
        break;
    }
}

void Tokenizer::Reset() { opened_ = 0; }

bool Tokenizer::CheckBrackets() { return !opened_; }

Token Tokenizer::GetToken() { return *current_token_; }