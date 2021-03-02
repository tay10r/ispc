#include <gtest/gtest.h>

#include <ispc/lexer.h>

TEST(LexerTest, Base10Integer) {
    ispc::ISPCLexer<char> lexer;
    lexer.SetInput("15");
    auto result = lexer.Lex();
    EXPECT_EQ(result.diagnostics.empty(), true);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::Int32Constant);
}

TEST(LexerTest, Base10IntegerOverflow) {
    ispc::ISPCLexer<char> lexer;
    lexer.SetInput("18446744073709551616");
    auto result = lexer.Lex();
    EXPECT_EQ(result.diagnostics.size(), 0);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::UInt64Constant);
    EXPECT_EQ(result.token.GetData().asInt, std::numeric_limits<std::uint64_t>::max());
}

TEST(LexerTest, Base10IntegerOverflowError) {
    ispc::ISPCLexer<char> lexer;
    lexer.GetOptions().warnIntegerOverflow = true;
    lexer.SetInput("18446744073709551616");
    auto result = lexer.Lex();
    ASSERT_EQ(result.diagnostics.size(), 1);
    EXPECT_EQ(result.diagnostics[0].id, ispc::DiagnosticID::IntegerOverflow);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::UInt64Constant);
    EXPECT_EQ(result.token.GetData().asInt, std::numeric_limits<std::uint64_t>::max());
}

TEST(LexerTest, Base10IntegerOverflow2) {
    ispc::ISPCLexer<char> lexer;
    lexer.GetOptions().warnIntegerOverflow = true;
    lexer.SetInput("18446744073709551625");
    auto result = lexer.Lex();
    ASSERT_EQ(result.diagnostics.size(), 1);
    EXPECT_EQ(result.diagnostics[0].id, ispc::DiagnosticID::IntegerOverflow);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::UInt64Constant);
    EXPECT_EQ(result.token.GetData().asInt, std::numeric_limits<std::uint64_t>::max());
}
