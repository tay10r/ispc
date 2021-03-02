#include "lexer_impl_flex.h"

#include "lexer_impl.h"

#include "flex_output.h"

#include <set>
#include <string>

void ParserInit();

namespace {

const char *ToName(TokenKind kind) {
    switch (kind) {
    case TOKEN_ASSERT:
        return "assert";
    case TOKEN_BOOL:
        return "bool";
    case TOKEN_BREAK:
        return "break";
    case TOKEN_CASE:
        return "case";
    case TOKEN_CDO:
        return "cdo";
    case TOKEN_CFOR:
        return "cfor";
    case TOKEN_CIF:
        return "cif";
    case TOKEN_CWHILE:
        return "cwhile";
    case TOKEN_CONST:
        return "const";
    case TOKEN_CONTINUE:
        return "continue";
    case TOKEN_DECLSPEC:
        return "declspec";
    case TOKEN_DEFAULT:
        return "default";
    case TOKEN_DO:
        return "do";
    case TOKEN_DELETE:
        return "delete";
    case TOKEN_DOUBLE:
        return "double";
    case TOKEN_ELSE:
        return "else";
    case TOKEN_ENUM:
        return "enum";
    case TOKEN_EXPORT:
        return "export";
    case TOKEN_EXTERN:
        return "extern";
    case TOKEN_FALSE:
        return "false";
    case TOKEN_FLOAT:
        return "float";
    case TOKEN_FOR:
        return "for";
    case TOKEN_FOREACH:
        return "foreach";
    case TOKEN_FOREACH_ACTIVE:
        return "foreach_active";
    case TOKEN_FOREACH_TILED:
        return "foreach_tiled";
    case TOKEN_FOREACH_UNIQUE:
        return "foreach_unique";
    case TOKEN_GOTO:
        return "goto";
    case TOKEN_IDENTIFIER:
        return "identifier";
    case TOKEN_IF:
        return "if";
    case TOKEN_IN:
        return "in";
    case TOKEN_INLINE:
        return "inline";
    case TOKEN_INT8:
        return "int8";
    case TOKEN_INT16:
        return "int16";
    case TOKEN_INT:
        return "int";
    case TOKEN_INT32DOTDOTDOT_CONSTANT:
        return "int32_constant...";
    case TOKEN_INT64:
        return "int64";
    case TOKEN_INT64DOTDOTDOT_CONSTANT:
        return "int64_constant...";
    case TOKEN_LAUNCH:
        return "launch";
    case TOKEN_UINT:
        return "uint";
    case TOKEN_UINT8:
        return "uint8";
    case TOKEN_UINT16:
        return "uint16";
    case TOKEN_UINT32DOTDOTDOT_CONSTANT:
        return "uint32_constant...";
    case TOKEN_UINT64:
        return "uint64";
    case TOKEN_UINT64DOTDOTDOT_CONSTANT:
        return "uint64_constant...";
    case TOKEN_NEW:
        return "new";
    case TOKEN_NULL:
        return "null";
    case TOKEN_PRAGMA:
        return "pragma";
    case TOKEN_PRINT:
        return "print";
    case TOKEN_RETURN:
        return "return";
    case TOKEN_SOA:
        return "soa";
    case TOKEN_SIGNED:
        return "signed";
    case TOKEN_SIZEOF:
        return "sizeof";
    case TOKEN_STATIC:
        return "static";
    case TOKEN_STRING_LITERAL:
        return "string_literal";
    case TOKEN_STRUCT:
        return "struct";
    case TOKEN_SWITCH:
        return "switch";
    case TOKEN_SYNC:
        return "sync";
    case TOKEN_TASK:
        return "task";
    case TOKEN_TRUE:
        return "true";
    case TOKEN_TYPEDEF:
        return "typedef";
    case TOKEN_TYPE_NAME:
        return "type_name";
    case TOKEN_UNIFORM:
        return "uniform";
    case TOKEN_UNMASKED:
        return "unmasked";
    case TOKEN_UNSIGNED:
        return "unsigned";
    case TOKEN_VARYING:
        return "varying";
    case TOKEN_VOID:
        return "void";
    case TOKEN_WHILE:
        return "while";
    case TOKEN_STRING_C_LITERAL:
        return "string_c_literal";
    case TOKEN_DOTDOTDOT:
        return "...";
    case TOKEN_FLOAT_CONSTANT:
        return "float_constant";
    case TOKEN_DOUBLE_CONSTANT:
        return "double_constant";
    case TOKEN_INT8_CONSTANT:
        return "int8_constant";
    case TOKEN_UINT8_CONSTANT:
        return "uint8_constant";
    case TOKEN_INT16_CONSTANT:
        return "int16_constant";
    case TOKEN_UINT16_CONSTANT:
        return "uint16_constant";
    case TOKEN_INT32_CONSTANT:
        return "int32_constant";
    case TOKEN_UINT32_CONSTANT:
        return "uint32_constant";
    case TOKEN_INT64_CONSTANT:
        return "int64_constant";
    case TOKEN_UINT64_CONSTANT:
        return "uint64_constant";
    case TOKEN_INC_OP:
        return "++";
    case TOKEN_DEC_OP:
        return "--";
    case TOKEN_LEFT_OP:
        return "<<";
    case TOKEN_RIGHT_OP:
        return ">>";
    case TOKEN_LE_OP:
        return "<=";
    case TOKEN_GE_OP:
        return ">=";
    case TOKEN_EQ_OP:
        return "==";
    case TOKEN_NE_OP:
        return "!=";
    case TOKEN_AND_OP:
        return "&&";
    case TOKEN_OR_OP:
        return "||";
    case TOKEN_MUL_ASSIGN:
        return "*=";
    case TOKEN_DIV_ASSIGN:
        return "/=";
    case TOKEN_MOD_ASSIGN:
        return "%=";
    case TOKEN_ADD_ASSIGN:
        return "+=";
    case TOKEN_SUB_ASSIGN:
        return "-=";
    case TOKEN_LEFT_ASSIGN:
        return "<<=";
    case TOKEN_RIGHT_ASSIGN:
        return ">>=";
    case TOKEN_AND_ASSIGN:
        return "&=";
    case TOKEN_XOR_ASSIGN:
        return "^=";
    case TOKEN_OR_ASSIGN:
        return "|=";
    case TOKEN_PTR_OP:
        return "->";
    case TOKEN_NOINLINE:
        return "noinline";
    case TOKEN_VECTORCALL:
        return "vectorcall";
    }
    return "symbol";
}

class FakeSymbolTable final : public SymbolTableLexerAdapter {
    std::set<std::string> types;

