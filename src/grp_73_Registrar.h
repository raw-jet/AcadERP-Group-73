// group-73
#ifndef ACADERP_REGISTRAR_HPP
#define ACADERP_REGISTRAR_HPP

#include "grp_73_CommandParser.h"
#include "grp_73_Model.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace acaderp {

struct ProgramRules {
    int total_credits = 160;
    int elective_credits = 24;
    int minimum_gpa_hundredths = 500;
    std::string thesis_minimum_grade = "C";
    int thesis_minimum_credits = 100;
    int thesis_minimum_gpa_hundredths = 600;
    int maximum_term_load = 24;
};

class Registrar {
public:
    std::optional<std::string> execute(const ParsedCommand& command);

private:
    ProgramRules rules_;
    std::map<std::string, std::unique_ptr<Course>> courses_;
    std::map<std::string, std::unique_ptr<Student>> students_;
    bool any_course_defined_ = false;

    Student& requireStudent(const std::string& roll);
    const Student& requireStudent(const std::string& roll) const;
    Course& requireCourse(const std::string& code);
    const Course& requireCourse(const std::string& code) const;
    void updateProgram(const std::vector<std::string>& tokens);
    void defineCourse(const std::vector<std::string>& tokens);
    void defineStudent(const std::vector<std::string>& tokens);
    void registerCourse(const std::vector<std::string>& tokens);
    void submitGrade(const std::vector<std::string>& tokens);
    void convertThesis(const std::vector<std::string>& tokens);

    std::optional<std::string> eligibilityFailure(const Student& student, const Course& course,
                                                  const Term& term) const;
    void throwEligibilityFailure(const std::string& code) const;
    const Attempt* countingAttempt(const Student& student, const Course& course) const noexcept;
    bool passedCourse(const Student& student, const Course& course) const noexcept;
    std::int64_t earnedCredits(const Student& student) const;
    std::optional<int> roundedGpaHundredths(const Student& student) const;
    std::string gpaLine(const Student& student) const;
    std::string averageLine(const Student& student) const;
    std::string graduationLine(const Student& student) const;
    std::string eligibilityLine(const Student& student, const Course& course, const Term& term) const;
    std::string transcript(const Student& student) const;
};

}  // namespace acaderp

#endif  // ACADERP_REGISTRAR_HPP
