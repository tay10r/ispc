#pragma once

#include <map>
#include <memory>
#include <sstream>
#include <vector>

#define Assert(expr)

class SymbolTableLexerAdapter {
  public:
    virtual ~SymbolTableLexerAdapter() = default;
    virtual bool IsType(const char *name) const = 0;
};

struct SourcePos final {
    /// The name of the source file.
    const char *name = "";
    int first_line = 1;
    int first_column = 1;
    int last_line = 1;
    int last_column = 1;
};

enum PragmaUnrollType { none, nounroll, unroll, count };

struct PragmaAttributes {
    enum class AttributeType { none, pragmaloop, pragmawarning };
    PragmaAttributes() {
        aType = AttributeType::none;
        unrollType = PragmaUnrollType::none;
        count = -1;
    }
    AttributeType aType;
    PragmaUnrollType unrollType;
    int count;
};

union SemanticValue {
    PragmaAttributes *pragmaAttributes;
    std::string *stringVal;
    double doubleVal;
    float floatVal;
    std::uint64_t intVal;
};

struct FlexLexerData final {
    int dataTypeWidth = 4;
    SourcePos yylloc;
    SemanticValue yylval;
    std::ostringstream log;
    std::unique_ptr<SymbolTableLexerAdapter> symbolTable;
    // Lines for which warnings are turned off.
    std::map<std::pair<int, std::string>, bool> turnOffWarnings;
    std::vector<std::string> dependencies;

    template <typename Arg> void WriteToLog(const Arg &arg) { this->log << arg; }

    template <typename Arg, typename... OtherArgs> void WriteToLog(const Arg &arg, const OtherArgs &... otherArgs) {
        this->log << arg;
        this->WriteToLog(otherArgs...);
    }

    void RegisterDependency(const std::string &d) { this->dependencies.emplace_back(d); }
};

template <typename... Args> void Warning(FlexLexerData *lexerData, const Args &... args) {
    lexerData->WriteToLog("warning: ");
    lexerData->WriteToLog(args...);
    lexerData->WriteToLog('\n');
}

template <typename... Args> void Error(FlexLexerData *lexerData, const Args &... args) {
    lexerData->WriteToLog("error: ");
    lexerData->WriteToLog(args...);
    lexerData->WriteToLog('\n');
}

class LexerImpl;

LexerImpl *MakeFlexLexer();

enum TokenKind {
    TOKEN_ASSERT,
    TOKEN_BOOL,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CDO,
    TOKEN_CFOR,
    TOKEN_CIF,
    TOKEN_CWHILE,
    TOKEN_CONST,
    TOKEN_CONTINUE,
    TOKEN_DECLSPEC,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_DELETE,
    TOKEN_DOUBLE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_EXPORT,
    TOKEN_EXTERN,
    TOKEN_FALSE,
    TOKEN_FLOAT,
    TOKEN_FOR,
    TOKEN_FOREACH,
    TOKEN_FOREACH_ACTIVE,
    TOKEN_FOREACH_TILED,
    TOKEN_FOREACH_UNIQUE,
    TOKEN_GOTO,
    TOKEN_IDENTIFIER,
    TOKEN_IF,
    TOKEN_IN,
    TOKEN_INLINE,
    TOKEN_INT8,
    TOKEN_INT16,
    TOKEN_INT,
    TOKEN_INT32DOTDOTDOT_CONSTANT,
    TOKEN_INT64,
    TOKEN_INT64DOTDOTDOT_CONSTANT,
    TOKEN_LAUNCH,
    TOKEN_UINT,
    TOKEN_UINT8,
    TOKEN_UINT16,
    TOKEN_UINT32DOTDOTDOT_CONSTANT,
    TOKEN_UINT64,
    TOKEN_UINT64DOTDOTDOT_CONSTANT,
    TOKEN_NEW,
    TOKEN_NULL,
    TOKEN_PRAGMA,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SOA,
    TOKEN_SIGNED,
    TOKEN_SIZEOF,
    TOKEN_STATIC,
    TOKEN_STRING_LITERAL,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_SYNC,
    TOKEN_TASK,
    TOKEN_TRUE,
    TOKEN_TYPEDEF,
    TOKEN_TYPE_NAME,
    TOKEN_UNIFORM,
    TOKEN_UNMASKED,
    TOKEN_UNSIGNED,
    TOKEN_VARYING,
    TOKEN_VOID,
    TOKEN_WHILE,
    TOKEN_STRING_C_LITERAL,
    TOKEN_DOTDOTDOT,
    TOKEN_FLOAT_CONSTANT,
    TOKEN_DOUBLE_CONSTANT,
    TOKEN_INT8_CONSTANT,
    TOKEN_UINT8_CONSTANT,
    TOKEN_INT16_CONSTANT,
    TOKEN_UINT16_CONSTANT,
    TOKEN_INT32_CONSTANT,
    TOKEN_UINT32_CONSTANT,
    TOKEN_INT64_CONSTANT,
    TOKEN_UINT64_CONSTANT,
    TOKEN_INC_OP,
    TOKEN_DEC_OP,
    TOKEN_LEFT_OP,
    TOKEN_RIGHT_OP,
    TOKEN_LE_OP,
    TOKEN_GE_OP,
    TOKEN_EQ_OP,
    TOKEN_NE_OP,
    TOKEN_AND_OP,
    TOKEN_OR_OP,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_LEFT_ASSIGN,
    TOKEN_RIGHT_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_PTR_OP,
    TOKEN_NOINLINE,
    TOKEN_VECTORCALL
};

#define YYSTYPE SemanticValue
