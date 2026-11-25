#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "args.hpp"
#include "commands.hpp"
#include "file_format.hpp"

namespace covarray::cli {

namespace {

// A file under the system temp directory, written in the constructor and
// removed in the destructor, so each test gets its own real file on disk
// without leaking it afterward. The name includes the test's own address
// so concurrent tests never collide.
class TempFile {
  public:
    TempFile(std::string_view extension, std::string_view content) {
        path_ = std::filesystem::temp_directory_path() /
                std::format("covarray_cli_test_{}.{}", static_cast<const void*>(this), extension);
        std::ofstream file(path_, std::ios::binary);
        file << content;
    }

    ~TempFile() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    TempFile(const TempFile&) = delete;
    auto operator=(const TempFile&) -> TempFile& = delete;

    [[nodiscard]] auto path() const -> std::string {
        return path_.string();
    }

  private:
    std::filesystem::path path_;
};

constexpr std::string_view fully_covered_text = "4 3 2^3 2\n0 0 0\n0 1 1\n1 0 1\n1 1 0\n";
constexpr std::string_view truncated_text = "3 3 2^3 2\n0 0 0\n0 1 1\n1 0 1\n";

} // namespace

TEST(CliFileFormat, DispatchesByExtension) {
    const TempFile txt("txt", fully_covered_text);
    const TempFile csv("csv", "4,3,2^3,2\n0,0,0\n0,1,1\n1,0,1\n1,1,0\n");
    const TempFile json(
        "json", R"({"n":4,"k":3,"t":2,"levels":[2,2,2],"rows":[[0,0,0],[0,1,1],[1,0,1],[1,1,0]]})"
    );

    for (const auto& path : {txt.path(), csv.path(), json.path()}) {
        const auto parsed = read_covering_array_file(path);
        ASSERT_TRUE(parsed.has_value()) << path;
        EXPECT_EQ(parsed->array.rows(), 4);
        EXPECT_EQ(parsed->spec.columns(), 3);
        EXPECT_EQ(parsed->spec.strength(), 2);
    }
}

TEST(CliFileFormat, RejectsAnUnrecognizedExtension) {
    const TempFile file("weird", "anything");
    const auto parsed = read_covering_array_file(file.path());
    ASSERT_FALSE(parsed.has_value());
    EXPECT_NE(parsed.error().find("unrecognized file extension"), std::string::npos);
}

TEST(CliFileFormat, RejectsAnUnreadableFile) {
    const auto parsed = read_covering_array_file("/nonexistent/path/does_not_exist.txt");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_NE(parsed.error().find("cannot read file"), std::string::npos);
}

TEST(CliFileFormat, SurfacesTheUnderlyingReaderMessageOnMalformedContent) {
    const TempFile file("txt", "not a valid parameter line\n");
    const auto parsed = read_covering_array_file(file.path());
    ASSERT_FALSE(parsed.has_value());
    EXPECT_FALSE(parsed.error().empty());
}

TEST(CliCommands, VerifyReportsFullCoverage) {
    const TempFile file("txt", fully_covered_text);
    const auto result = run_verify(file.path(), VerifyOptions{});
    EXPECT_EQ(result.code, ExitCode::success);
    EXPECT_NE(result.message.find("100.0000%"), std::string::npos);
    EXPECT_NE(result.message.find("fully covering"), std::string::npos);
}

TEST(CliCommands, VerifyReportsMissingTuplesForATruncatedArray) {
    const TempFile file("txt", truncated_text);
    const auto result = run_verify(file.path(), VerifyOptions{});
    EXPECT_EQ(result.code, ExitCode::not_covering);
    EXPECT_NE(result.message.find("75.0000%"), std::string::npos);
    EXPECT_NE(result.message.find("missing 3 tuple(s)"), std::string::npos);
}

TEST(CliCommands, VerifyStrengthOverrideChecksADifferentStrengthThanTheFile) {
    const TempFile file("txt", fully_covered_text);
    // The file declares t=2 (fully covering); overridden to t=3, the same
    // 4 rows can cover at most 4 of the 8 possible triples.
    VerifyOptions options;
    options.strength = 3U;
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::not_covering);
    EXPECT_NE(result.message.find("t=3"), std::string::npos);
    EXPECT_NE(result.message.find("50.0000%"), std::string::npos);
}

