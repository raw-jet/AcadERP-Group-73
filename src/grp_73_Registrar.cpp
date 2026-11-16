// group-73
#include "grp_73_Registrar.h"

#include "grp_73_ErpException.h"

#include <charconv>
#include <iomanip>
#include <optional>
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

std::vector<std::string> splitCommaList(const std::string& value, const std::string& option) {
    std::vector<std::string> result;
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto separator = value.find(',', start);
        const std::size_t end = separator == std::string::npos ? value.size() : separator;
        if (end == start) {
            throw ParseException("empty item in " + option + " list");
        }
        result.push_back(value.substr(start, end - start));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1U;
    }
    return result;
}

CourseType parseCourseType(const std::string& value) {
    if (value == "REGULAR") return CourseType::Regular;
    if (value == "SU") return CourseType::Satisfactory;
    if (value == "THESIS") return CourseType::Thesis;
    throw InvalidCourseException("course type must be REGULAR, SU, or THESIS");
}

CourseCategory parseCategory(const std::string& value) {
    if (value == "CORE") return CourseCategory::Core;
    if (value == "ELECTIVE") return CourseCategory::Elective;
    if (value == "MANDATORY_SU") return CourseCategory::MandatorySu;
    if (value == "OPEN") return CourseCategory::Open;
    throw InvalidCourseException("unknown course category: " + value);
}

std::string gradeHistory(const Attempt& attempt) {
    std::ostringstream output;
    bool first = true;
    if (attempt.thesisRecord() != nullptr) {
        for (const auto& entry : attempt.thesisRecord()->progress()) {
            if (!first) output << ',';
            output << entry.term.text << ':' << entry.grade.symbol();
            first = false;
        }
        if (attempt.thesisRecord()->finalLetter().has_value()) {
            if (!first) output << ',';
            output << "CONVERT:" << attempt.thesisRecord()->finalLetter()->symbol();
            first = false;
        }
    } else {
        for (const auto& grade : attempt.gradeHistory()) {
            if (!first) output << ',';
            output << grade.symbol();
            first = false;
        }
    }
    return first ? "-" : output.str();
}

}  // namespace

std::optional<std::string> Registrar::execute(const ParsedCommand& command) {
    const auto& tokens = command.tokens;
    switch (command.kind) {
        case CommandKind::Program:
            updateProgram(tokens);
            return std::nullopt;
        case CommandKind::Course:
            defineCourse(tokens);
            return std::nullopt;
        case CommandKind::Student:
            defineStudent(tokens);
            return std::nullopt;
        case CommandKind::Register:
            registerCourse(tokens);
            return std::nullopt;
        case CommandKind::Grade:
            submitGrade(tokens);
            return std::nullopt;
        case CommandKind::Convert:
            convertThesis(tokens);
            return std::nullopt;
        case CommandKind::Gpa:
            return gpaLine(requireStudent(tokens[1]));
        case CommandKind::Average:
            return averageLine(requireStudent(tokens[1]));
        case CommandKind::Graduate:
            return graduationLine(requireStudent(tokens[1]));
        case CommandKind::Eligible: {
            const Student& student = requireStudent(tokens[1]);
            const Course& course = requireCourse(tokens[2]);
            return eligibilityLine(student, course, Term::parse(tokens[3]));
        }
        case CommandKind::Transcript:
            return transcript(requireStudent(tokens[1]));
    }
    throw ParseException("unrecognized parsed command");
}

Student& Registrar::requireStudent(const std::string& roll) {
    const auto found = students_.find(roll);
    if (found == students_.end()) {
        throw UnknownStudentException("unknown student: " + roll);
    }
    return *found->second;
}

const Student& Registrar::requireStudent(const std::string& roll) const {
    const auto found = students_.find(roll);
    if (found == students_.end()) {
        throw UnknownStudentException("unknown student: " + roll);
    }
    return *found->second;
}

Course& Registrar::requireCourse(const std::string& code) {
    const auto found = courses_.find(code);
    if (found == courses_.end()) {
        throw UnknownCourseException("unknown course: " + code);
    }
    return *found->second;
}

const Course& Registrar::requireCourse(const std::string& code) const {
    const auto found = courses_.find(code);
    if (found == courses_.end()) {
        throw UnknownCourseException("unknown course: " + code);
    }
    return *found->second;
}

