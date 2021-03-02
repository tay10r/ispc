#pragma once

#include <cstddef>

namespace ispc {

struct LexerOptions final {
    /** This option will cause integer suffixes to have more strict checking
        than the previous Flex implementation. For example, previously this
        suffix would be allowable: "uulllkG", which leads to an unsigned 64-bit
        integer with a multiplier of 1024^4. In most (all?) C++ compilers, the
        extraneous occurance of 'u' and 'l' would cause an error. If this option
        is enabled, it will treat extraneous characters as an error. It will
        also disable more than one multiplier character. Finally, it will also
        treat all characters at and after the first non-digit character as part
        of the suffix sequence.
     */
    bool strictIntegerSuffixes = false;
    /** Enabling this option will cause integer overflows detected while
        scanning integer constants to emit a warning diagnostic. This also
        accounts for overflows due to multiplier characters. The original
        behavior is to ignore all overflows.
     */
    bool warnIntegerOverflow = false;
    /** Enabling this option will cause carriage returns and line feeds
        to be treated as a single newline. On Windows systems, newline sequences
        generally consist of a carriage return followed by a line feed
        character. Enabling this option will cause these characters to be put
        into a single character. It will also disable carriage return appearing
        in whitespace tokens, when a carriage return is followed by a newline.
        The original behavior of the Flex lexer was to treat all carriage return
        characters as whitespace characters.
     */
    bool checkCRLF = false;
    /** Indicates the number of bits in each SIMD lane. The default is 32-bits
        but it may also be 8, 16, or 64. This affects the token types returned
        by rules for constants.
     */
    std::size_t laneBits = 32;
};

} // namespace ispc
