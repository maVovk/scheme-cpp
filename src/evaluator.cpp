#include "utils/evaluator.h"
#include "utils/base_object.h"
#include "utils/object.h"
#include "utils/tokenizer.h"

#include <string>

EvalCategory DefineCategory(const std::string &symbol) {
    for (auto &[category, regex] : kEvalCategoriesRegexes) {
        if (std::regex_match(symbol, regex)) {
            return category;
        }
    }

    return EvalCategory::None;
}

std::unique_ptr<Evaluator> GetEvaluator(const Token &token) {
    auto p = std::get_if<SymbolToken>(&token);
    auto quote_p = std::get_if<QuoteToken>(&token);

    if (!p && !quote_p) {
        throw RuntimeError{"Non-symbol token cannot be evaluated"};
    }

    std::string symbol;
    if (p) {
        symbol = p->name;
    } else {
        symbol = "quote";
    }

    EvalCategory category = DefineCategory(symbol);

    switch (category) {
    case EvalCategory::Arithmetical:
        return std::make_unique<Arithmetical>(symbol);
    case EvalCategory::Predicator:
        return std::make_unique<Predicator>(
            symbol.substr(0, symbol.size() - 1));
    case EvalCategory::Comparator:
        return std::make_unique<Comparator>(symbol);
    case EvalCategory::ArrayFunctor:
        return std::make_unique<ArrayFunctor>(symbol);
    case EvalCategory::Functor:
        return std::make_unique<Functor>(symbol);
    case EvalCategory::Logical:
        return std::make_unique<Logical>(symbol);
    case EvalCategory::Quote:
        return std::make_unique<Quote>();
    case EvalCategory::None:
        return nullptr;
    }

    throw RuntimeError{"Unknown function"};
}

Arithmetical::Arithmetical(const std::string &symbol) {
    if (symbol == "+") {
        type_ = ArithmeticalOperations::Plus;
    } else if (symbol == "-") {
        type_ = ArithmeticalOperations::Minus;
    } else if (symbol == "*") {
        type_ = ArithmeticalOperations::Multiply;
    } else if (symbol == "/") {
        type_ = ArithmeticalOperations::Divide;
    } else {
        throw RuntimeError{"Wrong symbol for arithmetical operator"};
    }
}

Object::NodeType Arithmetical::Evaluate(Object::NodeType args) {
    int64_t result;
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    Object::NodeType number = nullptr;

    if (arguments.empty()) {
        switch (type_) {
        case ArithmeticalOperations::Minus:
        case ArithmeticalOperations::Divide:
            throw RuntimeError{"Invalid arguments for function"};
        default:
            break;
        }
    } else {
        number = arguments[0];
        if (!Is<Number>(number) && Is<Cell>(number)) {
            number = As<Cell>(number)->Call(nullptr);
        }
    }

    if (!number && args) {
        throw RuntimeError{"Invalid arguments for function"};
    }

    switch (type_) {
    case ArithmeticalOperations::Plus:
        result = 0;
        break;
    case ArithmeticalOperations::Multiply:
        result = 1;
        break;
    case ArithmeticalOperations::Minus:
    case ArithmeticalOperations::Divide:
        if (!Is<Number>(number)) {
            throw RuntimeError{"Unexpected token"};
        }

        result = As<Number>(number)->GetValue();
    }

    for (size_t i = 0; i != arguments.size(); ++i) {
        if (i == 0 && (type_ == ArithmeticalOperations::Minus ||
                       type_ == ArithmeticalOperations::Divide)) {
            continue;
        }

        if (!arguments[i]) {
            break;
        }

        number = arguments[i];
        if (!Is<Number>(number) && Is<Cell>(number)) {
            number = As<Cell>(number)->Call(nullptr);
        }

        if (!Is<Number>(number)) {
            throw RuntimeError{"Unexpected token"};
        }

        switch (type_) {
        case ArithmeticalOperations::Plus:
            result += As<Number>(number)->GetValue();
            break;
        case ArithmeticalOperations::Multiply:
            result *= As<Number>(number)->GetValue();
            break;
        case ArithmeticalOperations::Minus:
            result -= As<Number>(number)->GetValue();
            break;
        case ArithmeticalOperations::Divide:
            result /= As<Number>(number)->GetValue();
            break;
        }
    }

    return std::make_shared<Number>(ConstantToken{result});
}

