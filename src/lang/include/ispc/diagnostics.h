#pragma once

#include <ispc/source_range.h>

namespace ispc {

enum class Severity {
    /** Used to provide extra information on other diagnostics. */
    Remark,
    /** Indicates a diagnostic can be ignored. */
    Warning,
    /** Indicates that the source code should not be compiled. */
    Error,
    /** Indicates all source code processing should immediately halt. */
    Fatal
};

/** Enumerates all unique diagnostic types. */
enum class DiagnosticID {
    /** Occurs when an integer constant exceeds 64-bits. */
    IntegerOverflow,
    /** Occurs when the integer suffix is not recognized. */
    InvalidIntegerSuffix
};

/** Contains all the information required to issue a diagnostic. */
struct Diagnostic final {
    DiagnosticID id;
    Severity severity;
    /** The source range of the token causing the diagnostic. */
    SourceRange sourceRange;
};

} // namespace ispc