TEST(CliCommands, VerifyRejectsAStrengthOverrideThatExceedsColumnCount) {
    const TempFile file("txt", fully_covered_text);
    VerifyOptions options;
    options.strength = 99U;
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::usage_error);
    EXPECT_NE(result.message.find("error:"), std::string::npos);
}

TEST(CliCommands, VerifyReportsAFileErrorForAMissingFile) {
    const auto result = run_verify("/nonexistent/path/does_not_exist.txt", VerifyOptions{});
    EXPECT_EQ(result.code, ExitCode::file_error);
}

TEST(CliCommands, VerifyFastReportsFullCoverage) {
    const TempFile file("txt", fully_covered_text);
    VerifyOptions options;
    options.fast = true;
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::success);
    EXPECT_NE(result.message.find("fully covering (fast check)"), std::string::npos);
}

TEST(CliCommands, VerifyFastReportsOnlyTheFirstMissingTupleForATruncatedArray) {
    const TempFile file("txt", truncated_text);
    VerifyOptions options;
    options.fast = true;
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::not_covering);
    EXPECT_NE(result.message.find("not covering (fast check)"), std::string::npos);
    EXPECT_NE(result.message.find("missing values"), std::string::npos);
    // Only one tuple is ever reported in fast mode, unlike the "missing N
    // tuple(s)" summary the full enumeration prints.
    EXPECT_EQ(result.message.find("tuple(s)"), std::string::npos);
}

TEST(CliCommands, VerifyWritesEveryMissingTupleToTheRequestedFile) {
    const TempFile file("txt", truncated_text);
    const auto report_path =
        std::filesystem::temp_directory_path() /
        std::format("covarray_cli_test_report_{}.txt", static_cast<const void*>(&file));
    VerifyOptions options;
    options.missing_tuples_path = report_path.string();
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::not_covering);
    EXPECT_NE(result.message.find(report_path.string()), std::string::npos);

    std::ifstream report(report_path);
    ASSERT_TRUE(report.is_open());
    const std::string contents(
        (std::istreambuf_iterator<char>(report)), std::istreambuf_iterator<char>()
    );
    EXPECT_NE(contents.find("missing=3"), std::string::npos);
    EXPECT_NE(contents.find("combination_rank"), std::string::npos);

    std::error_code ignored;
    std::filesystem::remove(report_path, ignored);
}

TEST(CliCommands, VerifyRejectsCombiningFastWithMissingTuplesOutput) {
    const TempFile file("txt", fully_covered_text);
    VerifyOptions options;
    options.fast = true;
    options.missing_tuples_path = "ignored.txt";
    const auto result = run_verify(file.path(), options);
    EXPECT_EQ(result.code, ExitCode::usage_error);
    EXPECT_NE(result.message.find("--fast"), std::string::npos);
    EXPECT_NE(result.message.find("--missing-tuples"), std::string::npos);
}

TEST(CliCommands, InfoReportsRowsAndSpec) {
    const TempFile file("txt", fully_covered_text);
    const auto result = run_info(file.path());
    EXPECT_EQ(result.code, ExitCode::success);
    EXPECT_NE(result.message.find("N=4"), std::string::npos);
    EXPECT_NE(result.message.find("k=3 t=2 levels=[2,2,2]"), std::string::npos);
}

TEST(CliCommands, InfoReportsAFileErrorForAMissingFile) {
    const auto result = run_info("/nonexistent/path/does_not_exist.txt");
    EXPECT_EQ(result.code, ExitCode::file_error);
}

TEST(CliArgs, NoArgumentsIsAUsageError) {
    const auto outcome = parse_args({});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
    EXPECT_FALSE(outcome.args.has_value());
}

TEST(CliArgs, TopLevelHelpFlagPrintsHelpWithoutRequiringASubcommand) {
    for (const std::string flag : {"-h", "--help"}) {
        const auto outcome = parse_args({flag});
        EXPECT_EQ(outcome.status, ParseStatus::help);
        EXPECT_NE(outcome.text.find("Usage: covarray"), std::string::npos);
    }
}