void Registrar::updateProgram(const std::vector<std::string>& tokens) {
    if (any_course_defined_) {
        throw ParseException("PROGRAM must appear before the first COURSE");
    }
    ProgramRules candidate = rules_;
    for (std::size_t index = 1U; index < tokens.size(); ++index) {
        const auto separator = tokens[index].find('=');
        const std::string key = tokens[index].substr(0U, separator);
        const std::string value = tokens[index].substr(separator + 1U);
        if (key == "total") {
            candidate.total_credits = parseInteger(value, key);
        } else if (key == "elective") {
            candidate.elective_credits = parseInteger(value, key);
        } else if (key == "mingpa") {
            candidate.minimum_gpa_hundredths = parseHundredths(value, key);
        } else if (key == "thesis_min_grade") {
            candidate.thesis_minimum_grade = value;
        } else if (key == "thesis_min_credits") {
            candidate.thesis_minimum_credits = parseInteger(value, key);
        } else if (key == "thesis_min_gpa") {
            candidate.thesis_minimum_gpa_hundredths = parseHundredths(value, key);
        } else if (key == "maxload") {
            candidate.maximum_term_load = parseInteger(value, key);
        } else {
            throw ParseException("unknown PROGRAM key: " + key);
        }
    }
    if (candidate.total_credits < 0 || candidate.elective_credits < 0 ||
        candidate.thesis_minimum_credits < 0 || candidate.maximum_term_load < 0 ||
        !Grade::isLetterSymbol(candidate.thesis_minimum_grade)) {
        throw ParseException("PROGRAM values are outside their valid ranges");
    }
    rules_ = std::move(candidate);
}

void Registrar::defineCourse(const std::vector<std::string>& tokens) {
    const std::string& code = tokens[1];
    if (courses_.find(code) != courses_.end()) {
        throw DuplicateEntityException("course id already exists: " + code);
    }
    const CourseType type = parseCourseType(tokens[2]);
    const int credits = parseInteger(tokens[3], "credits");
    const CourseCategory category = parseCategory(tokens[4]);
    if (credits < 0 || credits > 24 || (credits == 0 && type != CourseType::Satisfactory)) {
        throw InvalidCourseException("credits must be 1..24, except that an SU course may have 0 credits");
    }
    if (category == CourseCategory::MandatorySu && type != CourseType::Satisfactory) {
        throw InvalidCourseException("MANDATORY_SU is valid only for SU courses");
    }

    std::vector<Prerequisite> prerequisites;
    std::vector<std::string> anti_requisites;
    std::unordered_set<std::string> seen_prerequisites;
    std::unordered_set<std::string> seen_anti_requisites;
    for (std::size_t index = 5U; index < tokens.size(); ++index) {
        const auto separator = tokens[index].find('=');
        const std::string key = tokens[index].substr(0U, separator);
        const std::string value = tokens[index].substr(separator + 1U);
        if (key == "PRE") {
            for (const std::string& entry : splitCommaList(value, key)) {
                const auto colon = entry.find(':');
                if (colon != std::string::npos && (colon == 0U || colon + 1U == entry.size() ||
                                                   entry.find(':', colon + 1U) != std::string::npos)) {
                    throw ParseException("malformed prerequisite; expected CODE[:MIN_GRADE]");
                }
                const std::string prerequisite_code = colon == std::string::npos ? entry : entry.substr(0U, colon);
                if (prerequisite_code == code || !seen_prerequisites.insert(prerequisite_code).second) {
                    throw InvalidCourseException("self or repeated prerequisite: " + prerequisite_code);
                }
                const auto prerequisite_it = courses_.find(prerequisite_code);
                if (prerequisite_it == courses_.end()) {
                    throw InvalidCourseException("prerequisite must already exist: " + prerequisite_code);
                }
                const Course& prerequisite_course = *prerequisite_it->second;
                std::string minimum_grade;
                if (colon == std::string::npos) {
                    minimum_grade = prerequisite_course.type() == CourseType::Satisfactory ? "S" : "D";
                } else {
                    minimum_grade = entry.substr(colon + 1U);
                }
                const bool valid_minimum = prerequisite_course.type() == CourseType::Satisfactory
                                               ? minimum_grade == "S"
                                               : Grade::isLetterSymbol(minimum_grade);
                if (!valid_minimum) {
                    throw InvalidCourseException("invalid prerequisite minimum grade for " + prerequisite_code);
                }
                prerequisites.push_back(Prerequisite{prerequisite_code, minimum_grade});
            }
        } else if (key == "ANTI") {
            for (const std::string& anti_code : splitCommaList(value, key)) {
                if (anti_code == code || !seen_anti_requisites.insert(anti_code).second) {
                    throw InvalidCourseException("self or repeated anti-requisite: " + anti_code);
                }
                if (courses_.find(anti_code) == courses_.end()) {
                    throw InvalidCourseException("anti-requisite must already exist: " + anti_code);
                }
                anti_requisites.push_back(anti_code);
            }
        }
    }

    std::unique_ptr<Course> new_course;
    if (type == CourseType::Regular) {
        new_course = std::make_unique<RegularCourse>(code, credits, category, std::move(prerequisites),
                                                     std::move(anti_requisites));
    } else if (type == CourseType::Satisfactory) {
        new_course = std::make_unique<SuCourse>(code, credits, category, std::move(prerequisites),
                                                std::move(anti_requisites));
    } else {
        new_course = std::make_unique<ThesisCourse>(code, credits, category, std::move(prerequisites),
                                                    std::move(anti_requisites));
    }
    courses_.emplace(code, std::move(new_course));
    any_course_defined_ = true;
}

