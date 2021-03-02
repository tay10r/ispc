/*
  Copyright (c) 2010-2019, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

%{
#include "lexer_impl_flex.h"

#include <map>
#include <string>

#include <stdlib.h>
#include <stdint.h>

static uint64_t lParseBinary(FlexLexerData *lexerData, const char *ptr, SourcePos pos, char **endPtr);
static int lParseInteger(FlexLexerData *lexerData, const char *tokenText, bool dotdotdot);
static void lCComment(FlexLexerData* lexerData, yyscan_t scanner);
static void lCppComment(yyscan_t scanner, SourcePos *);
static void lNextValidChar(SourcePos *, char const*&);
static void lPragmaIgnoreWarning(FlexLexerData *, std::string);
static void lPragmaUnroll(FlexLexerData* lexerData, std::string, bool);
static bool lConsumePragma(FlexLexerData *, yyscan_t);
static void lHandleCppHash(FlexLexerData* lexerData, char *tokenText);
static void lStringConst(FlexLexerData* lexerData, YYSTYPE *, SourcePos *, char *tokenText);
static double lParseHexFloat(const char *ptr);
extern void RegisterDependency(const std::string &fileName);

#define YY_USER_ACTION \
    yyextra->yylloc.first_line = yyextra->yylloc.last_line; \
    yyextra->yylloc.first_column = yyextra->yylloc.last_column; \
    yyextra->yylloc.last_column += yyleng;

#ifdef ISPC_HOST_IS_WINDOWS
inline int isatty(int) { return 0; }
#else
#include <unistd.h>
#endif // ISPC_HOST_IS_WINDOWS

static int allTokens[] = {
  TOKEN_ASSERT, TOKEN_BOOL, TOKEN_BREAK, TOKEN_CASE,
  TOKEN_CDO, TOKEN_CFOR, TOKEN_CIF, TOKEN_CWHILE,
  TOKEN_CONST, TOKEN_CONTINUE, TOKEN_DEFAULT, TOKEN_DO,
  TOKEN_DELETE, TOKEN_DOUBLE, TOKEN_ELSE, TOKEN_ENUM,
  TOKEN_EXPORT, TOKEN_EXTERN, TOKEN_FALSE, TOKEN_FLOAT, TOKEN_FOR,
  TOKEN_FOREACH, TOKEN_FOREACH_ACTIVE, TOKEN_FOREACH_TILED,
  TOKEN_FOREACH_UNIQUE, TOKEN_GOTO, TOKEN_IF, TOKEN_IN, TOKEN_INLINE,
  TOKEN_INT, TOKEN_INT8, TOKEN_INT16, TOKEN_INT, TOKEN_INT64, TOKEN_LAUNCH,
  TOKEN_UINT, TOKEN_UINT8, TOKEN_UINT16, TOKEN_UINT64,
  TOKEN_NEW, TOKEN_NULL, TOKEN_PRINT, TOKEN_RETURN, TOKEN_SOA, TOKEN_SIGNED,
  TOKEN_SIZEOF, TOKEN_STATIC, TOKEN_STRUCT, TOKEN_SWITCH, TOKEN_SYNC,
  TOKEN_TASK, TOKEN_TRUE, TOKEN_TYPEDEF, TOKEN_UNIFORM, TOKEN_UNMASKED,
  TOKEN_UNSIGNED, TOKEN_VARYING, TOKEN_VOID, TOKEN_WHILE,
  TOKEN_STRING_C_LITERAL, TOKEN_DOTDOTDOT,
  TOKEN_FLOAT_CONSTANT, TOKEN_DOUBLE_CONSTANT,
  TOKEN_INT8_CONSTANT, TOKEN_UINT8_CONSTANT,
  TOKEN_INT16_CONSTANT, TOKEN_UINT16_CONSTANT,
  TOKEN_INT32_CONSTANT, TOKEN_UINT32_CONSTANT,
  TOKEN_INT64_CONSTANT, TOKEN_UINT64_CONSTANT,
  TOKEN_INC_OP, TOKEN_DEC_OP, TOKEN_LEFT_OP, TOKEN_RIGHT_OP, TOKEN_LE_OP,
  TOKEN_GE_OP, TOKEN_EQ_OP, TOKEN_NE_OP, TOKEN_AND_OP, TOKEN_OR_OP,
  TOKEN_MUL_ASSIGN, TOKEN_DIV_ASSIGN, TOKEN_MOD_ASSIGN, TOKEN_ADD_ASSIGN,
  TOKEN_SUB_ASSIGN, TOKEN_LEFT_ASSIGN, TOKEN_RIGHT_ASSIGN, TOKEN_AND_ASSIGN,
  TOKEN_XOR_ASSIGN, TOKEN_OR_ASSIGN, TOKEN_PTR_OP, TOKEN_NOINLINE, TOKEN_VECTORCALL,
  ';', '{', '}', ',', ':', '=', '(', ')', '[', ']', '.', '&', '!', '~', '-',
  '+', '*', '/', '%', '<', '>', '^', '|', '?',
};

std::map<int, std::string> tokenToName;
std::map<std::string, std::string> tokenNameRemap;

const char *TokenToName(int kind) {
  auto it = tokenToName.find(kind);
  if (it == tokenToName.end())
    return "<unknown-token>";
  else
    return it->second.c_str();
}

void ParserInit() {
    tokenToName[TOKEN_ASSERT] = "assert";
    tokenToName[TOKEN_BOOL] = "bool";
    tokenToName[TOKEN_BREAK] = "break";
    tokenToName[TOKEN_CASE] = "case";
    tokenToName[TOKEN_CDO] = "cdo";
    tokenToName[TOKEN_CFOR] = "cfor";
    tokenToName[TOKEN_CIF] = "cif";
    tokenToName[TOKEN_CWHILE] = "cwhile";
    tokenToName[TOKEN_CONST] = "const";
    tokenToName[TOKEN_CONTINUE] = "continue";
    tokenToName[TOKEN_DEFAULT] = "default";
    tokenToName[TOKEN_DO] = "do";
    tokenToName[TOKEN_DELETE] = "delete";
    tokenToName[TOKEN_DOUBLE] = "double";
    tokenToName[TOKEN_ELSE] = "else";
    tokenToName[TOKEN_ENUM] = "enum";
    tokenToName[TOKEN_EXPORT] = "export";
    tokenToName[TOKEN_EXTERN] = "extern";
    tokenToName[TOKEN_FALSE] = "false";
    tokenToName[TOKEN_FLOAT] = "float";
    tokenToName[TOKEN_FOR] = "for";
    tokenToName[TOKEN_FOREACH] = "foreach";
    tokenToName[TOKEN_FOREACH_ACTIVE] = "foreach_active";
    tokenToName[TOKEN_FOREACH_TILED] = "foreach_tiled";
    tokenToName[TOKEN_FOREACH_UNIQUE] = "foreach_unique";
    tokenToName[TOKEN_GOTO] = "goto";
    tokenToName[TOKEN_IF] = "if";
    tokenToName[TOKEN_IN] = "in";
    tokenToName[TOKEN_INLINE] = "inline";
    tokenToName[TOKEN_NOINLINE] = "noinline";
    tokenToName[TOKEN_VECTORCALL] = "__vectorcall";
    tokenToName[TOKEN_INT] = "int";
    tokenToName[TOKEN_UINT] = "uint";
    tokenToName[TOKEN_INT8] = "int8";
    tokenToName[TOKEN_UINT8] = "uint8";
    tokenToName[TOKEN_INT16] = "int16";
    tokenToName[TOKEN_UINT16] = "uint16";
    tokenToName[TOKEN_INT] = "int";
    tokenToName[TOKEN_INT64] = "int64";
    tokenToName[TOKEN_UINT64] = "uint64";
    tokenToName[TOKEN_LAUNCH] = "launch";
    tokenToName[TOKEN_NEW] = "new";
    tokenToName[TOKEN_NULL] = "NULL";
    tokenToName[TOKEN_PRINT] = "print";
    tokenToName[TOKEN_RETURN] = "return";
    tokenToName[TOKEN_SOA] = "soa";
    tokenToName[TOKEN_SIGNED] = "signed";
    tokenToName[TOKEN_SIZEOF] = "sizeof";
    tokenToName[TOKEN_STATIC] = "static";
    tokenToName[TOKEN_STRUCT] = "struct";
    tokenToName[TOKEN_SWITCH] = "switch";
    tokenToName[TOKEN_SYNC] = "sync";
    tokenToName[TOKEN_TASK] = "task";
    tokenToName[TOKEN_TRUE] = "true";
    tokenToName[TOKEN_TYPEDEF] = "typedef";
    tokenToName[TOKEN_UNIFORM] = "uniform";
    tokenToName[TOKEN_UNMASKED] = "unmasked";
    tokenToName[TOKEN_UNSIGNED] = "unsigned";
    tokenToName[TOKEN_VARYING] = "varying";
    tokenToName[TOKEN_VOID] = "void";
    tokenToName[TOKEN_WHILE] = "while";
    tokenToName[TOKEN_STRING_C_LITERAL] = "\"C\"";
    tokenToName[TOKEN_DOTDOTDOT] = "...";
    tokenToName[TOKEN_FLOAT_CONSTANT] = "TOKEN_FLOAT_CONSTANT";
    tokenToName[TOKEN_DOUBLE_CONSTANT] = "TOKEN_DOUBLE_CONSTANT";
    tokenToName[TOKEN_INT8_CONSTANT] = "TOKEN_INT8_CONSTANT";
    tokenToName[TOKEN_UINT8_CONSTANT] = "TOKEN_UINT8_CONSTANT";
    tokenToName[TOKEN_INT16_CONSTANT] = "TOKEN_INT16_CONSTANT";
    tokenToName[TOKEN_UINT16_CONSTANT] = "TOKEN_UINT16_CONSTANT";
    tokenToName[TOKEN_INT32_CONSTANT] = "TOKEN_INT32_CONSTANT";
    tokenToName[TOKEN_UINT32_CONSTANT] = "TOKEN_UINT32_CONSTANT";
    tokenToName[TOKEN_INT64_CONSTANT] = "TOKEN_INT64_CONSTANT";
    tokenToName[TOKEN_UINT64_CONSTANT] = "TOKEN_UINT64_CONSTANT";
    tokenToName[TOKEN_INC_OP] = "++";
    tokenToName[TOKEN_DEC_OP] = "--";
    tokenToName[TOKEN_LEFT_OP] = "<<";
    tokenToName[TOKEN_RIGHT_OP] = ">>";
    tokenToName[TOKEN_LE_OP] = "<=";
    tokenToName[TOKEN_GE_OP] = ">=";
    tokenToName[TOKEN_EQ_OP] = "==";
    tokenToName[TOKEN_NE_OP] = "!=";
    tokenToName[TOKEN_AND_OP] = "&&";
    tokenToName[TOKEN_OR_OP] = "||";
    tokenToName[TOKEN_MUL_ASSIGN] = "*=";
    tokenToName[TOKEN_DIV_ASSIGN] = "/=";
    tokenToName[TOKEN_MOD_ASSIGN] = "%=";
    tokenToName[TOKEN_ADD_ASSIGN] = "+=";
    tokenToName[TOKEN_SUB_ASSIGN] = "-=";
    tokenToName[TOKEN_LEFT_ASSIGN] = "<<=";
    tokenToName[TOKEN_RIGHT_ASSIGN] = ">>=";
    tokenToName[TOKEN_AND_ASSIGN] = "&=";
    tokenToName[TOKEN_XOR_ASSIGN] = "^=";
    tokenToName[TOKEN_OR_ASSIGN] = "|=";
    tokenToName[TOKEN_PTR_OP] = "->";
    tokenToName[';'] = ";";
    tokenToName['{'] = "{";
    tokenToName['}'] = "}";
    tokenToName[','] = ",";
    tokenToName[':'] = ":";
    tokenToName['='] = "=";
    tokenToName['('] = "(";
    tokenToName[')'] = ")";
    tokenToName['['] = "[";
    tokenToName[']'] = "]";
    tokenToName['.'] = ".";
    tokenToName['&'] = "&";
    tokenToName['!'] = "!";
    tokenToName['~'] = "~";
    tokenToName['-'] = "-";
    tokenToName['+'] = "+";
    tokenToName['*'] = "*";
    tokenToName['/'] = "/";
    tokenToName['%'] = "%";
    tokenToName['<'] = "<";
    tokenToName['>'] = ">";
    tokenToName['^'] = "^";
    tokenToName['|'] = "|";
    tokenToName['?'] = "?";
    tokenToName[';'] = ";";

    tokenNameRemap["TOKEN_ASSERT"] = "\'assert\'";
    tokenNameRemap["TOKEN_BOOL"] = "\'bool\'";
    tokenNameRemap["TOKEN_BREAK"] = "\'break\'";
    tokenNameRemap["TOKEN_CASE"] = "\'case\'";
    tokenNameRemap["TOKEN_CDO"] = "\'cdo\'";
    tokenNameRemap["TOKEN_CFOR"] = "\'cfor\'";
    tokenNameRemap["TOKEN_CIF"] = "\'cif\'";
    tokenNameRemap["TOKEN_CWHILE"] = "\'cwhile\'";
    tokenNameRemap["TOKEN_CONST"] = "\'const\'";
    tokenNameRemap["TOKEN_CONTINUE"] = "\'continue\'";
    tokenNameRemap["TOKEN_DEFAULT"] = "\'default\'";
    tokenNameRemap["TOKEN_DO"] = "\'do\'";
    tokenNameRemap["TOKEN_DELETE"] = "\'delete\'";
    tokenNameRemap["TOKEN_DOUBLE"] = "\'double\'";
    tokenNameRemap["TOKEN_ELSE"] = "\'else\'";
    tokenNameRemap["TOKEN_ENUM"] = "\'enum\'";
    tokenNameRemap["TOKEN_EXPORT"] = "\'export\'";
    tokenNameRemap["TOKEN_EXTERN"] = "\'extern\'";
    tokenNameRemap["TOKEN_FALSE"] = "\'false\'";
    tokenNameRemap["TOKEN_FLOAT"] = "\'float\'";
    tokenNameRemap["TOKEN_FOR"] = "\'for\'";
    tokenNameRemap["TOKEN_FOREACH"] = "\'foreach\'";
    tokenNameRemap["TOKEN_FOREACH_ACTIVE"] = "\'foreach_active\'";
    tokenNameRemap["TOKEN_FOREACH_TILED"] = "\'foreach_tiled\'";
    tokenNameRemap["TOKEN_FOREACH_UNIQUE"] = "\'foreach_unique\'";
    tokenNameRemap["TOKEN_GOTO"] = "\'goto\'";
    tokenNameRemap["TOKEN_IDENTIFIER"] = "identifier";
    tokenNameRemap["TOKEN_IF"] = "\'if\'";
    tokenNameRemap["TOKEN_IN"] = "\'in\'";
    tokenNameRemap["TOKEN_INLINE"] = "\'inline\'";
    tokenNameRemap["TOKEN_NOINLINE"] = "\'noinline\'";
    tokenNameRemap["TOKEN_VECTORCALL"] = "\'__vectorcall\'";
    tokenNameRemap["TOKEN_INT"] = "\'int\'";
    tokenNameRemap["TOKEN_UINT"] = "\'uint\'";
    tokenNameRemap["TOKEN_INT8"] = "\'int8\'";
    tokenNameRemap["TOKEN_UINT8"] = "\'uint8\'";
    tokenNameRemap["TOKEN_INT16"] = "\'int16\'";
    tokenNameRemap["TOKEN_UINT16"] = "\'uint16\'";
    tokenNameRemap["TOKEN_INT"] = "\'int\'";
    tokenNameRemap["TOKEN_INT64"] = "\'int64\'";
    tokenNameRemap["TOKEN_UINT64"] = "\'uint64\'";
    tokenNameRemap["TOKEN_LAUNCH"] = "\'launch\'";
    tokenNameRemap["TOKEN_NEW"] = "\'new\'";
    tokenNameRemap["TOKEN_NULL"] = "\'NULL\'";
    tokenNameRemap["TOKEN_PRINT"] = "\'print\'";
    tokenNameRemap["TOKEN_RETURN"] = "\'return\'";
    tokenNameRemap["TOKEN_SOA"] = "\'soa\'";
    tokenNameRemap["TOKEN_SIGNED"] = "\'signed\'";
    tokenNameRemap["TOKEN_SIZEOF"] = "\'sizeof\'";
    tokenNameRemap["TOKEN_STATIC"] = "\'static\'";
    tokenNameRemap["TOKEN_STRUCT"] = "\'struct\'";
    tokenNameRemap["TOKEN_SWITCH"] = "\'switch\'";
    tokenNameRemap["TOKEN_SYNC"] = "\'sync\'";
    tokenNameRemap["TOKEN_TASK"] = "\'task\'";
    tokenNameRemap["TOKEN_TRUE"] = "\'true\'";
    tokenNameRemap["TOKEN_TYPEDEF"] = "\'typedef\'";
    tokenNameRemap["TOKEN_UNIFORM"] = "\'uniform\'";
    tokenNameRemap["TOKEN_UNMASKED"] = "\'unmasked\'";
    tokenNameRemap["TOKEN_UNSIGNED"] = "\'unsigned\'";
    tokenNameRemap["TOKEN_VARYING"] = "\'varying\'";
    tokenNameRemap["TOKEN_VOID"] = "\'void\'";
    tokenNameRemap["TOKEN_WHILE"] = "\'while\'";
    tokenNameRemap["TOKEN_STRING_C_LITERAL"] = "\"C\"";
    tokenNameRemap["TOKEN_DOTDOTDOT"] = "\'...\'";
    tokenNameRemap["TOKEN_FLOAT_CONSTANT"] = "float constant";
    tokenNameRemap["TOKEN_DOUBLE_CONSTANT"] = "double constant";
    tokenNameRemap["TOKEN_INT8_CONSTANT"] = "int8 constant";
    tokenNameRemap["TOKEN_UINT8_CONSTANT"] = "unsigned int8 constant";
    tokenNameRemap["TOKEN_INT16_CONSTANT"] = "int16 constant";
    tokenNameRemap["TOKEN_UINT16_CONSTANT"] = "unsigned int16 constant";
    tokenNameRemap["TOKEN_INT32_CONSTANT"] = "int32 constant";
    tokenNameRemap["TOKEN_UINT32_CONSTANT"] = "unsigned int32 constant";
    tokenNameRemap["TOKEN_INT64_CONSTANT"] = "int64 constant";
    tokenNameRemap["TOKEN_UINT64_CONSTANT"] = "unsigned int64 constant";
    tokenNameRemap["TOKEN_INC_OP"] = "\'++\'";
    tokenNameRemap["TOKEN_DEC_OP"] = "\'--\'";
    tokenNameRemap["TOKEN_LEFT_OP"] = "\'<<\'";
    tokenNameRemap["TOKEN_RIGHT_OP"] = "\'>>\'";
    tokenNameRemap["TOKEN_LE_OP"] = "\'<=\'";
    tokenNameRemap["TOKEN_GE_OP"] = "\'>=\'";
    tokenNameRemap["TOKEN_EQ_OP"] = "\'==\'";
    tokenNameRemap["TOKEN_NE_OP"] = "\'!=\'";
    tokenNameRemap["TOKEN_AND_OP"] = "\'&&\'";
    tokenNameRemap["TOKEN_OR_OP"] = "\'||\'";
    tokenNameRemap["TOKEN_MUL_ASSIGN"] = "\'*=\'";
    tokenNameRemap["TOKEN_DIV_ASSIGN"] = "\'/=\'";
    tokenNameRemap["TOKEN_MOD_ASSIGN"] = "\'%=\'";
    tokenNameRemap["TOKEN_ADD_ASSIGN"] = "\'+=\'";
    tokenNameRemap["TOKEN_SUB_ASSIGN"] = "\'-=\'";
    tokenNameRemap["TOKEN_LEFT_ASSIGN"] = "\'<<=\'";
    tokenNameRemap["TOKEN_RIGHT_ASSIGN"] = "\'>>=\'";
    tokenNameRemap["TOKEN_AND_ASSIGN"] = "\'&=\'";
    tokenNameRemap["TOKEN_XOR_ASSIGN"] = "\'^=\'";
    tokenNameRemap["TOKEN_OR_ASSIGN"] = "\'|=\'";
    tokenNameRemap["TOKEN_PTR_OP"] = "\'->\'";
    tokenNameRemap["$end"] = "end of file";
}


inline int ispcRand() {
#ifdef ISPC_HOST_IS_WINDOWS
    return rand();
#else
    return lrand48();
#endif
}

#define RT

%}

%option nounput
%option noyywrap
%option nounistd
%option reentrant
%option extra-type="FlexLexerData*"

WHITESPACE [ \t\r]+
INT_NUMBER (([0-9]+)|(0x[0-9a-fA-F]+)|(0b[01]+))[uUlL]*[kMG]?[uUlL]*
INT_NUMBER_DOTDOTDOT (([0-9]+)|(0x[0-9a-fA-F]+)|(0b[01]+))[uUlL]*[kMG]?[uUlL]*\.\.\.
FLOAT_NUMBER (([0-9]+|(([0-9]+\.[0-9]*[fF]?)|(\.[0-9]+)))([eE][-+]?[0-9]+)?[fF]?)
HEX_FLOAT_NUMBER (0x[01](\.[0-9a-fA-F]*)?p[-+]?[0-9]+[fF]?)
FORTRAN_DOUBLE_NUMBER (([0-9]+\.[0-9]*[dD])|([0-9]+\.[0-9]*[dD][-+]?[0-9]+)|([0-9]+[dD][-+]?[0-9]+)|(\.[0-9]*[dD][-+]?[0-9]+))



IDENT [a-zA-Z_][a-zA-Z_0-9]*
ZO_SWIZZLE ([01]+[w-z]+)+|([01]+[rgba]+)+|([01]+[uv]+)+

%%
"/*"            { lCComment(yyextra, yyscanner); }
"//"            { lCppComment(yyscanner, &yyextra->yylloc); }
"#pragma" {
    if (lConsumePragma(yyextra, yyscanner)) {
        return TOKEN_PRAGMA;
    }
}


__assert { RT; return TOKEN_ASSERT; }
bool { RT; return TOKEN_BOOL; }
break { RT; return TOKEN_BREAK; }
case { RT; return TOKEN_CASE; }
cbreak { RT; Warning(yyextra, "\"cbreak\" is deprecated. Use \"break\"."); return TOKEN_BREAK; }
ccontinue { RT; Warning(yyextra, "\"ccontinue\" is deprecated. Use \"continue\"."); return TOKEN_CONTINUE; }
cdo { RT; return TOKEN_CDO; }
cfor { RT; return TOKEN_CFOR; }
cif { RT; return TOKEN_CIF; }
cwhile { RT; return TOKEN_CWHILE; }
const { RT; return TOKEN_CONST; }
continue { RT; return TOKEN_CONTINUE; }
creturn { RT; Warning(yyextra, "\"creturn\" is deprecated. Use \"return\"."); return TOKEN_RETURN; }
__declspec { RT; return TOKEN_DECLSPEC; }
default { RT; return TOKEN_DEFAULT; }
do { RT; return TOKEN_DO; }
delete { RT; return TOKEN_DELETE; }
delete\[\] { RT; return TOKEN_DELETE; }
double { RT; return TOKEN_DOUBLE; }
else { RT; return TOKEN_ELSE; }
enum { RT; return TOKEN_ENUM; }
export { RT; return TOKEN_EXPORT; }
extern { RT; return TOKEN_EXTERN; }
false { RT; return TOKEN_FALSE; }
float { RT; return TOKEN_FLOAT; }
for { RT; return TOKEN_FOR; }
foreach { RT; return TOKEN_FOREACH; }
foreach_active { RT; return TOKEN_FOREACH_ACTIVE; }
foreach_tiled { RT; return TOKEN_FOREACH_TILED; }
foreach_unique { RT; return TOKEN_FOREACH_UNIQUE; }
goto { RT; return TOKEN_GOTO; }
if { RT; return TOKEN_IF; }
in { RT; return TOKEN_IN; }
inline { RT; return TOKEN_INLINE; }
noinline { RT; return TOKEN_NOINLINE; }
__vectorcall { RT; return TOKEN_VECTORCALL; }
int { RT; return TOKEN_INT; }
uint { RT; return TOKEN_UINT; }
int8 { RT; return TOKEN_INT8; }
uint8 { RT; return TOKEN_UINT8; }
int16 { RT; return TOKEN_INT16; }
uint16 { RT; return TOKEN_UINT16; }
int32 { RT; return TOKEN_INT; }
uint32 { RT; return TOKEN_UINT; }
int64 { RT; return TOKEN_INT64; }
uint64 { RT; return TOKEN_UINT64; }
launch { RT; return TOKEN_LAUNCH; }
new { RT; return TOKEN_NEW; }
NULL { RT; return TOKEN_NULL; }
print { RT; return TOKEN_PRINT; }
return { RT; return TOKEN_RETURN; }
soa { RT; return TOKEN_SOA; }
signed { RT; return TOKEN_SIGNED; }
sizeof { RT; return TOKEN_SIZEOF; }
static { RT; return TOKEN_STATIC; }
struct { RT; return TOKEN_STRUCT; }
switch { RT; return TOKEN_SWITCH; }
sync { RT; return TOKEN_SYNC; }
task { RT; return TOKEN_TASK; }
true { RT; return TOKEN_TRUE; }
typedef { RT; return TOKEN_TYPEDEF; }
uniform { RT; return TOKEN_UNIFORM; }
unmasked { RT; return TOKEN_UNMASKED; }
unsigned { RT; return TOKEN_UNSIGNED; }
varying { RT; return TOKEN_VARYING; }
void { RT; return TOKEN_VOID; }
while { RT; return TOKEN_WHILE; }
\"C\" { RT; return TOKEN_STRING_C_LITERAL; }
\.\.\. { RT; return TOKEN_DOTDOTDOT; }

"operator*"  { return TOKEN_IDENTIFIER; }
"operator+"  { return TOKEN_IDENTIFIER; }
"operator-"  { return TOKEN_IDENTIFIER; }
"operator<<" { return TOKEN_IDENTIFIER; }
"operator>>" { return TOKEN_IDENTIFIER; }
"operator/" { return TOKEN_IDENTIFIER; }
"operator%" { return TOKEN_IDENTIFIER; }

L?\"(\\.|[^\\"])*\" { lStringConst(yyextra, &yyextra->yylval, &yyextra->yylloc, yytext); return TOKEN_STRING_LITERAL; }

{IDENT} {
    RT;
    /* We have an identifier--is it a type name or an identifier?
       The symbol table will straighten us out... */
    yyextra->yylval.stringVal = new std::string(yytext);
    if (yyextra->symbolTable->IsType(yytext))
        return TOKEN_TYPE_NAME;
    else
        return TOKEN_IDENTIFIER;
}

