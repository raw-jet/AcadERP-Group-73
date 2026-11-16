// group-73
#include "grp_73_Model.h"

#include <charconv>
#include <cctype>
#include <string_view>

namespace acaderp {
namespace {

bool isAllDigits(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    for (const char ch : value) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }
    return true;
}

int parseSmallInteger(std::string_view value) {
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        throw ParseException("invalid term: expected YYYY-1, YYYY-2, or YYYY-3");
    }
    return result;
}

}  // namespace

Term Term::parse(const std::string& value) {
    const std::string_view view(value);
    if (view.size() != 6U || view[4] != '-' || !isAllDigits(view.substr(0U, 4U)) ||
        !isAllDigits(view.substr(5U, 1U))) {
        throw ParseException("invalid term: expected YYYY-1, YYYY-2, or YYYY-3");
    }
    const int year_value = parseSmallInteger(view.substr(0U, 4U));
    const int semester_value = parseSmallInteger(view.substr(5U, 1U));
    if (year_value == 0 || semester_value < 1 || semester_value > 3) {
        throw ParseException("invalid term: expected YYYY-1, YYYY-2, or YYYY-3");
    }
    return Term{year_value, semester_value, year_value * 3 + semester_value, value};
}

Grade Grade::create(const std::string& symbol) {
    if (symbol == "A") return Grade(symbol, 10);
    if (symbol == "A-") return Grade(symbol, 9);
    if (symbol == "B") return Grade(symbol, 8);
    if (symbol == "B-") return Grade(symbol, 7);
    if (symbol == "C") return Grade(symbol, 6);
    if (symbol == "C-") return Grade(symbol, 5);
    if (symbol == "D") return Grade(symbol, 4);
    if (symbol == "E") return Grade(symbol, 2);
    if (symbol == "F") return Grade(symbol, 0);
    if (symbol == "I" || symbol == "S" || symbol == "X") return Grade(symbol, 0);
    throw InvalidGradeSymbolException("invalid grade symbol: " + symbol);
}

bool Grade::isKnownSymbol(const std::string& symbol) noexcept {
    return isLetterSymbol(symbol) || symbol == "I" || symbol == "S" || symbol == "X";
}

bool Grade::isLetterSymbol(const std::string& symbol) noexcept {
    return symbol == "A" || symbol == "A-" || symbol == "B" || symbol == "B-" ||
           symbol == "C" || symbol == "C-" || symbol == "D" || symbol == "E" || symbol == "F";
}

bool Grade::isPass() const noexcept {
    return symbol_ == "S" || (isLetter() && points_ >= 4);
}

std::unique_ptr<CourseMaterial> TeachingMaterials::clone() const {
    return std::make_unique<TeachingMaterials>(*this);
}

Course::Course(std::string code, int credits, CourseCategory category,
               std::vector<Prerequisite> prerequisites, std::vector<std::string> anti_requisites)
    : code_(std::move(code)), credits_(credits), category_(category),
      prerequisites_(std::move(prerequisites)), anti_requisites_(std::move(anti_requisites)),
      materials_(std::make_unique<TeachingMaterials>()) {}

void RegularCourse::validateGrade(const Grade& grade) const {
    if (!grade.isLetter() && grade.symbol() != "I") {
        throw GradeTypeMismatchException("regular courses accept letter grades or I");
    }
}

bool RegularCourse::countsInGpa(const Grade& grade) const noexcept {
    return grade.isLetter();
}

bool RegularCourse::earnsCredits(const Grade& grade) const noexcept {
    return grade.isLetter() && grade.isPass();
}

void SuCourse::validateGrade(const Grade& grade) const {
    if (grade.symbol() != "S" && grade.symbol() != "X" && grade.symbol() != "I") {
        throw GradeTypeMismatchException("SU courses accept S, X, or I");
    }
}

bool SuCourse::earnsCredits(const Grade& grade) const noexcept {
    return grade.symbol() == "S";
}

void ThesisCourse::validateGrade(const Grade& grade) const {
    if (grade.symbol() != "S" && grade.symbol() != "X") {
        throw GradeTypeMismatchException("thesis progress grades must be S or X");
    }
}

bool ThesisCourse::countsInGpa(const Grade& grade) const noexcept {
    return grade.isLetter();
}

