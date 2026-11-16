// group-73
#ifndef ACADERP_COMMAND_PARSER_HPP
#define ACADERP_COMMAND_PARSER_HPP

#include <optional>
#include <string>
#include <vector>

namespace acaderp {

enum class CommandKind {
    Program,
    Course,
    Student,
    Register,
    Grade,
    Convert,
    Gpa,
    Average,
    Graduate,
    Eligible,
    Transcript
};

struct ParsedCommand {
    CommandKind kind;
    std::vector<std::string> tokens;
};

class CommandParser {
public:
    std::optional<ParsedCommand> parse(const std::string& line) const;
};

}  // namespace acaderp

#endif  // ACADERP_COMMAND_PARSER_HPP
