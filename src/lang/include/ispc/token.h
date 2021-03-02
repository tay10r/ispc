#pragma once

#include <ispc/source_range.h>

#include <cstdint>

namespace ispc {

/** @brief Enumerates the several categories of tokens produced by the lexer. */
enum class TokenKind {
    /** Used for when the scanner is called at the end of the file or then the
     * token is not initialized. */
    EndOfFile,
    Identifier,
    Int8Constant,
    Int16Constant,
    Int32Constant,
    Int64Constant,
    UInt8Constant,
    UInt16Constant,
    UInt32Constant,
    UInt64Constant,
    Newline,
    Whitespace
};

/** @brief A union of all types allowed of token data.

    @tparam Char The character type of the input source.
 */
template <typename Char> union TokenData {
    const Char *asString;
    float asFloat;
    double asDouble;
    std::uint64_t asInt;

    static TokenData MakeDouble(double value) noexcept {
        TokenData out;
        out.asDouble = value;
        return out;
    }

    static TokenData MakeFloat(float value) noexcept {
        TokenData out;
        out.asFloat = value;
        return out;
    }

    static TokenData MakeString(const Char *str) noexcept {
        TokenData out;
        out.asString = str;
        return out;
    }

    static TokenData MakeInt(std::uint64_t value) noexcept {
        TokenData out;
        out.asInt = value;
        return out;
    }

    static TokenData Make() noexcept {
        TokenData out;
        out.asInt = 0;
        return out;
    }
};

/** @brief Represents a single unit of output from the lexer.

    @tparam Char The character type of the input source.
 */
template <typename Char> class Token final {
  public:
    constexpr Token() noexcept { this->data.asInt = 0; }

    constexpr const TokenData<Char> &GetData() const noexcept { return this->data; }

    constexpr TokenKind GetKind() const noexcept { return this->kind; }

    constexpr const SourceRange &GetSourceRange() const noexcept { return this->sourceRange; }

    constexpr bool operator==(TokenKind k) const noexcept { return kind == k; }
    constexpr bool operator!=(TokenKind k) const noexcept { return kind != k; }

  private:
    template <typename C, typename R, typename... OtherRs> friend class Lexer;

    /** The range from which this token was found. */
    SourceRange sourceRange;

    TokenKind kind = TokenKind::EndOfFile;

    TokenData<Char> data;
};

} // namespace ispc