  public:
    bool IsType(const char *name) const override { return types.find(name) != types.end(); }
};

struct FlexToken final {
    TokenKind kind;
    SemanticValue value;
    SourcePos location;
};

class FlexScanner final : public LexerImpl {
    yyscan_t scanner;
    FlexLexerData extraData;
    std::vector<FlexToken> tokens;

  public:
    FlexScanner(std::size_t laneBits) {
        ParserInit();
        extraData.symbolTable.reset(new FakeSymbolTable());
        extraData.dataTypeWidth = laneBits;
        yylex_init_extra(&extraData, &scanner);
    }

    ~FlexScanner() {
        for (auto &tok : tokens)
            FreeToken(tok);
        yylex_destroy(scanner);
    }

    void ScanTokens() override {

        for (;;) {

            auto tokenKind = yylex(scanner);

            if (tokenKind <= 0)
                break;

            FlexToken tok{TokenKind(tokenKind), extraData.yylval, extraData.yylloc};

            tokens.emplace_back(tok);
        }
    }

    void SetInput(const std::string &str) override { yy_scan_bytes(str.data(), str.size(), scanner); }

    void PrintTokens(std::ostream &stream) const override {
        for (const auto &tok : tokens)
            PrintToken(tok, stream);
    }

  protected:
    static void FreeToken(FlexToken &tok) {
        switch (tok.kind) {
        case TOKEN_IDENTIFIER:
        case TOKEN_STRING_LITERAL:
        case TOKEN_STRING_C_LITERAL:
            delete tok.value.stringVal;
            break;
        default:
            break;
        }
    }
    void PrintToken(const FlexToken &tok, std::ostream &stream) const {
        stream << ToName(tok.kind);
        stream << " '";
        switch (tok.kind) {
        case TOKEN_IDENTIFIER:
        case TOKEN_STRING_LITERAL:
        case TOKEN_STRING_C_LITERAL:
            stream << *tok.value.stringVal;
            break;
        case TOKEN_FLOAT_CONSTANT:
            stream << tok.value.floatVal;
            break;
        case TOKEN_DOUBLE_CONSTANT:
            stream << tok.value.doubleVal;
            break;
        case TOKEN_INT32DOTDOTDOT_CONSTANT:
        case TOKEN_INT64DOTDOTDOT_CONSTANT:
        case TOKEN_UINT32DOTDOTDOT_CONSTANT:
        case TOKEN_UINT64DOTDOTDOT_CONSTANT:
        case TOKEN_INT8_CONSTANT:
        case TOKEN_UINT8_CONSTANT:
        case TOKEN_INT16_CONSTANT:
        case TOKEN_UINT16_CONSTANT:
        case TOKEN_INT32_CONSTANT:
        case TOKEN_UINT32_CONSTANT:
        case TOKEN_INT64_CONSTANT:
        case TOKEN_UINT64_CONSTANT:
            stream << tok.value.intVal;
            break;
            break;
        default:
            break;
        }
        stream << "' ";
        stream << "from " << tok.location.first_line << ':' << tok.location.first_column;
        stream << ' ';
        stream << "to " << tok.location.last_line << ':' << tok.location.last_column;
        stream << std::endl;
    }
};

} // namespace

LexerImpl *MakeFlexLexer(std::size_t laneBits) { return new FlexScanner(laneBits); }