{INT_NUMBER} {
    RT;
    return lParseInteger(yyextra, yytext, false);
}

{INT_NUMBER_DOTDOTDOT} {
    RT;
    return lParseInteger(yyextra, yytext, true);
}

{FORTRAN_DOUBLE_NUMBER} {
    RT;
    {
      int i = 0;
      while (yytext[i] != 'd' && yytext[i] != 'D') i++;
      yytext[i] = 'E';
    }
    yyextra->yylval.doubleVal = atof(yytext);
    return TOKEN_DOUBLE_CONSTANT;
}


{FLOAT_NUMBER} {
    RT;
    yyextra->yylval.floatVal = (float)atof(yytext);
    return TOKEN_FLOAT_CONSTANT;
}

{HEX_FLOAT_NUMBER} {
    RT;
    yyextra->yylval.floatVal = (float)lParseHexFloat(yytext);
    return TOKEN_FLOAT_CONSTANT;
}



"++" { RT; return TOKEN_INC_OP; }
"--" { RT; return TOKEN_DEC_OP; }
"<<" { RT; return TOKEN_LEFT_OP; }
">>" { RT; return TOKEN_RIGHT_OP; }
"<=" { RT; return TOKEN_LE_OP; }
">=" { RT; return TOKEN_GE_OP; }
"==" { RT; return TOKEN_EQ_OP; }
"!=" { RT; return TOKEN_NE_OP; }
"&&" { RT; return TOKEN_AND_OP; }
"||" { RT; return TOKEN_OR_OP; }
"*=" { RT; return TOKEN_MUL_ASSIGN; }
"/=" { RT; return TOKEN_DIV_ASSIGN; }
"%=" { RT; return TOKEN_MOD_ASSIGN; }
"+=" { RT; return TOKEN_ADD_ASSIGN; }
"-=" { RT; return TOKEN_SUB_ASSIGN; }
"<<=" { RT; return TOKEN_LEFT_ASSIGN; }
">>=" { RT; return TOKEN_RIGHT_ASSIGN; }
"&=" { RT; return TOKEN_AND_ASSIGN; }
"^=" { RT; return TOKEN_XOR_ASSIGN; }
"|=" { RT; return TOKEN_OR_ASSIGN; }
"->" { RT; return TOKEN_PTR_OP; }
";"             { RT; return ';'; }
("{"|"<%")      { RT; return '{'; }
("}"|"%>")      { RT; return '}'; }
","             { RT; return ','; }
":"             { RT; return ':'; }
"="             { RT; return '='; }
"("             { RT; return '('; }
")"             { RT; return ')'; }
("["|"<:")      { RT; return '['; }
("]"|":>")      { RT; return ']'; }
"."             { RT; return '.'; }
"&"             { RT; return '&'; }
"!"             { RT; return '!'; }
"~"             { RT; return '~'; }
"-"             { RT; return '-'; }
"+"             { RT; return '+'; }
"*"             { RT; return '*'; }
"/"             { RT; return '/'; }
"%"             { RT; return '%'; }
"<"             { RT; return '<'; }
">"             { RT; return '>'; }
"^"             { RT; return '^'; }
"|"             { RT; return '|'; }
"?"             { RT; return '?'; }

