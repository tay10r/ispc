#pragma once

#include <ispc/token.h>

#include <ispc/lexer_rules/identifier.h>
#include <ispc/lexer_rules/integer.h>
#include <ispc/lexer_rules/newline.h>

#include <array>

namespace ispc {

template <typename Char, typename FirstRule, typename... OtherRules> class Lexer;

/** @brief A type definition for a lexer containing the rules for ISPC. */
template <typename Char = char>
using ISPCLexer = Lexer<Char, IdentifierLexerRule<Char>, IntegerLexerRule, NewlineLexerRule>;

/** @brief The result of a lexer call.

    @detail This structure stores the token that was produced along with any
    errors that were found while scanning it.
 */
template <typename Char> struct LexResult final {

    Token<Char> token;

    LexicalError error = LexicalError::None;

    constexpr bool HasError() const noexcept { return error != LexicalError::None; }
};

template <typename Char> bool AtEnd(const ISPCLexer<Char> &lexer) { return lexer.AtEnd(); }

template <typename Char> LexResult<Char> Lex(ISPCLexer<Char> &lexer);

template <typename Char> void SetInput(ISPCLexer<Char> &lexer, std::basic_string<Char> input) {
    lexer.SetInput(std::move(input));
}

/* Implementation details are below. */

/** @brief A generic lexer.

    @detail The lexer is made up of arbitrary rules. While the rules are generic
    template parameters, they ideally should be derived from @ref LexerRule for
    readability purposes.

    When a call to the lexer is made, it will run each rule in the order of the
    template parameters and determine the number of characters each rule
    matches. Once all the rules have been ran, the lexer will choose which one
    is to be returned to the caller. This is done in one of two ways. By default
    the lexer will choose the rule which matched the greatest number of
    characters. If more than one rule matched the same number of characters, the
    rule appearing first in the template parameter list gets chosen.
    Alternatively the caller may override this behavior by implementing a rule
    selector and passing it to @ref Lexer::Lex as a parameter. The rule selector
    must be callable and accept an array of @ref LexerRuleResult as a parameter.
    The rule selector must also return the index of the rule it selects. For
    example:

    @code{.cpp}
      struct RuleSelector final {
        template <std::size_t ruleCount>
        std::size_t operator () (const std::array<LexerRuleResult, ruleCount> &ruleResults) const;
      };
    @endcode

    A rule selector may also use existing typedefs for readability.

    @code{.cpp}
      struct RuleSelector final {
        std::size_t operator () (const ISPCLexer<char>::RuleResultArray &ruleResults) const;
      };
    @endcode
 */
template <typename Char, typename FirstRule, typename... OtherRules> class Lexer {

  public:
    static constexpr std::size_t GetRuleCount() noexcept { return sizeof...(OtherRules) + 1; }

    using RuleResult = LexerRuleResult;

    using RuleResultArray = std::array<RuleResult, sizeof...(OtherRules) + 1>;

    bool AtEnd() const noexcept { return (this->index >= this->source.size()) || this->allRulesFailed; }

    LexResult<Char> Lex() {
        DefaultRuleSelector defSelector{};
        return this->Lex(defSelector);
    }

    template <typename RuleSelector> LexResult<Char> Lex(const RuleSelector &ruleSelector) {

        if (!filterTokens)
            return LexUnfiltered(ruleSelector);

        while (!AtEnd()) {
            auto result = LexUnfiltered(ruleSelector);
            if (result.HasError())
                return result;

            switch (result.token.GetKind()) {
            case TokenKind::Newline:
                continue;
            case TokenKind::Identifier:
            case TokenKind::Int8Constant:
            case TokenKind::Int16Constant:
            case TokenKind::Int32Constant:
            case TokenKind::Int64Constant:
            case TokenKind::UInt8Constant:
            case TokenKind::UInt16Constant:
            case TokenKind::UInt32Constant:
            case TokenKind::UInt64Constant:
            case TokenKind::EndOfFile:
                return result;
            }
        }

        return {};
    }

    void SetInput(std::basic_string<Char> s) {
        this->source = std::move(s);
        this->index = 0;
        this->line = 1;
        this->column = 1;
    }

  private:
    template <typename RuleSelector> LexResult<Char> LexUnfiltered(const RuleSelector &ruleSelector) {

        const char *ruleInputText = source.data() + index;

        std::size_t ruleInputLength = source.size() - index;

        RuleResultArray ruleResults;

        this->grammar.Lex(ruleResults.data(), ruleInputText, ruleInputLength);

        if (None(ruleResults)) {
            this->allRulesFailed = true;
            return {};
        }

        std::size_t ruleIndex = ruleSelector(ruleResults);

        if (ruleIndex >= this->GetRuleCount()) {
            this->allRulesFailed = true;
            return {};
        }

        return this->Produce(ruleIndex, ruleResults[ruleIndex]);
    }

    static bool None(const RuleResultArray &ruleResults) {
        for (std::size_t i = 0; i < GetRuleCount(); i++) {
            if (ruleResults[i].matchLength > 0)
                return false;
        }
        return true;
    }

    void Advance(std::size_t charCount) {

        for (std::size_t i = 0; i < charCount; i++) {

            auto c = this->source[this->index + i];

            if (c == '\n') {
                this->line++;
                this->column = 1;
            } else if ((c & 0xc0) != 0x80) {
                this->column++;
            }
        }

        this->index += charCount;
    }

    LexResult<Char> Produce(std::size_t ruleIndex, const RuleResult &ruleResult) {

        Token<Char> token;

        token.data = grammar.ExecuteRuleAction(ruleIndex, source.data() + index, ruleResult);

        token.kind = ruleResult.tokenKind;

        token.sourceRange.first_line = int(this->line);
        token.sourceRange.first_column = int(this->column);

        Advance(ruleResult.matchLength);

        token.sourceRange.last_line = int(this->line);
        token.sourceRange.last_column = int(this->column);

        return {token, ruleResult.error};
    }

    struct DefaultRuleSelector final {

        std::size_t operator()(const RuleResultArray &ruleResults) const {

            std::size_t ruleIndex = 0;

            std::size_t ruleLength = 0;

            for (std::size_t i = 0; i < GetRuleCount(); i++) {
                if (ruleResults[i].matchLength > ruleLength) {
                    ruleIndex = i;
                    ruleLength = ruleResults[i].matchLength;
                }
            }

            return ruleIndex;
        }
    };

    template <typename R, typename... OtherR> struct Grammar final {

        R rule;

        Grammar<OtherR...> grammar;

        void Lex(RuleResult *result, const Char *source, std::size_t length) {

            result[0] = rule(source, length);

            grammar.Lex(result + 1, source, length);
        }

        TokenData<Char> ExecuteRuleAction(std::size_t index, const Char *s, const RuleResult &result) {
            if (index == 0)
                return rule(s, result);
            else
                return grammar.ExecuteRuleAction(index - 1, s, result);
        }
    };

    template <typename LastR> struct Grammar<LastR> final {
        LastR lastRule;

        void Lex(RuleResult *result, const Char *source, std::size_t length) { result[0] = lastRule(source, length); }

        TokenData<Char> ExecuteRuleAction(std::size_t index, const Char *s, const RuleResult &result) {
            if (index == 0)
                return lastRule(s, result);
            else
                return TokenData<Char>::Make();
        }
    };

    /** If true, all comments, whitespace, and newlines are skipped. */
    bool filterTokens = true;

    bool allRulesFailed = false;

    Grammar<FirstRule, OtherRules...> grammar;

    std::size_t line = 1;

    std::size_t column = 1;

    std::basic_string<Char> source;

    std::size_t index = 0;
};

} // namespace ispc