TEST(CliArgs, UnrecognizedSubcommandIsAUsageError) {
    const auto outcome = parse_args({"bogus"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
    EXPECT_NE(outcome.text.find("unrecognized subcommand 'bogus'"), std::string::npos);
}

TEST(CliArgs, VerifyParsesTheFileAndStrengthOption) {
    for (const std::string flag : {"-t", "--strength"}) {
        const auto outcome = parse_args({"verify", "array.txt", flag, "3"});
        ASSERT_EQ(outcome.status, ParseStatus::run);
        ASSERT_TRUE(outcome.args.has_value());
        EXPECT_EQ(outcome.args->command, Subcommand::verify);
        EXPECT_EQ(outcome.args->file, "array.txt");
        ASSERT_TRUE(outcome.args->strength.has_value());
        EXPECT_EQ(*outcome.args->strength, 3U);
    }
}

TEST(CliArgs, VerifyWithoutStrengthLeavesItUnset) {
    const auto outcome = parse_args({"verify", "array.txt"});
    ASSERT_EQ(outcome.status, ParseStatus::run);
    ASSERT_TRUE(outcome.args.has_value());
    EXPECT_FALSE(outcome.args->strength.has_value());
}

TEST(CliArgs, VerifyWithoutAFileIsAUsageError) {
    const auto outcome = parse_args({"verify"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, VerifyRejectsANonIntegerStrength) {
    const auto outcome = parse_args({"verify", "array.txt", "-t", "abc"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
    EXPECT_NE(outcome.text.find("'abc'"), std::string::npos);
}

TEST(CliArgs, VerifyRejectsAStrengthFlagMissingItsValue) {
    const auto outcome = parse_args({"verify", "array.txt", "-t"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, VerifyRejectsAnUnrecognizedOption) {
    const auto outcome = parse_args({"verify", "array.txt", "--bogus"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, VerifyRejectsAnExtraPositionalArgument) {
    const auto outcome = parse_args({"verify", "array.txt", "extra.txt"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, VerifyHelpFlagPrintsHelpInsteadOfParsing) {
    const auto outcome = parse_args({"verify", "--help"});
    EXPECT_EQ(outcome.status, ParseStatus::help);
    EXPECT_NE(outcome.text.find("covarray verify"), std::string::npos);
}

TEST(CliArgs, VerifyParsesTheFastFlag) {
    const auto outcome = parse_args({"verify", "array.txt", "--fast"});
    ASSERT_EQ(outcome.status, ParseStatus::run);
    ASSERT_TRUE(outcome.args.has_value());
    EXPECT_TRUE(outcome.args->fast);
}

TEST(CliArgs, VerifyWithoutFastLeavesItUnset) {
    const auto outcome = parse_args({"verify", "array.txt"});
    ASSERT_EQ(outcome.status, ParseStatus::run);
    ASSERT_TRUE(outcome.args.has_value());
    EXPECT_FALSE(outcome.args->fast);
}

TEST(CliArgs, VerifyParsesTheMissingTuplesOption) {
    for (const std::string flag : {"-o", "--missing-tuples"}) {
        const auto outcome = parse_args({"verify", "array.txt", flag, "report.txt"});
        ASSERT_EQ(outcome.status, ParseStatus::run);
        ASSERT_TRUE(outcome.args.has_value());
        ASSERT_TRUE(outcome.args->missing_tuples_path.has_value());
        EXPECT_EQ(*outcome.args->missing_tuples_path, "report.txt");
    }
}

TEST(CliArgs, VerifyRejectsAMissingTuplesFlagWithoutAValue) {
    const auto outcome = parse_args({"verify", "array.txt", "-o"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, InfoDoesNotAcceptTheFastFlagOrMissingTuplesOption) {
    EXPECT_EQ(parse_args({"info", "array.txt", "--fast"}).status, ParseStatus::usage_error);
    EXPECT_EQ(
        parse_args({"info", "array.txt", "-o", "report.txt"}).status, ParseStatus::usage_error
    );
}

TEST(CliArgs, InfoParsesTheFile) {
    const auto outcome = parse_args({"info", "array.txt"});
    ASSERT_EQ(outcome.status, ParseStatus::run);
    ASSERT_TRUE(outcome.args.has_value());
    EXPECT_EQ(outcome.args->command, Subcommand::info);
    EXPECT_EQ(outcome.args->file, "array.txt");
}

TEST(CliArgs, InfoDoesNotAcceptAStrengthOption) {
    const auto outcome = parse_args({"info", "array.txt", "-t", "3"});
    EXPECT_EQ(outcome.status, ParseStatus::usage_error);
}

TEST(CliArgs, InfoHelpFlagPrintsHelpInsteadOfParsing) {
    const auto outcome = parse_args({"info", "--help"});
    EXPECT_EQ(outcome.status, ParseStatus::help);
    EXPECT_NE(outcome.text.find("covarray info"), std::string::npos);
}

} // namespace covarray::cli
