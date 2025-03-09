#include "utils/scheme.h"
#include "utils/base_object.h"
#include "utils/error.h"
#include "utils/object.h"
#include "utils/parser.h"
#include "utils/tokenizer.h"

#include <cassert>
#include <iostream>
#include <sstream>

// #define DEBUG

std::string Interpreter::Run(const std::string &query) {
    std::stringstream ss{query};
    tokenizer_.Update(&ss);

    auto ast = Read(&tokenizer_);

#ifdef DEBUG
    std::cout << ast << std::endl;
    std::cout << "----------------" << std::endl;
#endif

    if (!ast) {
        throw RuntimeError{"nullptr cannot be called"};
    }

    // std::vector<Object::NodeType>
    Object::NodeType args;
    if (!Is<Number>(ast)) {

        ast = ast->Call(args);
    }

    std::stringstream output;

    if (Is<Cell>(ast)) {
        output << "(";
    }

    output << ast;

    if (Is<Cell>(ast)) {
        output << ")";
    }

    return output.str();
}
