#pragma once

#include "tokenizer.h"

#include <string>

class Interpreter {
  public:
    std::string Run(const std::string &query);

  private:
    Tokenizer tokenizer_;
};