Predicator::Predicator(const std::string &type) {
    if (type == "number") {
        type_ = PredicateTypes::Integer;
    } else if (type == "boolean") {
        type_ = PredicateTypes::Boolean;
    } else if (type == "pair") {
        type_ = PredicateTypes::Pair;
    } else if (type == "list") {
        type_ = PredicateTypes::List;
    } else if (type == "null") {
        type_ = PredicateTypes::Null;
    } else {
        throw RuntimeError{"Wrong symbol for arithmetical operator"};
    }
}

Object::NodeType Predicator::Evaluate(Object::NodeType args) {
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    if (type_ == PredicateTypes::Integer || type_ == PredicateTypes::Boolean) {
        bool result = false;

        if (Is<Number>(arguments[0])) {
            auto num = As<Number>(arguments[0]);

            if (num->IsBoolean()) {
                result = type_ == PredicateTypes::Boolean;
            } else {
                result = type_ == PredicateTypes::Integer;
            }
        }

        return std::make_shared<Number>(BooleanToken{result});
    } else if (type_ == PredicateTypes::Null) {
        return std::make_shared<Number>(
            BooleanToken{As<Cell>(args)->Call(nullptr) == nullptr});
    } else if (type_ == PredicateTypes::Pair) {
        if (!Is<Cell>(args)) {
            return std::make_shared<Number>(BooleanToken{false});
        }

        auto obj = As<Cell>(args)->Call(nullptr);

        if (!obj) {
            return std::make_shared<Number>(BooleanToken{false});
        }
        if (As<Cell>(obj)->GetFirst() && As<Cell>(obj)->GetSecond()) {
            return std::make_shared<Number>(BooleanToken{true});
        }

        return std::make_shared<Number>(BooleanToken{false});
    } else if (type_ == PredicateTypes::List) {
        auto obj = As<Cell>(args)->Call(nullptr);

        if (!obj) {
            return std::make_shared<Number>(BooleanToken{true});
        }

        std::vector<Object::NodeType> arguments;
        ToVector(obj, arguments);

        if (!arguments[arguments.size() - 1]) {
            return std::make_shared<Number>(BooleanToken{true});
        }

        return std::make_shared<Number>(BooleanToken{false});
    }
    throw RuntimeError{"Not implemented predicator"};
}

Comparator::Comparator(const std::string &type) {
    if (type == "=") {
        type_ = CompareType::EQ;
    } else if (type == "<=") {
        type_ = CompareType::LE;
    } else if (type == ">=") {
        type_ = CompareType::GE;
    } else if (type == "<") {
        type_ = CompareType::LS;
    } else if (type == ">") {
        type_ = CompareType::GR;
    } else {
        throw RuntimeError{"Wrong symbol for comparison"};
    }
}

Object::NodeType Comparator::Evaluate(Object::NodeType args) {
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    if (arguments.empty()) {
        return std::make_shared<Number>(BooleanToken{true});
    }

    bool result = true;

    for (size_t i = 1; i != arguments.size(); ++i) {
        if (!arguments[i]) {
            break;
        }

        switch (type_) {
        case CompareType::EQ:
            result &= (As<Number>(arguments[i - 1])->GetValue() ==
                       As<Number>(arguments[i])->GetValue());
            break;
        case CompareType::LE:
            result &= (As<Number>(arguments[i - 1])->GetValue() <=
                       As<Number>(arguments[i])->GetValue());
            break;
        case CompareType::GE:
            result &= (As<Number>(arguments[i - 1])->GetValue() >=
                       As<Number>(arguments[i])->GetValue());
            break;
        case CompareType::LS:
            result &= (As<Number>(arguments[i - 1])->GetValue() <
                       As<Number>(arguments[i])->GetValue());
            break;
        case CompareType::GR:
            result &= (As<Number>(arguments[i - 1])->GetValue() >
                       As<Number>(arguments[i])->GetValue());
            break;
        }

        if (!result) {
            break;
        }
    }

    return std::make_shared<Number>(BooleanToken{result});
}

