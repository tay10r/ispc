// This program will read and input file and tokenize it with the two different
// lexer implementations. If the tokens produced by both lexers are different,
// then the program emits an error message and exits.

#include "lexer_impl.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

/// Used for parsing command line arguments.
class ArgStream final {
    int argc = 0;
    char **argv = nullptr;
    int index = 0;

  public:
    ArgStream(int argc, char **argv) : argc(argc), argv(argv) {}
    /// Indicates whether or not the current argument is a non-option.
    ///
    /// @return
    ///  True if the current argument is a non-option, false otherwise.
    bool AtNonOpt() const noexcept;
    /// Gets the current argument.
    ///
    /// @return
    ///  A pointer to the current argument. A null string is never
    ///  returned by this function.
    const char *GetCurrent() const noexcept;
    /// Skips the current argument.
    void Next() { index++; }
    /// Matches a long option. If the option is found, the argument
    /// index is incremented.
    ///
    /// @return
    ///  True if a match is made, false otherwise.
    bool MatchOpt(const char *longOpt);
    /// Attempts to match an integer value from the current argument. If a match
    /// is made, then the argument index is incremented.
    ///
    /// @param optName
    ///  If non-null then an error is reported if the argument isn't an integer.
    ///
    /// @return
    ///  True if a match is made, false otherwise.
    bool MatchValue(unsigned int &value, const char *optName);
    /// Returns the current argument to the caller and increments the argument
    /// index.
    const char *PopArg() noexcept { return argv[index++]; }
    /// Indicates whether or not the last argument was reached.
    bool AtEnd() const noexcept { return index >= argc; }
};

std::string CombineFiles(const std::vector<const char *> &inputPaths);

struct LexerResult final {
    /// The tokens printed to a string stream.
    std::string printedTokens;
    /// The time it took to scan the tokens, in terms of seconds.
    float duration;
};

LexerResult RunLexer(const std::string &source, std::unique_ptr<LexerImpl> lexer);

bool RunTest(const std::string &input, std::size_t bits = 32);

void PrintResults(const std::string &expected, const std::string &actual);

} // namespace

int main(int argc, char **argv) {

    bool testAllLaneWidths = false;

    bool randomInput = false;

    unsigned int randomSeed = std::chrono::system_clock::now().time_since_epoch().count();

    std::vector<const char *> inputPaths;

    ArgStream argStream(argc - 1, argv + 1);

    while (!argStream.AtEnd()) {
        if (argStream.MatchOpt("random-input")) {
            randomInput = true;
        } else if (argStream.MatchOpt("random-seed")) {
            if (!argStream.MatchValue(randomSeed, "random-seed"))
                return EXIT_FAILURE;
        } else if (argStream.MatchOpt("test-all-lane-widths")) {
            testAllLaneWidths = true;
        } else if (argStream.AtNonOpt()) {
            inputPaths.emplace_back(argStream.PopArg());
        } else {
            std::cerr << "Unknown option '" << argStream.PopArg() << "'" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (inputPaths.empty() && !randomInput) {
        std::cerr << "No input files specified." << std::endl;
        return EXIT_FAILURE;
    }

    auto input = CombineFiles(inputPaths);

    if (randomInput) {

        std::cout << "Using random seed: " << randomSeed << std::endl;

        std::mt19937 rng(randomSeed);

        std::uniform_int_distribution<char> charDist(-128, 127);

        std::uniform_int_distribution<size_t> lengthDist(0, 1024 * 1024);

        auto length = lengthDist(rng);

        std::cout << "Generating " << length << " bytes of random input." << std::endl;

        auto oldLength = input.size();

        input.resize(input.size() + length);

        for (decltype(length) i = 0; i < length; i++)
            input[oldLength + i] = charDist(rng);
    }

    if (testAllLaneWidths) {
        for (std::size_t bits = 8; bits < 128; bits *= 2) {
            if (!RunTest(input, bits))
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    } else {
        return RunTest(input) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

namespace {

bool RunTest(const std::string &input, std::size_t laneBits) {

    auto prevResult = RunLexer(input, LexerImpl::MakeFlexLexer(laneBits));
    auto nextResult = RunLexer(input, LexerImpl::MakeNonFlexLexer(laneBits));

    if (nextResult.printedTokens != prevResult.printedTokens) {
        std::cerr << "Test failed (lane bits : " << laneBits << ")." << std::endl;
        PrintResults(prevResult.printedTokens, nextResult.printedTokens);
        return false;
    }

    float speedupFactor = nextResult.duration / prevResult.duration;

    std::cout << "Test passed (speed up factor: " << speedupFactor << ")" << std::endl;

    return true;
}

LexerResult RunLexer(const std::string &source, std::unique_ptr<LexerImpl> lexer) {

    lexer->SetInput(source);

    auto startTime = std::chrono::high_resolution_clock::now();

    lexer->ScanTokens();

    auto stopTime = std::chrono::high_resolution_clock::now();

    auto delta = stopTime - startTime;

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

    std::ostringstream printStream;

    lexer->PrintTokens(printStream);

    return {printStream.str(), duration.count() / 1000.0f};
}

void PrintResults(const std::string &expected, const std::string &actual) {

    std::cout << "  expected:" << std::endl;

    std::istringstream eStream(expected);

    while (eStream) {

        std::string line;

        if (std::getline(eStream, line))
            std::cout << "  | " << line << std::endl;
    }

    std::cout << "  actual:" << std::endl;

    std::istringstream aStream(actual);

    while (aStream) {

        std::string line;

        if (std::getline(aStream, line))
            std::cout << "  | " << line << std::endl;
    }
}

const char *ArgStream::GetCurrent() const noexcept {
    if (AtEnd())
        return "";
    else
        return argv[index];
}

bool ArgStream::AtNonOpt() const noexcept { return GetCurrent()[0] != '-'; }

bool ArgStream::MatchOpt(const char *longOpt) {
    const char *arg = GetCurrent();

    if ((arg[0] == '-') && (arg[1] == '-') && (std::strcmp(arg + 2, longOpt) == 0)) {
        Next();
        return true;
    } else {
        return false;
    }
}

bool ArgStream::MatchValue(unsigned int &value, const char *optName) {
    const char *arg = GetCurrent();

    if (std::sscanf(arg, "%u", &value) != 1) {
        if (optName)
            std::cerr << "Missing value for '" << optName << "'" << std::endl;
        return false;
    }

    Next();

    return true;
}

std::string CombineFiles(const std::vector<const char *> &inputPaths) {

    std::ostringstream inputStream;

    for (const auto &inputPath : inputPaths) {

        std::ifstream file(inputPath, std::ios::binary | std::ios::in);

        if (!file.good()) {
            std::cerr << "Failed to open '" << inputPath << '"' << std::endl;
            std::exit(EXIT_FAILURE);
        }

        inputStream << file.rdbuf();
    }

    return inputStream.str();
}

} // namespace
