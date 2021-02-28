#pragma once

#include <ispc/lexer_rule.h>

#include <set>
#include <string>

namespace ispc {

/** @brief A lexer rule for identifiers and type names.

    @detail This rule recognizes identifiers. Since identifiers may also be type
    names, this rule also tracks which identifiers are type names. It also keeps
    a set of identifiers so that memory is not allocated for identifiers with
    the same character content.
 */
template <typename Char> class IdentifierLexerRule final : public LexerRule<IdentifierLexerRule<Char>> {
  public:
    using RuleResult = LexerRuleResult;

    RuleResult Lex(const Char *source, std::size_t length) const {

        if (!length || !IsNonDigit(source[0]))
            return {};

        std::size_t match = 1;

        while (match < length) {
            auto c = source[match];
            if (IsDigit(c) || IsNonDigit(c))
                match++;
            else
                break;
        }

        return {TokenKind::Identifier, match};
    }

    TokenData<Char> ExecuteAction(const Char *source, const RuleResult &result) {

        string_type str(source, result.matchLength);

        auto it = knownIdentifiers.emplace(std::move(str)).first;

        return TokenData<Char>::MakeString(it->c_str());
    }

  private:
    using string_type = std::basic_string<Char>;

    /** Repeated identifiers are stored here. */
    std::set<string_type> knownIdentifiers;
};

} // namespace ispc
