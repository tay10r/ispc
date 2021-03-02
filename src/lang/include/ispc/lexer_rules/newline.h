#pragma once

#include <ispc/lexer_rule.h>

namespace ispc {

/** @brief A lexer rule for newlines.

    @note For compatibility purposes, only line feed characters are treated as
    newlines. Carriage returns or CR+LF pairs are not treated as newline tokens.
 * */
class NewlineLexerRule final : public LexerRule<NewlineLexerRule> {
  public:
    template <typename Char, typename Context>
    LexerRuleResult Lex(const LexerRuleProxy<Char> &ruleProxy, const Context &) const {
        if (ruleProxy.Peek(0) == '\n')
            return {TokenKind::Newline, 1};

        if (ruleProxy.GetLexerOptions().checkCRLF) {
            if ((ruleProxy.Peek(0) == '\r') && (ruleProxy.Peek(1) == '\n'))
                return {TokenKind::Newline, 2};
        }

        return {};
    }

    template <typename Char, typename Context>
    TokenData<Char> ExecuteAction(const LexerRuleProxy<Char> &, const RuleResult &, const Context &) {
        return TokenData<Char>::Make();
    }
};

} // namespace ispc