ArrayFunctor::ArrayFunctor(const std::string &type) {
    if (type == "min") {
        type_ = ArrayFunction::Min;
    } else if (type == "max") {
        type_ = ArrayFunction::Max;
    } else if (type == "cons") {
        type_ = ArrayFunction::Cons;
    } else if (type == "car") {
        type_ = ArrayFunction::Car;
    } else if (type == "cdr") {
        type_ = ArrayFunction::Cdr;
    } else if (type == "list") {
        type_ = ArrayFunction::List;
    } else if (type == "list-ref") {
        type_ = ArrayFunction::List_Ref;
    } else if (type == "list-tail") {
        type_ = ArrayFunction::List_Tail;
    } else {
        throw RuntimeError{"Wrong symbol for array functor"};
    }
}

Object::NodeType ArrayFunctor::Evaluate(Object::NodeType args) {
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    if (type_ == ArrayFunction::Min || type_ == ArrayFunction::Max) {
        if (arguments.empty()) {
            throw RuntimeError{"Empty array passed"};
        }

        int64_t result = As<Number>(arguments[0])->GetValue();
        for (size_t i = 1; i != arguments.size(); ++i) {
            if (!arguments[i]) {
                break;
            }

            if (type_ == ArrayFunction::Min) {
                result = std::min(result, As<Number>(arguments[i])->GetValue());
            }
            if (type_ == ArrayFunction::Max) {
                result = std::max(result, As<Number>(arguments[i])->GetValue());
            }
        }

        return std::make_shared<Number>(ConstantToken{result});
    } else if (type_ == ArrayFunction::Car) {
        auto obj = As<Cell>(args)->Call(nullptr);

        if (!obj) {
            throw RuntimeError{"Index out of range"};
        }

        std::vector<Object::NodeType> arguments;
        ToVector(obj, arguments);

        return arguments[0];
    } else if (type_ == ArrayFunction::Cdr) {
        auto obj = As<Cell>(args)->Call(nullptr);

        if (!obj) {
            throw RuntimeError{"Index out of range"};
        }

        std::vector<Object::NodeType> arguments;
        ToVector(obj, arguments);

        return FromVector(1, arguments);
    } else if (type_ == ArrayFunction::Cons) {
        std::vector<Object::NodeType> arguments;
        ToVector(args, arguments);

        if (arguments.size() != 3) {
            throw RuntimeError{"Wrong pair size"};
        }
        arguments.pop_back();

        return FromVector(0, arguments);
    } else if (type_ == ArrayFunction::List) {
        std::vector<Object::NodeType> arguments;
        if (!args) {
            arguments.push_back(nullptr);
        } else {
            ToVector(args, arguments);
        }

        return FromVector(0, arguments);
    } else if (type_ == ArrayFunction::List_Tail ||
               type_ == ArrayFunction::List_Ref) {
        std::vector<Object::NodeType> arguments;
        ToVector(args, arguments);

        std::vector<Object::NodeType> array;

        if (!Is<Cell>(arguments[0])) {
            throw RuntimeError{"No array in List-Tail, List-Ref"};
        }

        ToVector(As<Cell>(arguments[0])->Call(nullptr), array);
        auto pos = As<Number>(arguments[1]);

        if (!pos) {
            throw RuntimeError{"No position in List-Tail, List-Ref"};
        }

        if (type_ == ArrayFunction::List_Ref) {
            if (array.size() <= static_cast<size_t>(pos->GetValue()) + 1) {
                throw RuntimeError{"Index out of range"};
            }

            return array[pos->GetValue()];
        } else {
            if (array.size() <= static_cast<size_t>(pos->GetValue())) {
                throw RuntimeError{"Index out of range"};
            }

            return FromVector(pos->GetValue(), array);
        }
    }

    throw RuntimeError{"Not implemented"};
}

