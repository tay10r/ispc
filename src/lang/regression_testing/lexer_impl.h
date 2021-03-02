#pragma once

#include <memory>
#include <string>

#include <cstddef>

/// This class is used as an abstraction of the lexer implementation.
class LexerImpl {
  public:
    static std::unique_ptr<LexerImpl> MakeFlexLexer(std::size_t laneBits);
    static std::unique_ptr<LexerImpl> MakeNonFlexLexer(std::size_t laneBits);
    virtual ~LexerImpl() = default;
    virtual void PrintTokens(std::ostream &) const = 0;
    virtual void ScanTokens() = 0;
    virtual void SetInput(const std::string &) = 0;
};