{WHITESPACE} { }

\n {
    yyextra->yylloc.last_line++;
    yyextra->yylloc.last_column = 1;
}

#(line)?[ ][0-9]+[ ]\"(\\.|[^\\"])*\"[^\n]* {
    lHandleCppHash(yyextra, yytext);
}

. {
    Error(yyextra, "Illegal character: %c (0x%x)", yytext[0], int(yytext[0]));
    YY_USER_ACTION
}

%%

/*short { return TOKEN_SHORT; }*/
/*long { return TOKEN_LONG; }*/
/*signed { return TOKEN_SIGNED; }*/
/*volatile { return TOKEN_VOLATILE; }*/
/*"long"[ \t\v\f\n]+"long" { return TOKEN_LONGLONG; }*/
/*union { return TOKEN_UNION; }*/
/*"..." { return TOKEN_ELLIPSIS; }*/

/** Return the integer version of a binary constant from a string.
 */
static uint64_t
lParseBinary(FlexLexerData *lexerData, const char *ptr, SourcePos pos, char **endPtr) {
    uint64_t val = 0;
    bool warned = false;

    while (*ptr == '0' || *ptr == '1') {
        if ((val & (((int64_t)1)<<63)) && warned == false) {
            // We're about to shift out a set bit
            Warning(lexerData, "Can't represent binary constant with a 64-bit integer type");
            warned = true;
        }

        val = (val << 1) | (*ptr == '0' ? 0 : 1);
        ++ptr;
    }
    *endPtr = (char *)ptr;
    return val;
}


