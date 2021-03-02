#pragma once

#include <ispc/lexer_rule.h>

#include <ispc/impl/number_base.h>

#include <limits>

namespace ispc {

class IntegerLexerRule final : public LexerRule<IntegerLexerRule> {
  public:
    template <typename Char, typename Context>
    LexerRuleResult Lex(const LexerRuleProxy<Char> &ruleProxy, const Context &) const {
        if (NumberBase<16>::HasPrefix(ruleProxy))
            return Lex(ruleProxy, NumberBase<16>());
        else if (NumberBase<2>::HasPrefix(ruleProxy))
            return Lex(ruleProxy, NumberBase<2>());
        else
            return Lex(ruleProxy, NumberBase<10>());
    }

    template <typename Char, typename Context>
    TokenData<Char> ExecuteAction(const LexerRuleProxy<Char> &ruleProxy, const RuleResult &result, const Context &) {
        if (NumberBase<16>::HasPrefix(ruleProxy))
            return TokenData<Char>::MakeInt(ToInt(ruleProxy, result.matchLength, NumberBase<16>()).second);
        else if (NumberBase<2>::HasPrefix(ruleProxy))
            return TokenData<Char>::MakeInt(ToInt(ruleProxy, result.matchLength, NumberBase<2>()).second);
        else
            return TokenData<Char>::MakeInt(ToInt(ruleProxy, result.matchLength, NumberBase<10>()).second);
    }

  protected:
    struct LegacySuffix final {
        std::size_t u = 0;
        std::size_t l = 0;
        bool k = false;
        bool m = false;
        bool g = false;
    };

    template <typename Char>
    static std::size_t LexSuffix(const LexerRuleProxy<Char> &ruleProxy, std::size_t charIndex,
                                 LegacySuffix &suffix) noexcept {

        while (!ruleProxy.IsOutOfBounds(charIndex)) {

            auto c = ruleProxy.Peek(charIndex);

            if (ToLower(c) == 'u')
                suffix.u++;
            else if (ToLower(c) == 'l')
                suffix.l++;
            else if (c == 'k')
                suffix.k = true;
            else if (c == 'M')
                suffix.m = true;
            else if (c == 'G')
                suffix.g = true;
            else
                break;

            charIndex++;
        }

        return charIndex;
    }

    static TokenKind GetTokenKindFromValue(std::uint64_t value) noexcept {
        if (value <= 0x7fffffff)
            return TokenKind::Int32Constant;
        else if (value <= 0xffffffff)
            return TokenKind::UInt32Constant;
        else if (value <= 0x7fffffffffffffff)
            return TokenKind::Int64Constant;
        else
            return TokenKind::UInt64Constant;
    }

    template <typename IntType> static bool CheckedMulAssign(IntType &dst, IntType operand) noexcept {
        IntType tmp = dst * operand;
        bool overflowed = tmp < dst;
        dst = tmp;
        return overflowed;
    }

    template <typename IntType> static bool CheckedAddAssign(IntType &dst, IntType operand) noexcept {
        IntType tmp = dst + operand;
        bool overflowed = tmp < dst;
        dst = tmp;
        return overflowed;
    }

    template <typename Char, int base>
    std::pair<bool, std::uint64_t> ToInt(const LexerRuleProxy<Char> &ruleProxy, std::size_t tokLength,
                                         NumberBase<base> numberBase) const {

        std::uint64_t value = 0;

        auto overflowed = false;

        std::size_t charIndex = numberBase.GetPrefixSize();

        while (charIndex < tokLength) {

            auto c = ruleProxy.Peek(charIndex);

            if (!numberBase.InRange(c))
                break;

            std::uint64_t v = numberBase.ToValue(c);

            overflowed |= CheckedMulAssign(value, static_cast<std::uint64_t>(base));
            overflowed |= CheckedAddAssign(value, v);

            charIndex++;
        }

        bool kibi = false;
        bool mebi = false;
        bool gibi = false;

        while (charIndex < tokLength) {

            auto c = ruleProxy.Peek(charIndex);
            if (c == 'k')
                kibi = true;
            else if (c == 'M')
                mebi = true;
            else if (c == 'G')
                gibi = true;

            charIndex++;
        }

        using u64 = std::uint64_t;

        if (kibi)
            overflowed |= CheckedMulAssign(value, u64(1024));

        if (mebi)
            overflowed |= CheckedMulAssign(value, u64(1024) * u64(1024ULL));

        if (gibi)
            overflowed |= CheckedMulAssign(value, u64(1024) * u64(1024) * u64(1024));

        if (overflowed)
            return {true, std::numeric_limits<std::uint64_t>::max()};

        // TODO : handle 'u' and 'l' suffixes

        return {false, value};
    }

    template <typename Char, int base>
    LexerRuleResult Lex(const LexerRuleProxy<Char> &ruleProxy, NumberBase<base> numberBase) const {

        std::size_t matchLength = numberBase.GetPrefixSize();

        while (!ruleProxy.IsOutOfBounds(matchLength)) {
            if (numberBase.InRange(ruleProxy.Peek(matchLength)))
                matchLength++;
            else
                break;
        }

        if (!matchLength)
            return {};

        if (ruleProxy.GetLexerOptions().strictIntegerSuffixes) {

        } else {

            LegacySuffix legacySuffix;

            matchLength = LexSuffix(ruleProxy, matchLength, legacySuffix);
        }

        LexerRuleResult ruleResult;

        ruleResult.matchLength = matchLength;

        auto result = ToInt(ruleProxy, matchLength, numberBase);

        auto overflowed = result.first;

        if (overflowed && ruleProxy.GetLexerOptions().warnIntegerOverflow)
            ruleResult.errors.emplace_back(LexicalError{DiagnosticID::IntegerOverflow});

        ruleResult.tokenKind = GetTokenKindFromValue(result.second);

        return ruleResult;
    }
};

} // namespace ispc
