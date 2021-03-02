#pragma once

#include <ispc/lexical_error.h>

#include <string>
#include <vector>

namespace ispc {

/** @brief This class is how the rules interface with the lexer.

    @detail Lexer rules are able to examine the source code that the lexer is
    currently at as well as report errors detected by the rule. Lexer rules
    should only issue errors if they have some certainty that the rule will be
    selected. For example, identifier rules should not issue errors if the first
    character is not an alphabetical or underscore. On the other hand, an error
    should be issued if an illegal character is found in a numeric sequence or
    a string is not terminated.
 */
template <typename Char> class LexerRuleProxy final {
  public:
    /** @brief Creates a copy of the text content in the input buffer.

        @detail This function can be used when a copy of the character content
        is required by a token or by a rule action.

        @param n The number of characters to copy from the input buffer.

        @return A copy of the input buffer, up to @p n characters.
     */
    std::basic_string<Char> CopyString(std::size_t n) const;

    /** @brief Gets a character at a certain offset from the lexer's location.

        @param offset The offset from the lexer's current location to get the
        character at. For example, if the lexer is at index 32 and the value of
        @p offset is 3, then the character at index 35 of the source file is
        returned by this function.

        @return The character at the given offset. If the value of @p offset
        ends up being out of bounds, then the value of zero is returned instead.
     */
    Char Peek(std::size_t offset) const noexcept;

    bool IsOutOfBounds(std::size_t offset) const noexcept { return (sourceIdx + offset) >= sourceLen; }

    const LexerOptions &GetLexerOptions() const noexcept { return options; }

  private:
    template <typename, typename, typename...> friend class Lexer;

    LexerRuleProxy(const Char *s, std::size_t len, std::size_t idx, const LexerOptions &o) noexcept
        : source(s), sourceLen(len), sourceIdx(idx), options(o) {}

    const Char *source;
    std::size_t sourceLen;
    std::size_t sourceIdx;
    const LexerOptions &options;
};

/* Implementation below */

template <typename Char> std::basic_string<Char> LexerRuleProxy<Char>::CopyString(std::size_t n) const {
    return std::basic_string<Char>(source + sourceIdx, n);
}

template <typename Char> Char LexerRuleProxy<Char>::Peek(std::size_t offset) const noexcept {
    auto absIndex = sourceIdx + offset;
    if (absIndex >= sourceLen)
        return 0;
    else
        return source[absIndex];
}

} // namespace ispc