static int
lParseInteger(FlexLexerData *lexerData, const char *tokenText, bool dotdotdot) {

    auto &yylval = lexerData->yylval;

    int ls = 0, us = 0;

    char *endPtr = NULL;
    if (tokenText[0] == '0' && tokenText[1] == 'b')
        yylval.intVal = lParseBinary(lexerData, tokenText+2, lexerData->yylloc, &endPtr);
    else {
#if defined(ISPC_HOST_IS_WINDOWS) && !defined(__MINGW32__)
        yylval.intVal = _strtoui64(tokenText, &endPtr, 0);
#else
        // FIXME: should use strtouq and then issue an error if we can't
        // fit into 64 bits...
        yylval.intVal = strtoull(tokenText, &endPtr, 0);
#endif
    }

    bool kilo = false, mega = false, giga = false;
    for (; *endPtr; endPtr++) {
        if (*endPtr == 'k')
            kilo = true;
        else if (*endPtr == 'M')
            mega = true;
        else if (*endPtr == 'G')
            giga = true;
        else if (*endPtr == 'l' || *endPtr == 'L')
            ls++;
        else if (*endPtr == 'u' || *endPtr == 'U')
            us++;
        else
            Assert(dotdotdot && *endPtr == '.');
    }
    if (kilo)
        yylval.intVal *= 1024;
    if (mega)
        yylval.intVal *= 1024*1024;
    if (giga)
        yylval.intVal *= 1024*1024*1024;

    if (dotdotdot) {
        if (ls >= 2)
            return us ? TOKEN_UINT64DOTDOTDOT_CONSTANT : TOKEN_INT64DOTDOTDOT_CONSTANT;
        else if (ls == 1)
            return us ? TOKEN_UINT32DOTDOTDOT_CONSTANT : TOKEN_INT32DOTDOTDOT_CONSTANT;

        // See if we can fit this into a 32-bit integer...
        if ((yylval.intVal & 0xffffffff) == yylval.intVal)
            return us ? TOKEN_UINT32DOTDOTDOT_CONSTANT : TOKEN_INT32DOTDOTDOT_CONSTANT;
        else
            return us ? TOKEN_UINT64DOTDOTDOT_CONSTANT : TOKEN_INT64DOTDOTDOT_CONSTANT;
    }
    else {
        if (ls >= 2)
            return us ? TOKEN_UINT64_CONSTANT : TOKEN_INT64_CONSTANT;
        else if (ls == 1)
            return us ? TOKEN_UINT32_CONSTANT : TOKEN_INT32_CONSTANT;
        else if (us) {
            // u suffix only
            if (yylval.intVal <= 0xffffffffL)
                return TOKEN_UINT32_CONSTANT;
            else
                return TOKEN_UINT64_CONSTANT;
        }
        else {
            // No u or l suffix
            // If we're compiling to an 8-bit mask target and the constant
            // fits into 8 bits, return an 8-bit int.
            if (lexerData->dataTypeWidth == 8) {
                if (yylval.intVal <= 0x7fULL)
                    return TOKEN_INT8_CONSTANT;
                else if (yylval.intVal <= 0xffULL)
                    return TOKEN_UINT8_CONSTANT;
            }
            // And similarly for 16-bit masks and constants
            if (lexerData->dataTypeWidth == 16) {
                if (yylval.intVal <= 0x7fffULL)
                    return TOKEN_INT16_CONSTANT;
                else if (yylval.intVal <= 0xffffULL)
                    return TOKEN_UINT16_CONSTANT;
            }
            // Otherwise, see if we can fit this into a 32-bit integer...
            if (yylval.intVal <= 0x7fffffffULL)
                return TOKEN_INT32_CONSTANT;
            else if (yylval.intVal <= 0xffffffffULL)
                return TOKEN_UINT32_CONSTANT;
            else if (yylval.intVal <= 0x7fffffffffffffffULL)
                return TOKEN_INT64_CONSTANT;
            else
                return TOKEN_UINT64_CONSTANT;
        }
    }
}