bool ThesisCourse::earnsCredits(const Grade& grade) const noexcept {
    return grade.isLetter() && grade.isPass();
}

void ThesisRecord::submitProgress(const Term& term, const Grade& grade) {
    // Validate the registration-time boundary first. A submission dated before
    // the thesis existed is an invalid transition, even if this record later
    // reached FINAL or TERMINATED.
    if (term < start_term_) {
        throw InvalidThesisTransitionException("thesis progress term predates registration");
    }
    if (state_ == ThesisState::Final) {
        throw GradeAlreadyFinalException("a final thesis cannot receive progress grades");
    }
    if (state_ == ThesisState::Terminated) {
        throw ThesisTerminatedException();
    }
    if (grade.symbol() != "S" && grade.symbol() != "X") {
        throw GradeTypeMismatchException("thesis progress grades must be S or X");
    }
    if (!progress_.empty()) {
        if (term == progress_.back().term) {
            throw GradeAlreadyFinalException("only one thesis progress grade is allowed per term");
        }
        if (term < progress_.back().term) {
            throw InvalidThesisTransitionException("thesis progress terms must strictly increase");
        }
    }

    progress_.push_back(ProgressEntry{term, grade});
    if (grade.symbol() == "X") {
        state_ = previous_progress_was_x_ ? ThesisState::Terminated : ThesisState::AtRisk;
        previous_progress_was_x_ = true;
    } else {
        state_ = ThesisState::InProgress;
        previous_progress_was_x_ = false;
    }
}

void ThesisRecord::convert(const Grade& letter) {
    if (state_ == ThesisState::Final) {
        throw GradeAlreadyFinalException("thesis has already been converted");
    }
    if (state_ == ThesisState::Terminated) {
        throw ThesisTerminatedException();
    }
    if (state_ != ThesisState::InProgress) {
        throw InvalidThesisTransitionException("only an in-progress thesis can be converted");
    }
    if (!letter.isLetter()) {
        throw GradeTypeMismatchException("CONVERT requires a letter grade");
    }
    final_letter_.emplace(letter);
    state_ = ThesisState::Final;
}

Attempt::Attempt(Course& course, Term term) : course_(&course), term_(std::move(term)) {
    if (course.type() == CourseType::Thesis) {
        thesis_record_ = std::make_unique<ThesisRecord>(term_);
    }
}

void Attempt::recordGrade(const Grade& grade) {
    if (thesis_record_) {
        throw GradeTypeMismatchException("thesis grades must be submitted as progress or conversion");
    }
    if (!grade_history_.empty() && grade_history_.back().isFinal()) {
        throw GradeAlreadyFinalException();
    }
    if (!grade_history_.empty() && grade_history_.back().symbol() == "I" && grade.symbol() == "I") {
        throw GradeAlreadyFinalException("an incomplete grade can only be replaced by a final grade");
    }
    grade_history_.push_back(grade);
}

bool Attempt::hasFinalGrade() const noexcept {
    if (thesis_record_) {
        return thesis_record_->state() == ThesisState::Final;
    }
    return !grade_history_.empty() && grade_history_.back().isFinal();
}

const Grade* Attempt::finalGrade() const noexcept {
    if (thesis_record_) {
        const auto& letter = thesis_record_->finalLetter();
        return letter.has_value() ? &letter.value() : nullptr;
    }
    return !grade_history_.empty() && grade_history_.back().isFinal() ? &grade_history_.back() : nullptr;
}

bool Attempt::isOpen() const noexcept {
    if (thesis_record_) {
        const ThesisState current = thesis_record_->state();
        return current != ThesisState::Final && current != ThesisState::Terminated;
    }
    return !hasFinalGrade();
}

