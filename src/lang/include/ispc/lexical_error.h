#pragma once

#include <ispc/diagnostics.h>

namespace ispc {

struct LexicalError final {
    DiagnosticID diagnosticID;
    std::size_t index = 0;
    std::size_t length = 0;
};

} // namespace ispc