/** Handle a C-style comment in the source.
 */
static void
lCComment(FlexLexerData* lexerData, yyscan_t scanner) {
    char c, prev = 0;

    auto pos = &lexerData->yylloc;

    while ((c = yyinput(scanner)) != 0) {
        ++pos->last_column;

        if (c == '\n') {
            pos->last_line++;
            pos->last_column = 1;
        }
        if (c == '/' && prev == '*')
            return;
        prev = c;
    }
    Error(lexerData, "unterminated comment");
}

static void lNextValidChar(SourcePos *pos, char const*&currChar) {
    while ((*currChar == ' ') || (*currChar == '\t') || (*currChar == '\r')) {
        ++pos->last_column;
        currChar++;
    }
}

/** Handle pragma directive to unroll loops.
*/
static void lPragmaUnroll(FlexLexerData* lexerData, std::string fromUserReq, bool isNounroll) {

    YYSTYPE *yylval = &lexerData->yylval;

    SourcePos *pos = &lexerData->yylloc;

    const char *currChar = fromUserReq.data();
    yylval->pragmaAttributes = new PragmaAttributes();
    yylval->pragmaAttributes->aType = PragmaAttributes::AttributeType::pragmaloop;
    int count = -1;

    lNextValidChar(pos, currChar);

    if (isNounroll) {
        if (*currChar == '\n') {
            yylval->pragmaAttributes->unrollType = PragmaUnrollType::nounroll;
            pos->last_column = 1;
            pos->last_line++;
            return;
        }
        pos->last_column = 1;
        pos->last_line++;
        Warning(lexerData, "extra tokens at end of '#pragma nounroll'.");
        return;

    }

    if (*currChar == '\n') {
        yylval->pragmaAttributes->unrollType = PragmaUnrollType::unroll;
        pos->last_column = 1;
        pos->last_line++;
        return;
    }

    bool popPar = false;
    if (*currChar == '(') {
        popPar = true;
        currChar++;
        ++pos->last_column;
    }

    char *endPtr = NULL;
#if defined(ISPC_HOST_IS_WINDOWS) && !defined(__MINGW32__)
    count = _strtoui64(currChar, &endPtr, 0);
#else
    // FIXME: should use strtouq and then issue an error if we can't
    // fit into 64 bits...
    count = strtoull(currChar, &endPtr, 0);
#endif

    if((count == 0) && (endPtr != currChar)){
        Error(lexerData, "'#pragma unroll()' invalid value '0'; must be positive.");
    }

    lNextValidChar(pos, const_cast<const char*&>(endPtr));

    if (popPar == true) {
        if (*endPtr == ')') {
            ++pos->last_column;
            endPtr++;
            lNextValidChar(pos, const_cast<const char*&>(endPtr));
        }
        else {
            Error(lexerData, "Incomplete '#pragma unroll()' : expected ')'.");
        }
    }

    yylval->pragmaAttributes->unrollType = PragmaUnrollType::count;
    yylval->pragmaAttributes->count = count;
    pos->last_line++;
    pos->last_column = 1;
}

