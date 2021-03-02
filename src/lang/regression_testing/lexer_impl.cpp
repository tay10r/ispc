#include "lexer_impl.h"

#include "lexer_impl_flex.h"
#include "lexer_impl_nonflex.h"

std::unique_ptr<LexerImpl> LexerImpl::MakeNonFlexLexer(std::size_t laneBits) {
    return std::unique_ptr<LexerImpl>(::MakeNonFlexLexer(laneBits));
}

std::unique_ptr<LexerImpl> LexerImpl::MakeFlexLexer(std::size_t laneBits) {
    return std::unique_ptr<LexerImpl>(::MakeFlexLexer(laneBits));
}