void Registrar::defineStudent(const std::vector<std::string>& tokens) {
    const std::string& roll = tokens[1];
    if (students_.find(roll) != students_.end()) {
        throw DuplicateEntityException("student id already exists: " + roll);
    }
    auto student = std::make_unique<Student>(roll, tokens[2]);
    students_.emplace(roll, std::move(student));
}

void Registrar::registerCourse(const std::vector<std::string>& tokens) {
    Student& student = requireStudent(tokens[1]);
    Course& course = requireCourse(tokens[2]);
    const Term term = Term::parse(tokens[3]);
    const std::optional<std::string> failure = eligibilityFailure(student, course, term);
    if (failure.has_value()) {
        throwEligibilityFailure(failure.value());
    }

    bool improvement = false;
    const Attempt* previous = countingAttempt(student, course);
    const Grade* previous_grade = previous == nullptr ? nullptr : previous->finalGrade();
    if (previous_grade != nullptr && course.type() == CourseType::Regular &&
        (previous_grade->symbol() == "C-" || previous_grade->symbol() == "D")) {
        improvement = true;
    }
    auto attempt = std::make_unique<Attempt>(course, term);
    student.addAttempt(std::move(attempt), improvement);
}

void Registrar::submitGrade(const std::vector<std::string>& tokens) {
    Student& student = requireStudent(tokens[1]);
    Course& course = requireCourse(tokens[2]);
    const Grade grade = Grade::create(tokens[4]);
    course.validateGrade(grade);
    Attempt* attempt = student.findAttempt(course.code(), Term::parse(tokens[3]));
    if (attempt == nullptr) {
        throw NotRegisteredException("no registration for " + course.code() + " in " + tokens[3]);
    }
    if (attempt->thesisRecord() != nullptr) {
        attempt->thesisRecord()->submitProgress(Term::parse(tokens[3]), grade);
    } else {
        attempt->recordGrade(grade);
    }
}

void Registrar::convertThesis(const std::vector<std::string>& tokens) {
    Student& student = requireStudent(tokens[1]);
    Course& course = requireCourse(tokens[2]);
    const Grade letter = Grade::create(tokens[3]);
    if (course.type() != CourseType::Thesis || !letter.isLetter()) {
        throw GradeTypeMismatchException("CONVERT requires a thesis and a letter grade");
    }
    Attempt* target = student.latestAttempt(course.code());
    if (target == nullptr) {
        throw NotRegisteredException("no thesis registration for " + course.code());
    }
    if (target->thesisRecord() == nullptr) {
        throw GradeTypeMismatchException("CONVERT requires a thesis registration");
    }
    target->thesisRecord()->convert(letter);
}

