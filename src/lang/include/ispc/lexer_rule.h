#pragma once

#include <ispc/lexer_rule_proxy.h>
#include <ispc/token.h>

namespace ispc {

/** @brief This structure describes the result of a lexer rule. It indicates the
    number of characters that the rule matched and the kind of token that the
    rule produced.
 */
struct LexerRuleResult final {

    TokenKind tokenKind = TokenKind::EndOfFile;

    /** The number of characters that matched the rule. */
    std::size_t matchLength = 0;

    std::vector<LexicalError> errors;

    LexerRuleResult() = default;

    LexerRuleResult(TokenKind k, std::size_t len) : tokenKind(k), matchLength(len) {}

    bool operator<(const LexerRuleResult &other) const noexcept { return this->matchLength < other.matchLength; }
};

/** @brief Used to describe how to lex a specific kind of token.

    @detail This is the base class of a lexer rule. Newly added rules must
    derive this class using static polymorphism. Two methods are required to be
    implemented. Both methods are passed a proxy to the lexer and a context
    object. The lexer proxy is used to examine the character content of the
    source code. The context object is used for context-sensitive rules. If a
    rule is context sensitive, then the caller must pass either the context data
    or a context tag to the @ref Lexer::Lex function. The

    The first is the method to match characters from the lexer input. This
    method should have the following signature:

    @code{.cpp}
      template <typename Char, typename Context>
      LexerRuleResult DerivedRule::Lex(LexerRuleProxy<Char> &ruleProxy, const Context&);
    @endcode

    The second required method is the action of the rule. It is not guaranteed
    that, if the rule matches a pattern in the first function, the action for
    that rule will be called. The action of the rule is only called if it is
    selected by the lexer.

    The signature of the action method should be this:

    @code{.cpp}
      template <typename Char, typename Context>
      TokenData<Char> DerivedRule::ExecuteAction(LexerRuleProxy<Char> &ruleProxy, const Context&);
    @endcode

    Like the first method, the string is always null terminated.
    The result instance is the same one returned by the first function.
 */
template <typename Derived> class LexerRule {
  public:
    using RuleResult = LexerRuleResult;

    template <typename Char, typename Context>
    RuleResult operator()(const LexerRuleProxy<Char> &ruleProxy, const Context &context) {
        return static_cast<Derived *>(this)->Lex(ruleProxy, context);
    }

    template <typename Char, typename Context>
    TokenData<Char> operator()(const LexerRuleProxy<Char> &ruleProxy, const LexerRuleResult &ruleResult,
                               const Context &context) {
        return static_cast<Derived *>(this)->ExecuteAction(ruleProxy, ruleResult, context);
    }
};

template <typename Char> bool InRange(Char c, Char lo, Char hi) noexcept { return (c >= lo) && (c <= hi); }

template <typename Char> bool IsAlpha(Char c) noexcept { return InRange(c, 'a', 'z') || InRange(c, 'A', 'Z'); }

template <typename Char> bool IsNonDigit(Char c) noexcept { return IsAlpha(c) || (c == Char('_')); }

template <typename Char> bool IsDigit(Char c) noexcept { return InRange(c, '0', '9'); }

template <typename Char> Char ToLower(Char c) noexcept {
    if ((c >= 'A') && (c <= 'Z'))
        return c + 32;
    else
        return c;
}

} // namespace ispc
