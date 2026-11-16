// group-73
#ifndef ACADERP_MODEL_HPP
#define ACADERP_MODEL_HPP

#include "grp_73_ErpException.h"

#include <memory>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace acaderp {

enum class CourseType { Regular, Satisfactory, Thesis };
enum class CourseCategory { Core, Elective, MandatorySu, Open };
enum class ThesisState { Registered, InProgress, AtRisk, Terminated, Final };

struct Term {
    int year;
    int semester;
    int order;
    std::string text;

    static Term parse(const std::string& value);
    bool operator<(const Term& other) const noexcept { return order < other.order; }
    bool operator==(const Term& other) const noexcept { return order == other.order; }
};

class Grade {
public:
    static Grade create(const std::string& symbol);
    static bool isKnownSymbol(const std::string& symbol) noexcept;
    static bool isLetterSymbol(const std::string& symbol) noexcept;

    const std::string& symbol() const noexcept { return symbol_; }
    int points() const noexcept { return points_; }
    bool isPass() const noexcept;
    bool isFinal() const noexcept { return symbol_ != "I"; }
    bool isLetter() const noexcept { return isLetterSymbol(symbol_); }

private:
    Grade(std::string symbol, int points) : symbol_(std::move(symbol)), points_(points) {}

    std::string symbol_;
    int points_;
};

struct Prerequisite {
    std::string course_code;
    std::string minimum_grade;
};

class CourseMaterial {
public:
    virtual ~CourseMaterial() = default;
    virtual std::unique_ptr<CourseMaterial> clone() const = 0;
    virtual void setSyllabus(std::string syllabus) = 0;
    virtual void addLectureNote(std::string note) = 0;
    virtual void addAssignment(std::string assignment) = 0;

    const std::string& syllabus() const noexcept { return syllabus_; }
    const std::vector<std::string>& lectureNotes() const noexcept { return lecture_notes_; }
    const std::vector<std::string>& assignments() const noexcept { return assignments_; }

protected:
    std::string syllabus_;
    std::vector<std::string> lecture_notes_;
    std::vector<std::string> assignments_;
};

class TeachingMaterials final : public CourseMaterial {
public:
    std::unique_ptr<CourseMaterial> clone() const override;
    void setSyllabus(std::string syllabus) override { syllabus_ = std::move(syllabus); }
    void addLectureNote(std::string note) override { lecture_notes_.push_back(std::move(note)); }
    void addAssignment(std::string assignment) override { assignments_.push_back(std::move(assignment)); }
};

class Course {
public:
    Course(std::string code, int credits, CourseCategory category,
           std::vector<Prerequisite> prerequisites, std::vector<std::string> anti_requisites);
    virtual ~Course() = default;

    const std::string& code() const noexcept { return code_; }
    int credits() const noexcept { return credits_; }
    CourseCategory category() const noexcept { return category_; }
    const std::vector<Prerequisite>& prerequisites() const noexcept { return prerequisites_; }
    const std::vector<std::string>& antiRequisites() const noexcept { return anti_requisites_; }
    CourseMaterial& materials() noexcept { return *materials_; }
    const CourseMaterial& materials() const noexcept { return *materials_; }
    std::unique_ptr<CourseMaterial> teachingCopy() const { return materials_->clone(); }

    virtual CourseType type() const noexcept = 0;
    virtual void validateGrade(const Grade& grade) const = 0;
    virtual bool countsInGpa(const Grade& grade) const noexcept = 0;
    virtual bool earnsCredits(const Grade& grade) const noexcept = 0;

private:
    std::string code_;
    int credits_;
    CourseCategory category_;
    std::vector<Prerequisite> prerequisites_;
    std::vector<std::string> anti_requisites_;
    std::unique_ptr<CourseMaterial> materials_;
};

class RegularCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Regular; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade& grade) const noexcept override;
    bool earnsCredits(const Grade& grade) const noexcept override;
};

class SuCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Satisfactory; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade&) const noexcept override { return false; }
    bool earnsCredits(const Grade& grade) const noexcept override;
};

class ThesisCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Thesis; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade& grade) const noexcept override;
    bool earnsCredits(const Grade& grade) const noexcept override;
};

struct ProgressEntry {
    Term term;
    Grade grade;
};

class ThesisRecord {
public:
    explicit ThesisRecord(Term start_term) : start_term_(std::move(start_term)) {}

    ThesisState state() const noexcept { return state_; }
    const std::vector<ProgressEntry>& progress() const noexcept { return progress_; }
    const std::optional<Grade>& finalLetter() const noexcept { return final_letter_; }
    const Term& startTerm() const noexcept { return start_term_; }
    void submitProgress(const Term& term, const Grade& grade);
    void convert(const Grade& letter);

private:
    Term start_term_;
    ThesisState state_ = ThesisState::Registered;
    std::vector<ProgressEntry> progress_;
    std::optional<Grade> final_letter_;
    bool previous_progress_was_x_ = false;
};

class Attempt {
public:
    Attempt(Course& course, Term term);

    Course& course() const noexcept { return *course_; }
    const Term& term() const noexcept { return term_; }
    const std::vector<Grade>& gradeHistory() const noexcept { return grade_history_; }
    ThesisRecord* thesisRecord() noexcept { return thesis_record_.get(); }
    const ThesisRecord* thesisRecord() const noexcept { return thesis_record_.get(); }
    void recordGrade(const Grade& grade);
    bool hasFinalGrade() const noexcept;
    const Grade* finalGrade() const noexcept;
    bool isOpen() const noexcept;

private:
    Course* course_;
    Term term_;
    std::vector<Grade> grade_history_;
    std::unique_ptr<ThesisRecord> thesis_record_;
};

class Student {
public:
    Student(std::string roll, std::string name) : roll_(std::move(roll)), name_(std::move(name)) {}

    const std::string& roll() const noexcept { return roll_; }
    const std::string& name() const noexcept { return name_; }
    const std::vector<std::unique_ptr<Attempt>>& attempts() const noexcept { return attempts_; }
    Attempt* findAttempt(const std::string& course_code, const Term& term) noexcept;
    Attempt* latestAttempt(const std::string& course_code) noexcept;
    const Attempt* latestAttempt(const std::string& course_code) const noexcept;
    Attempt* latestOpenAttempt(const std::string& course_code) noexcept;
    const Attempt* countingAttempt(const std::string& course_code) const noexcept;
    bool hasOpenAttempt(const std::string& course_code) const noexcept;
    bool improvementUsed(const std::string& course_code) const;
    void addAttempt(std::unique_ptr<Attempt> attempt, bool is_improvement);

private:
    std::string roll_;
    std::string name_;
    std::vector<std::unique_ptr<Attempt>> attempts_;
    std::map<std::string, std::vector<Attempt*>> attempts_by_course_;
    std::set<std::string> improved_courses_;
};

std::string toString(CourseType type);
std::string toString(CourseCategory category);
std::string toString(ThesisState state);

}  // namespace acaderp

#endif  // ACADERP_MODEL_HPP
