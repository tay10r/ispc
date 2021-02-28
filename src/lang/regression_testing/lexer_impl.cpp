#include "lexer_impl.h"

#include "lexer_impl_flex.h"
#include "lexer_impl_nonflex.h"

std::unique_ptr<LexerImpl>
LexerImpl::MakeNonFlexLexer() {
    return std::unique_ptr<LexerImpl>(::MakeNonFlexLexer());
}

std::unique_ptr<LexerImpl>
LexerImpl::MakeFlexLexer() {
    return std::unique_ptr<LexerImpl>(::MakeFlexLexer());
}