Functor::Functor(const std::string &type) {
    if (type == "abs") {
        type_ = Function::Abs;
    } else {
        throw RuntimeError{"Wrong symbol for functor"};
    }
}

Object::NodeType Functor::Evaluate(Object::NodeType args) {
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    if (arguments.empty() || arguments.size() > 2 ||
        !Is<Number>(arguments[0])) {
        throw RuntimeError{"Wrong arguments amount for abs"};
    }

    return std::make_shared<Number>(
        ConstantToken{std::abs(As<Number>(arguments[0])->GetValue())});
}

Logical::Logical(const std::string &type) {
    if (type == "and") {
        type_ = LogicalOperation::And;
    } else if (type == "or") {
        type_ = LogicalOperation::Or;
    } else if (type == "not") {
        type_ = LogicalOperation::Not;
    } else {
        throw RuntimeError{"Wrong symbol for logical operator"};
    }
}

Object::NodeType Logical::Evaluate(Object::NodeType args) {
    std::vector<Object::NodeType> arguments;
    ToVector(args, arguments);

    if (type_ == LogicalOperation::Not) {
        if (!args || arguments.empty() || arguments.size() > 2) {
            throw RuntimeError{"Wrong arguments amount for not"};
        }

        Object::NodeType obj = nullptr;

        if (Is<Cell>(arguments[0])) {
            obj = As<Cell>(arguments[0])->Call(nullptr);
        } else {
            obj = arguments[0];
        }

        if (Is<Number>(obj) && As<Number>(obj)->IsBoolean()) {
            return std::make_shared<Number>(
                BooleanToken{!As<Number>(obj)->GetBooleanValue()});
        }

        return std::make_shared<Number>(BooleanToken{false});
    }

    if (!args) {
        return std::make_shared<Number>(
            BooleanToken{type_ == LogicalOperation::And});
    }

    bool result;

    if (type_ == LogicalOperation::And) {
        result = true;
    } else if (type_ == LogicalOperation::Or) {
        result = false;
    }

    for (size_t i = 0; i != arguments.size(); ++i) {
        bool current_value;
        Object::NodeType element = nullptr;

        if (Is<Cell>(arguments[i])) {
            element = As<Cell>(arguments[i])->Call(nullptr);
        } else {
            element = arguments[i];
        }

        // arguments.clear();

        if (element && Is<Number>(element) &&
            As<Number>(element)->IsBoolean()) {
            current_value = As<Number>(element)->GetBooleanValue();
        } else {
            continue;
        }

        switch (type_) {
        case LogicalOperation::And:
            result &= current_value;
            break;
        case LogicalOperation::Or:
            result |= current_value;
            break;
        case LogicalOperation::Not:
            throw RuntimeError{"Unsupported operation"};
        }

        if (!result && (type_ == LogicalOperation::And)) {
            return std::make_shared<Number>(BooleanToken{result});
        }
        if (result && (type_ == LogicalOperation::Or)) {
            return std::make_shared<Number>(BooleanToken{result});
        }
    }

    if ((result && (type_ == LogicalOperation::And)) ||
        (!result && (type_ == LogicalOperation::Or))) {
        if (Is<Cell>(arguments[arguments.size() - 2])) {
            return As<Cell>(arguments[arguments.size() - 2])->Call(nullptr);
        }

        return arguments[arguments.size() - 2];
    }

    return std::make_shared<Number>(BooleanToken{result});
}

Object::NodeType Quote::Evaluate(Object::NodeType args) { return args; }