#include <gtest/gtest.h>

#include <ispc/lexer.h>

TEST(LexerTest, Base10Integer) {
    ispc::ISPCLexer<char> lexer;
    lexer.SetInput("15");
    auto result = lexer.Lex();
    EXPECT_EQ(result.error, ispc::LexicalError::None);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::Int32Constant);
}

TEST(LexerTest, Base10IntegerOverflow) {
    ispc::ISPCLexer<char> lexer;
    lexer.SetInput("18446744073709551616");
    auto result = lexer.Lex();
    EXPECT_EQ(result.error, ispc::LexicalError::IntegerOverflow);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::UInt64Constant);
}

TEST(LexerTest, Base10IntegerOverflow2) {
    ispc::ISPCLexer<char> lexer;
    lexer.SetInput("18446744073709551625");
    auto result = lexer.Lex();
    EXPECT_EQ(result.error, ispc::LexicalError::IntegerOverflow);
    EXPECT_EQ(result.token.GetKind(), ispc::TokenKind::UInt64Constant);
}