/** Handle pragma directive to ignore warning.
*/
static void
lPragmaIgnoreWarning(FlexLexerData *lexerData, std::string fromUserReq) {

    SourcePos *pos = &lexerData->yylloc;

    std::string userReq;
    const char *currChar = fromUserReq.data();
    bool perfWarningOnly = false;
    lNextValidChar(pos, currChar);

    if (*currChar == '\n') {
        pos->last_column = 1;
        pos->last_line++;
        std::pair<int, std::string> key = std::pair<int, std::string>(pos->last_line, pos->name);
        lexerData->turnOffWarnings[key] = perfWarningOnly;
        return;
    }
    else if (*currChar == '(') {
        currChar++;
        lNextValidChar(pos, currChar);
        while (*currChar != 0 && *currChar != '\n' && *currChar != ' ' && *currChar != ')') {
            userReq += *currChar;
            currChar++;
            ++pos->last_column;
        }
        if ((*currChar == ' ') || (*currChar == ')')) {
            lNextValidChar(pos, currChar);
            if (*currChar == ')') {
                currChar++;
                ++pos->last_column;
                lNextValidChar(pos, currChar);
                if (*currChar == '\n') {
                    pos->last_column = 1;
                    pos->last_line++;
                    if (userReq.compare("perf") == 0) {
                        perfWarningOnly = true;
                        std::pair<int, std::string> key = std::pair<int, std::string>(pos->last_line, pos->name);
                        lexerData->turnOffWarnings[key] = perfWarningOnly;
                    }
                    else if (userReq.compare("all") == 0) {
                        std::pair<int, std::string> key = std::pair<int, std::string>(pos->last_line, pos->name);
                        lexerData->turnOffWarnings[key] = perfWarningOnly;
                    }
                    else {
                        Error(lexerData, "Incorrect argument for '#pragma ignore warning()'.");
                    }
                    return;
                }
            }
        }
        else if (*currChar == '\n') {
            Error(lexerData, "Incomplete '#pragma ignore warning()' : expected ')'.");
            pos->last_column = 1;
            pos->last_line++;
            return;
        }
    }
    Error(lexerData, "Undefined #pragma.");
}

