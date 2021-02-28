#pragma once

#include <ispc/lexical_error.h>
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

    LexicalError error = LexicalError::None;

    constexpr bool operator<(const LexerRuleResult &other) const noexcept {
        return this->matchLength < other.matchLength;
    }

    constexpr bool HasError() const noexcept { return error != LexicalError::None; }
};

/** @brief Used to describe how to lex a specific kind of token.

    @detail This is the base class of a lexer rule. Newly added rules must
    derive this class using static polymorphism. Two methods are required to be
    implemented.

    The first is the method to match characters from the lexer input. This
    method should have the following signature:

    @code{.cpp}
      template <typename Char>
      LexerRuleResult DerivedRule::Lex(const Char* source, std::size_t length);
    @endcode

    The string passed to this function is null terminated.

    The second required method is the action of the rule. It is not guaranteed
    that, if the rule matches a pattern in the first function, the action for
    that rule will be called. The action of the rule is only called if it is
    selected by the lexer.

    The signature of the action method should be this:

    @code{.cpp}
      template <typename Char>
      TokenData<Char> DerivedRule::ExecuteAction(const Char* source, const LexerRuleResult& result);
    @endcode

    Like the first method, the string is always null terminated.
    The result instance is the same one returned by the first function.
 */
template <typename Derived> class LexerRule {
  public:
    using RuleResult = LexerRuleResult;

    template <typename Char> RuleResult operator()(const Char *source, std::size_t length) {
        return static_cast<Derived *>(this)->Lex(source, length);
    }

    template <typename Char> TokenData<Char> operator()(const Char *source, const RuleResult &result) {
        return static_cast<Derived *>(this)->ExecuteAction(source, result);
    }
};

template <typename Char> static bool InRange(Char c, Char lo, Char hi) noexcept { return (c >= lo) && (c <= hi); }

template <typename Char> static bool IsAlpha(Char c) noexcept { return InRange(c, 'a', 'z') || InRange(c, 'A', 'Z'); }

template <typename Char> static bool IsNonDigit(Char c) noexcept { return IsAlpha(c) || (c == Char('_')); }

template <typename Char> static bool IsDigit(Char c) noexcept { return InRange(c, '0', '9'); }

} // namespace ispc
