#pragma once

#include <cstddef>

class LexerImpl;

LexerImpl *MakeNonFlexLexer(std::size_t laneBits);