/** Consume line starting with '#pragma' and decide on next action based on
 * directive.
 */
static bool lConsumePragma(FlexLexerData *lexerData, yyscan_t scanner) {

    SourcePos *pos = &lexerData->yylloc;


    char c;
    std::string userReq;
    do {
        c = yyinput(scanner);
        ++pos->last_column;
    } while ((c == ' ') || (c == '\t') || (c == '\r'));
    if (c == '\n') {
        // Ignore pragma since - directive provided.
        pos->last_column = 1;
        pos->last_line++;
        return false;
    }

    while (c != '\n') {
        userReq += c;
        c = yyinput(scanner);
    }
    userReq += c;
    std::string loopUnroll("unroll"), loopNounroll("nounroll"), ignoreWarning("ignore warning");
    if (loopUnroll == userReq.substr(0, loopUnroll.size())) {
        pos->last_column += loopUnroll.size();
        lPragmaUnroll(lexerData, userReq.erase(0, loopUnroll.size()), false);
        return true;
    }
    else if (loopNounroll == userReq.substr(0, loopNounroll.size())) {
        pos->last_column += loopNounroll.size();
        lPragmaUnroll(lexerData, userReq.erase(0, loopNounroll.size()), true);
        return true;
    }
    else if (ignoreWarning == userReq.substr(0, ignoreWarning.size())) {
        pos->last_column += ignoreWarning.size();
        lPragmaIgnoreWarning(lexerData, userReq.erase(0, ignoreWarning.size()));
        return false;
    }

    // Ignore pragma : invalid directive provided.
    Warning(lexerData, "unknown pragma ignored.");
    return false;
}

