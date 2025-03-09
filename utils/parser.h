#pragma once

#include "base_object.h"
#include "object.h"
#include "tokenizer.h"

#include <cstddef>
#include <memory>

Object::NodeType ConvertNull(Object::NodeType obj);

std::shared_ptr<Object> ReadToken(Tokenizer *tokenizer);

std::shared_ptr<Object> Read(Tokenizer *tokenizer, bool first = true);

std::shared_ptr<Object> ReadList(Tokenizer *tokenizer);