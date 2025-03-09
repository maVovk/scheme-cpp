#pragma once

#include <istream>
#include <map>
#include <memory>
#include <regex>
#include <variant>

struct SymbolToken {
    std::string name;

    bool operator==(const SymbolToken &other) const;
};

struct QuoteToken {
    static const std::string kName;

    bool operator==(const QuoteToken &) const;
};

struct DotToken {
    bool operator==(const DotToken &) const;
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    int64_t value;

    bool operator==(const ConstantToken &other) const;
};

struct BooleanToken {
    bool value;

    bool operator==(const BooleanToken &other) const;
};

enum class TokenType {
    Symbol,
    Quote,
    Dot,
    OpenBracket,
    CloseBracket,
    Constant,
    Boolean,
    None
};

using Token = std::variant<ConstantToken, BooleanToken, BracketToken,
                           SymbolToken, QuoteToken, DotToken>;

const std::map<TokenType, std::regex> kTokenRegexes{
    {TokenType::Constant, std::regex{"(-|\\+)?[\\d]+"}},
    {TokenType::Boolean, std::regex{"#[t|f]"}},
    {TokenType::Symbol,
     std::regex{"(?!#(?:t|f)\\b)([a-zA-Z<=>\\*\\/#][a-zA-Z<=>\\*\\/"
                "#0-9\\?\\!-]*|[\\+\\*\\/]|-(?=(\\D|$)))"}},
    {TokenType::Quote, std::regex{"'"}},
    {TokenType::Dot, std::regex{"\\."}},
    {TokenType::OpenBracket, std::regex{"\\("}},
    {TokenType::CloseBracket, std::regex{"\\)"}}};

TokenType DefineType(const std::string &str);

TokenType GetType(const Token &token);

class Tokenizer {
  public:
    Tokenizer() = default;

    Tokenizer(std::istream *in);

    void Update(std::istream *in);

    bool IsEnd();

    void Next();

    void Reset();

    bool CheckBrackets();

    Token GetToken();

  private:
    int opened_ = 0;

    std::istream *input_stream_ = nullptr;
    std::unique_ptr<Token> current_token_ = nullptr;
};