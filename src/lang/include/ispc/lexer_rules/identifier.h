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

    template <typename Context> RuleResult Lex(const LexerRuleProxy<Char> &ruleProxy, const Context &) const {

        if (!IsNonDigit(ruleProxy.Peek(0)))
            return {};

        std::size_t match = 1;

        while (!ruleProxy.IsOutOfBounds(match)) {
            auto c = ruleProxy.Peek(match);
            if (IsDigit(c) || IsNonDigit(c))
                match++;
            else
                break;
        }

        return {TokenKind::Identifier, match};
    }

    template <typename Context>
    TokenData<Char> ExecuteAction(const LexerRuleProxy<Char> &ruleProxy, const RuleResult &result, const Context &) {

        auto it = knownIdentifiers.emplace(ruleProxy.CopyString(result.matchLength)).first;

        return TokenData<Char>::MakeString(it->c_str());
    }

  private:
    using string_type = std::basic_string<Char>;

    /** Repeated identifiers are stored here. */
    std::set<string_type> knownIdentifiers;
};

} // namespace ispc
