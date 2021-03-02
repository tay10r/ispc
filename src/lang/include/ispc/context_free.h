#pragma once

namespace ispc {

/** @brief This is a tag to indicate that lexer or parser rule is context free.

    @detail Lexer or parser rules can have function overloads for context
    sensitive rules that take a parameter with a type other than this one.
 * */
class ContextFree final {};

} // namespace ispc