/** Handle a C++-style comment--eat everything up until the end of the line.
 */
static void
lCppComment(yyscan_t scanner, SourcePos *pos) {
    char c;
    do {
        c = yyinput(scanner);
    } while (c != 0 && c != '\n');
    if (c == '\n') {
        pos->last_line++;
        pos->last_column = 1;
    }
}

/** Handle a line that starts with a # character; this should be something
    left behind by the preprocessor indicating the source file/line
    that our current position corresponds to.
 */
static void lHandleCppHash(FlexLexerData* lexerData, char *tokenText) {

    SourcePos *pos = &lexerData->yylloc;

    char *ptr, *src;

    // Advance past the opening stuff on the line.
    Assert(tokenText[0] == '#');

    if (tokenText[1] == ' ')
        // On Linux/OSX, the preprocessor gives us lines like
        // # 1234 "foo.c"
        ptr = &tokenText[2];
    else {
        // On windows, cl.exe's preprocessor gives us lines of the form:
        // #line 1234 "foo.c"
        Assert(!strncmp(&tokenText[1], "line ", 5));
        ptr = &tokenText[6];
    }

    // Now we can set the line number based on the integer in the string
    // that ptr is pointing at.
    pos->last_line = strtol(ptr, &src, 10) - 1;
    pos->last_column = 1;
    // Make sure that the character after the integer is a space and that
    // then we have open quotes
    Assert(src != ptr && src[0] == ' ' && src[1] == '"');
    src += 2;

    // And the filename is everything up until the closing quotes
    std::string filename;
    while (*src != '"') {
        Assert(*src && *src != '\n');
        filename.push_back(*src);
        ++src;
    }
    pos->name = strdup(filename.c_str());
    lexerData->RegisterDependency(filename);
}


/** Given a pointer to a position in a string, return the character that it
    represents, accounting for the escape characters supported in string
    constants.  (i.e. given the literal string "\\", return the character
    '/').  The return value is the new position in the string and the
    decoded character is returned in *pChar.
*/
static char *
lEscapeChar(FlexLexerData *lexerData, char *str, char *pChar, SourcePos *pos)
{
    if (*str != '\\') {
        *pChar = *str;
    }
    else {
        char *tail;
        ++str;
        switch (*str) {
        case '\'': *pChar = '\''; break;
        case '\"': *pChar = '\"'; break;
        case '?':  *pChar = '\?'; break;
        case '\\': *pChar = '\\'; break;
        case 'a':  *pChar = '\a'; break;
        case 'b':  *pChar = '\b'; break;
        case 'f':  *pChar = '\f'; break;
        case 'n':  *pChar = '\n'; break;
        case 'r':  *pChar = '\r'; break;
        case 't':  *pChar = '\t'; break;
        case 'v':  *pChar = '\v'; break;
        // octal constants \012
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7':
            *pChar = (char)strtol(str, &tail, 8);
            str = tail - 1;
            break;
        // hexidecimal constant \xff
        case 'x':
            *pChar = (char)strtol(str, &tail, 16);
            str = tail - 1;
            break;
        default:
            Error(lexerData, "Bad character escape sequence: '%s'.", str);
            break;
        }
    }
    ++str;
    return str;
}


/** Parse a string constant in the source file.  For each character in the
    string, handle any escaped characters with lEscapeChar() and keep eating
    characters until we come to the closing quote.
*/
static void
lStringConst(FlexLexerData* lexerData, YYSTYPE *yylval, SourcePos *pos, char *tokenText)
{
    char *p;
    std::string str;
    p = strchr(tokenText, '"') + 1;
    if (p == NULL)
       return;

    while (*p != '\"') {
       char cval = '\0';
       p = lEscapeChar(lexerData, p, &cval, pos);
       str.push_back(cval);
    }
    yylval->stringVal = new std::string(str);
}


/** Compute the value 2^n, where the exponent is given as an integer.
    There are more efficient ways to do this, for example by just slamming
    the bits into the appropriate bits of the double, but let's just do the
    obvious thing.
*/
static double
ipow2(int exponent) {
    if (exponent < 0)
        return 1. / ipow2(-exponent);

    double ret = 1.;
    while (exponent > 16) {
        ret *= 65536.;
        exponent -= 16;
    }
    while (exponent-- > 0)
        ret *= 2.;
    return ret;
}


/** Parse a hexadecimal-formatted floating-point number (C99 hex float
    constant-style).
*/
static double
lParseHexFloat(const char *ptr) {
    Assert(ptr != NULL);

    Assert(ptr[0] == '0' && ptr[1] == 'x');
    ptr += 2;

    // Start initializing the mantissa
    Assert(*ptr == '0' || *ptr == '1');
    double mantissa = (*ptr == '1') ? 1. : 0.;
    ++ptr;

    if (*ptr == '.') {
        // Is there a fraction part?  If so, the i'th digit we encounter
        // gives the 1/(16^i) component of the mantissa.
        ++ptr;

        double scale = 1. / 16.;
        // Keep going until we come to the 'p', which indicates that we've
        // come to the exponent
        while (*ptr != 'p') {
            // Figure out the raw value from 0-15
            int digit;
            if (*ptr >= '0' && *ptr <= '9')
                digit = *ptr - '0';
            else if (*ptr >= 'a' && *ptr <= 'f')
                digit = 10 + *ptr - 'a';
            else {
                Assert(*ptr >= 'A' && *ptr <= 'F');
                digit = 10 + *ptr - 'A';
            }

            // And add its contribution to the mantissa
            mantissa += scale * digit;
            scale /= 16.;
            ++ptr;
        }
    }
    else
        // If there's not a '.', then we better be going straight to the
        // exponent
        Assert(*ptr == 'p');

    ++ptr; // skip the 'p'

    // interestingly enough, the exponent is provided base 10..
    char* endptr = NULL;
    int exponent = (int)strtol(ptr, &endptr, 10);
    Assert(ptr != endptr);

    // Does stdlib exp2() guarantee exact results for integer n where can
    // be represented exactly as doubles?  I would hope so but am not sure,
    // so let's be sure.
    return mantissa * ipow2(exponent);
}