std::optional<std::string> Registrar::eligibilityFailure(const Student& student, const Course& course,
                                                               const Term& term) const {
    const Attempt* counted = countingAttempt(student, course);
    if (counted != nullptr && passedCourse(student, course)) {
        const Grade* grade = counted->finalGrade();
        const bool improvement_allowed = course.type() == CourseType::Regular && grade != nullptr &&
                                         (grade->symbol() == "C-" || grade->symbol() == "D") &&
                                         (!student.improvementUsed(course.code()) ||
                                          student.hasOpenAttempt(course.code()));
        if (!improvement_allowed) {
            return "E2";
        }
    }
    if (student.hasOpenAttempt(course.code())) {
        return "E3";
    }
    for (const Prerequisite& prerequisite : course.prerequisites()) {
        const Course& prerequisite_course = *courses_.at(prerequisite.course_code);
        const Attempt* prerequisite_attempt = countingAttempt(student, prerequisite_course);
        const Grade* prerequisite_grade = prerequisite_attempt == nullptr ? nullptr : prerequisite_attempt->finalGrade();
        if (prerequisite_grade == nullptr || !prerequisite_course.earnsCredits(*prerequisite_grade)) {
            return "E4";
        }
        if (prerequisite_course.type() == CourseType::Satisfactory) {
            if (prerequisite_grade->symbol() != "S") return "E4";
        } else if (prerequisite_grade->points() < Grade::create(prerequisite.minimum_grade).points()) {
            return "E4";
        }
    }
    for (const std::string& anti_code : course.antiRequisites()) {
        const Course& anti_course = *courses_.at(anti_code);
        if (passedCourse(student, anti_course)) {
            return "E5";
        }
    }

    std::int64_t term_credits = 0;
    for (const auto& attempt : student.attempts()) {
        if (attempt->term() == term) {
            term_credits += attempt->course().credits();
        }
    }
    term_credits += course.credits();
    if (term_credits > rules_.maximum_term_load) {
        return "E6";
    }

    if (course.type() == CourseType::Thesis) {
        const std::int64_t earned = earnedCredits(student);
        const std::optional<int> gpa = roundedGpaHundredths(student);
        if (earned < rules_.thesis_minimum_credits || !gpa.has_value() ||
            gpa.value() < rules_.thesis_minimum_gpa_hundredths) {
            return "E7";
        }
        for (const auto& attempt : student.attempts()) {
            if (attempt->course().type() == CourseType::Thesis && attempt->course().code() != course.code() &&
                (attempt->isOpen() || passedCourse(student, attempt->course()))) {
                return "E7";
            }
        }
    }
    return std::nullopt;
}

void Registrar::throwEligibilityFailure(const std::string& code) const {
    if (code == "E2") throw AlreadyPassedException();
    if (code == "E3") throw DuplicateRegistrationException();
    if (code == "E4") throw PrerequisiteNotMetException();
    if (code == "E5") throw AntiRequisiteException();
    if (code == "E6") throw CreditLimitExceededException();
    if (code == "E7") throw ThesisNotAllowedException();
    throw ParseException("internal eligibility failure code");
}

const Attempt* Registrar::countingAttempt(const Student& student, const Course& course) const noexcept {
    return student.countingAttempt(course.code());
}

bool Registrar::passedCourse(const Student& student, const Course& course) const noexcept {
    const Attempt* attempt = countingAttempt(student, course);
    const Grade* grade = attempt == nullptr ? nullptr : attempt->finalGrade();
    return grade != nullptr && course.earnsCredits(*grade);
}

std::int64_t Registrar::earnedCredits(const Student& student) const {
    std::int64_t total = 0;
    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        if (passedCourse(student, course)) {
            total += course.credits();
        }
    }
    return total;
}

std::optional<int> Registrar::roundedGpaHundredths(const Student& student) const {
    std::int64_t numerator = 0;
    std::int64_t denominator = 0;
    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        const Attempt* attempt = countingAttempt(student, course);
        const Grade* grade = attempt == nullptr ? nullptr : attempt->finalGrade();
        if (grade != nullptr && course.countsInGpa(*grade)) {
            numerator += static_cast<std::int64_t>(course.credits()) * grade->points();
            denominator += course.credits();
        }
    }
    if (denominator == 0) {
        return std::nullopt;
    }
    const std::int64_t rounded = (200 * numerator + denominator) / (2 * denominator);
    return static_cast<int>(rounded);
}

std::string Registrar::gpaLine(const Student& student) const {
    const std::optional<int> gpa = roundedGpaHundredths(student);
    if (!gpa.has_value()) {
        throw NoGpaDataException();
    }
    std::ostringstream output;
    output << "GPA " << student.roll() << ' ' << gpa.value() / 100 << '.' << std::setw(2) << std::setfill('0')
           << gpa.value() % 100;
    return output.str();
}

