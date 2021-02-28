#include "lexer_impl_nonflex.h"

#include <ispc/lexer.h>

#include "lexer_impl.h"

#include <ostream>
#include <vector>

namespace {

class NonFlexLexer final : public LexerImpl {
    ispc::ISPCLexer<char> lexer;
    std::vector<ispc::Token<char>> tokens;

  public:
    void ScanTokens() override {
        while (!this->lexer.AtEnd()) {
            auto result = this->lexer.Lex();
            if (result.token != ispc::TokenKind::EndOfFile)
                tokens.emplace_back(result.token);
            else
                break;
        }
    }

    void SetInput(const std::string &in) override { this->lexer.SetInput(in); }

    void PrintTokens(std::ostream &stream) const override {
        for (const auto &tok : tokens)
            PrintToken(tok, stream);
    };

  protected:
    static const char *ToString(ispc::TokenKind k) noexcept {
        switch (k) {
        case ispc::TokenKind::EndOfFile:
            return "eof";
        case ispc::TokenKind::Newline:
            return "newline";
        case ispc::TokenKind::Identifier:
            return "identifier";
        case ispc::TokenKind::UInt8Constant:
            return "uint8_constant";
        case ispc::TokenKind::UInt16Constant:
            return "uint16_constant";
        case ispc::TokenKind::UInt32Constant:
            return "uint32_constant";
        case ispc::TokenKind::UInt64Constant:
            return "uint64_constant";
        case ispc::TokenKind::Int8Constant:
            return "int8_constant";
        case ispc::TokenKind::Int16Constant:
            return "int16_constant";
        case ispc::TokenKind::Int32Constant:
            return "int32_constant";
        case ispc::TokenKind::Int64Constant:
            return "int64_constant";
        }

        return "<unknown>";
    }

    static void PrintToken(const ispc::Token<char> &tok, std::ostream &stream) {

        stream << ToString(tok.GetKind());

        stream << " '";

        switch (tok.GetKind()) {
        case ispc::TokenKind::EndOfFile:
            break;
        case ispc::TokenKind::Newline:
            stream << "\\n";
            break;
        case ispc::TokenKind::Int8Constant:
        case ispc::TokenKind::Int16Constant:
        case ispc::TokenKind::Int32Constant:
        case ispc::TokenKind::Int64Constant:
        case ispc::TokenKind::UInt8Constant:
        case ispc::TokenKind::UInt16Constant:
        case ispc::TokenKind::UInt32Constant:
        case ispc::TokenKind::UInt64Constant:
            stream << tok.GetData().asInt;
            break;
        case ispc::TokenKind::Identifier:
            stream << tok.GetData().asString;
            break;
        }

        stream << "' ";

        auto sourceRange = tok.GetSourceRange();

        stream << "from ";
        stream << sourceRange.GetFirstLine() << ':' << sourceRange.GetFirstColumn();
        stream << " to ";
        stream << sourceRange.GetLastLine() << ':' << sourceRange.GetLastColumn();
        stream << std::endl;
    }
};

} // namespace

LexerImpl *MakeNonFlexLexer() { return new NonFlexLexer(); }