Attempt* Student::findAttempt(const std::string& course_code, const Term& term) noexcept {
    const auto found = attempts_by_course_.find(course_code);
    if (found == attempts_by_course_.end()) return nullptr;

    for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
        Attempt* const attempt = *it;
        if (attempt->course().type() == CourseType::Thesis) {
            // Progress is keyed by its progress term, not the registration term.
            // Select the newest thesis registration that had started by this term;
            // this also lets a command aimed at a terminated earlier attempt receive
            // E_THESIS_TERMINATED after a later retry has been registered.
            if (!(term < attempt->term())) return attempt;
        } else if (attempt->term() == term) {
            return attempt;
        }
    }
    // If the course is a thesis but every recorded attempt starts after the
    // supplied progress term, return the earliest attempt. ThesisRecord then
    // reports E_THESIS_TRANSITION (progress predates registration) rather than
    // incorrectly reporting E_NOT_REGISTERED.
    if (!found->second.empty() && found->second.front()->course().type() == CourseType::Thesis) {
        return found->second.front();
    }
    return nullptr;
}

const Attempt* Student::latestAttempt(const std::string& course_code) const noexcept {
    const auto found = attempts_by_course_.find(course_code);
    return found == attempts_by_course_.end() || found->second.empty() ? nullptr : found->second.back();
}

Attempt* Student::latestAttempt(const std::string& course_code) noexcept {
    const auto found = attempts_by_course_.find(course_code);
    return found == attempts_by_course_.end() || found->second.empty() ? nullptr : found->second.back();
}

Attempt* Student::latestOpenAttempt(const std::string& course_code) noexcept {
    const auto found = attempts_by_course_.find(course_code);
    if (found == attempts_by_course_.end()) return nullptr;
    for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
        if ((*it)->isOpen()) return *it;
    }
    return nullptr;
}

const Attempt* Student::countingAttempt(const std::string& course_code) const noexcept {
    const auto found = attempts_by_course_.find(course_code);
    if (found == attempts_by_course_.end()) return nullptr;
    for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
        if ((*it)->hasFinalGrade()) return *it;
    }
    return nullptr;
}

bool Student::hasOpenAttempt(const std::string& course_code) const noexcept {
    const auto found = attempts_by_course_.find(course_code);
    if (found == attempts_by_course_.end()) return false;
    for (const Attempt* attempt : found->second) {
        if (attempt->isOpen()) return true;
    }
    return false;
}

bool Student::improvementUsed(const std::string& course_code) const {
    return improved_courses_.find(course_code) != improved_courses_.end();
}

void Student::addAttempt(std::unique_ptr<Attempt> attempt, bool is_improvement) {
    const std::string course_code = attempt->course().code();
    Attempt* const attempt_pointer = attempt.get();

    // Reserve the owning vector before changing any logical state.
    attempts_.reserve(attempts_.size() + 1U);
    const auto course_attempts = attempts_by_course_.try_emplace(course_code);

    // If this call created the map entry, remove it automatically if a later
    // allocation fails. This preserves the strong exception guarantee without
    // catching exceptions inside the model layer.
    struct IndexRollback {
        std::map<std::string, std::vector<Attempt*>>& index;
        std::map<std::string, std::vector<Attempt*>>::iterator entry;
        bool active;
        ~IndexRollback() {
            if (active) index.erase(entry);
        }
    } rollback{attempts_by_course_, course_attempts.first, course_attempts.second};

    course_attempts.first->second.reserve(course_attempts.first->second.size() + 1U);
    if (is_improvement) {
        improved_courses_.insert(course_code);
    }
    course_attempts.first->second.push_back(attempt_pointer);

    // Capacity was reserved above, and moving a unique_ptr cannot throw.
    attempts_.push_back(std::move(attempt));
    rollback.active = false;
}

std::string toString(CourseType type) {
    switch (type) {
        case CourseType::Regular: return "REGULAR";
        case CourseType::Satisfactory: return "SU";
        case CourseType::Thesis: return "THESIS";
    }
    return "";
}

std::string toString(CourseCategory category) {
    switch (category) {
        case CourseCategory::Core: return "CORE";
        case CourseCategory::Elective: return "ELECTIVE";
        case CourseCategory::MandatorySu: return "MANDATORY_SU";
        case CourseCategory::Open: return "OPEN";
    }
    return "";
}

std::string toString(ThesisState state) {
    switch (state) {
        case ThesisState::Registered: return "REGISTERED";
        case ThesisState::InProgress: return "IN_PROGRESS";
        case ThesisState::AtRisk: return "AT_RISK";
        case ThesisState::Terminated: return "TERMINATED";
        case ThesisState::Final: return "FINAL";
    }
    return "";
}

}  // namespace acaderp