std::string Registrar::averageLine(const Student& student) const {
    const std::optional<int> gpa = roundedGpaHundredths(student);
    if (!gpa.has_value()) {
        throw NoGpaDataException();
    }
    const int value = gpa.value();
    std::string average;
    if (value >= 950) average = "A";
    else if (value >= 850) average = "A-";
    else if (value >= 750) average = "B";
    else if (value >= 650) average = "B-";
    else if (value >= 550) average = "C";
    else if (value >= 450) average = "C-";
    else if (value >= 350) average = "D";
    else if (value >= 100) average = "E";
    else average = "F";
    return "AVG " + student.roll() + " " + average;
}

std::string Registrar::graduationLine(const Student& student) const {
    std::vector<std::string> failures;
    if (earnedCredits(student) < rules_.total_credits) failures.push_back("R1");

    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        if (course.category() == CourseCategory::Core && !passedCourse(student, course)) {
            failures.push_back("R2");
            break;
        }
    }

    std::int64_t elective_credits = 0;
    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        if (course.category() == CourseCategory::Elective && passedCourse(student, course)) {
            elective_credits += course.credits();
        }
    }
    if (elective_credits < rules_.elective_credits) failures.push_back("R3");

    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        if (course.category() == CourseCategory::MandatorySu) {
            const Attempt* attempt = countingAttempt(student, course);
            const Grade* grade = attempt == nullptr ? nullptr : attempt->finalGrade();
            if (grade == nullptr || grade->symbol() != "S") {
                failures.push_back("R4");
                break;
            }
        }
    }

    const int minimum_thesis_points = Grade::create(rules_.thesis_minimum_grade).points();
    bool thesis_requirement_met = false;
    for (const auto& entry : courses_) {
        const Course& course = *entry.second;
        if (course.type() != CourseType::Thesis) continue;
        const Attempt* attempt = countingAttempt(student, course);
        const Grade* grade = attempt == nullptr ? nullptr : attempt->finalGrade();
        if (grade != nullptr && grade->points() >= minimum_thesis_points) {
            thesis_requirement_met = true;
            break;
        }
    }
    if (!thesis_requirement_met) failures.push_back("R5");

    const std::optional<int> gpa = roundedGpaHundredths(student);
    if (!gpa.has_value() || gpa.value() < rules_.minimum_gpa_hundredths) failures.push_back("R6");

    for (const auto& attempt : student.attempts()) {
        if (attempt->course().type() != CourseType::Thesis && attempt->isOpen()) {
            failures.push_back("R7");
            break;
        }
    }

    if (failures.empty()) return "GRAD " + student.roll() + " YES";
    std::ostringstream output;
    output << "GRAD " << student.roll() << " NO ";
    for (std::size_t index = 0U; index < failures.size(); ++index) {
        if (index != 0U) output << ',';
        output << failures[index];
    }
    return output.str();
}

std::string Registrar::eligibilityLine(const Student& student, const Course& course, const Term& term) const {
    const std::optional<std::string> failure = eligibilityFailure(student, course, term);
    if (!failure.has_value()) {
        return "ELIGIBLE " + student.roll() + " " + course.code() + " YES";
    }
    return "ELIGIBLE " + student.roll() + " " + course.code() + " NO " + failure.value();
}

std::string Registrar::transcript(const Student& student) const {
    std::ostringstream output;
    output << "TRANSCRIPT " << student.roll();
    for (const auto& attempt_ptr : student.attempts()) {
        const Attempt& attempt = *attempt_ptr;
        const Course& course = attempt.course();
        std::string status;
        if (attempt.thesisRecord() != nullptr) {
            status = toString(attempt.thesisRecord()->state());
        } else if (!attempt.hasFinalGrade()) {
            status = attempt.gradeHistory().empty() ? "PENDING" : "INCOMPLETE";
        } else {
            status = "FINAL";
        }
        output << " | " << course.code() << ' ' << attempt.term().text << ' ' << toString(course.type()) << ' '
               << course.credits() << ' ' << toString(course.category()) << ' ' << status << ' '
               << gradeHistory(attempt);
    }
    return output.str();
}

}  // namespace acaderp
