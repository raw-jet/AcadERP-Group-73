// group-73
#include "grp_73_CommandParser.h"

#include "grp_73_ErpException.h"
#include "grp_73_Model.h"

#include <charconv>
#include <cctype>
#include <sstream>
#include <string_view>
#include <unordered_set>

namespace acaderp {
namespace {

int parseInteger(const std::string& value, const std::string& description) {
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        throw ParseException("invalid " + description + ": expected an integer");
    }
    return result;
}

int parseHundredths(const std::string& value, const std::string& description) {
    const std::string_view view(value);
    const auto point = view.find('.');
    if (point == std::string_view::npos || point == 0U || point > 2U || view.size() - point != 3U) {
        throw ParseException("invalid " + description + ": expected a value from 0.00 to 10.00");
    }
    const auto all_digits = [](std::string_view part) {
        if (part.empty()) return false;
        for (const char ch : part) {
            if (ch < '0' || ch > '9') return false;
        }
        return true;
    };
    if (!all_digits(view.substr(0U, point)) || !all_digits(view.substr(point + 1U))) {
        throw ParseException("invalid " + description + ": expected a value from 0.00 to 10.00");
    }
    int whole = 0;
    int fraction = 0;
    const auto whole_result = std::from_chars(view.data(), view.data() + point, whole);
    const auto fraction_result = std::from_chars(view.data() + point + 1U, view.data() + view.size(), fraction);
    if (whole_result.ec != std::errc{} || whole_result.ptr != view.data() + point ||
        fraction_result.ec != std::errc{} || fraction_result.ptr != view.data() + view.size() ||
        view[point + 1U] < '0' || view[point + 1U] > '9' || view[point + 2U] < '0' || view[point + 2U] > '9' ||
        whole < 0 || whole > 10 || (whole == 10 && fraction != 0)) {
        throw ParseException("invalid " + description + ": expected a value from 0.00 to 10.00");
    }
    return whole * 100 + fraction;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(std::move(token));
    }
    return tokens;
}

void requireTokenCount(const std::vector<std::string>& tokens, std::size_t minimum, std::size_t maximum) {
    if (tokens.size() < minimum || tokens.size() > maximum) {
        throw ParseException("wrong number of tokens for " + tokens.front());
    }
}

void validateProgramTokens(const std::vector<std::string>& tokens) {
    static const std::unordered_set<std::string> integer_keys{
        "total", "elective", "thesis_min_credits", "maxload"};
    static const std::unordered_set<std::string> decimal_keys{"mingpa", "thesis_min_gpa"};
    static const std::unordered_set<std::string> all_keys{
        "total", "elective", "mingpa", "thesis_min_grade", "thesis_min_credits", "thesis_min_gpa", "maxload"};
    std::unordered_set<std::string> seen;
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        const std::string& token = tokens[index];
        const auto separator = token.find('=');
        if (separator == std::string::npos || separator == 0U || separator + 1U == token.size()) {
            throw ParseException("PROGRAM options must use key=value syntax");
        }
        const std::string key = token.substr(0U, separator);
        const std::string value = token.substr(separator + 1U);
        if (all_keys.find(key) == all_keys.end() || !seen.insert(key).second) {
            throw ParseException("unknown or repeated PROGRAM key: " + key);
        }
        if (integer_keys.find(key) != integer_keys.end()) {
            static_cast<void>(parseInteger(value, key));
        } else if (decimal_keys.find(key) != decimal_keys.end()) {
            static_cast<void>(parseHundredths(value, key));
        } else if (!Grade::isLetterSymbol(value)) {
            throw ParseException("thesis_min_grade must be a letter grade");
        }
    }
}

void validateCourseOptions(const std::vector<std::string>& tokens) {
    std::unordered_set<std::string> seen;
    for (std::size_t index = 5U; index < tokens.size(); ++index) {
        const std::string& token = tokens[index];
        const auto separator = token.find('=');
        if (separator == std::string::npos || separator + 1U == token.size()) {
            throw ParseException("COURSE options must be PRE=... or ANTI=...");
        }
        const std::string key = token.substr(0U, separator);
        if ((key != "PRE" && key != "ANTI") || !seen.insert(key).second) {
            throw ParseException("unknown or repeated COURSE option: " + key);
        }
    }
}

}  // namespace

std::optional<ParsedCommand> CommandParser::parse(const std::string& line) const {
    const auto first_non_space = line.find_first_not_of(" \t\r\n\f\v");
    if (first_non_space == std::string::npos || line[first_non_space] == '#') {
        return std::nullopt;
    }
    std::vector<std::string> tokens = tokenize(line);
    const std::string& command = tokens.front();
    CommandKind kind;
    if (command == "PROGRAM") {
        kind = CommandKind::Program;
        requireTokenCount(tokens, 1U, static_cast<std::size_t>(-1));
        validateProgramTokens(tokens);
    } else if (command == "COURSE") {
        kind = CommandKind::Course;
        requireTokenCount(tokens, 5U, static_cast<std::size_t>(-1));
        static_cast<void>(parseInteger(tokens[3], "credits"));
        validateCourseOptions(tokens);
    } else if (command == "STUDENT") {
        kind = CommandKind::Student;
        requireTokenCount(tokens, 3U, 3U);
    } else if (command == "REGISTER") {
        kind = CommandKind::Register;
        requireTokenCount(tokens, 4U, 4U);
        static_cast<void>(Term::parse(tokens[3]));
    } else if (command == "GRADE") {
        kind = CommandKind::Grade;
        requireTokenCount(tokens, 5U, 5U);
        static_cast<void>(Term::parse(tokens[3]));
    } else if (command == "CONVERT") {
        kind = CommandKind::Convert;
        requireTokenCount(tokens, 4U, 4U);
    } else if (command == "GPA") {
        kind = CommandKind::Gpa;
        requireTokenCount(tokens, 2U, 2U);
    } else if (command == "AVG") {
        kind = CommandKind::Average;
        requireTokenCount(tokens, 2U, 2U);
    } else if (command == "GRAD") {
        kind = CommandKind::Graduate;
        requireTokenCount(tokens, 2U, 2U);
    } else if (command == "ELIGIBLE") {
        kind = CommandKind::Eligible;
        requireTokenCount(tokens, 4U, 4U);
        static_cast<void>(Term::parse(tokens[3]));
    } else if (command == "TRANSCRIPT") {
        kind = CommandKind::Transcript;
        requireTokenCount(tokens, 2U, 2U);
    } else {
        throw ParseException("unknown command: " + command);
    }
    return ParsedCommand{kind, std::move(tokens)};
}

}  // namespace acaderp
