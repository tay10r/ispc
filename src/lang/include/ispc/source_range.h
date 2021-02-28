#pragma once

#include <cstddef>

namespace ispc {

/** @brief Describes the range in which a token occupies the input file.

    @detail This structure is suppose to preserve compatibility with Bison. It
    therefore has some inconsistencies with other parts of the language library,
    such as integer types instead of size_t and snake-case names. Prefer to use
    the accessor methods for new code.
 */
struct SourceRange final {
    int first_line = 1;
    int first_column = 1;
    int last_line = 1;
    int last_column = 1;

    std::size_t GetFirstLine() const noexcept { return std::size_t(first_line); }
    std::size_t GetFirstColumn() const noexcept { return std::size_t(first_column); }
    std::size_t GetLastLine() const noexcept { return std::size_t(last_line); }
    std::size_t GetLastColumn() const noexcept { return std::size_t(last_column); }
};

} // namespace ispc
