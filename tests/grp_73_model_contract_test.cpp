// group-73
#include "grp_73_Model.h"

#include <array>
#include <string>
#include <utility>

namespace {

template <typename Exception, typename Function>
bool throwsCode(Function&& function, const std::string& expected) {
    try {
        function();
    } catch (const Exception& error) {
        return error.code() == expected;
    } catch (...) {
        return false;
    }
    return false;
}

bool expect(bool condition) {
    return condition;
}

}  // namespace

int main() {
    using namespace acaderp;

    const std::array<std::pair<const char*, int>, 12> grades{{
        {"A", 10}, {"A-", 9}, {"B", 8}, {"B-", 7}, {"C", 6}, {"C-", 5},
        {"D", 4}, {"E", 2}, {"F", 0}, {"I", 0}, {"S", 0}, {"X", 0}}};
    for (const auto& entry : grades) {
        const Grade grade = Grade::create(entry.first);
        if (!expect(grade.symbol() == entry.first && grade.points() == entry.second &&
                    Grade::isKnownSymbol(entry.first))) return 1;
    }
    if (!expect(!Grade::isKnownSymbol("a") && !Grade::isKnownSymbol("A+"))) return 2;
    if (!expect(!Grade::create("I").isFinal() && Grade::create("S").isFinal())) return 3;
    if (!expect(Grade::create("D").isPass() && Grade::create("S").isPass() &&
                !Grade::create("E").isPass() && !Grade::create("F").isPass())) return 4;
    if (!throwsCode<InvalidGradeSymbolException>([] { static_cast<void>(Grade::create("a")); },
                                                 "E_GRADE_SYMBOL")) return 5;

    const Term t2024_1 = Term::parse("2024-1");
    const Term t2024_2 = Term::parse("2024-2");
    if (!expect(t2024_1 < t2024_2 && !(t2024_2 < t2024_1) && t2024_1 == Term::parse("2024-1"))) return 6;
    if (!throwsCode<ParseException>([] { static_cast<void>(Term::parse("0000-1")); }, "E_PARSE")) return 7;
    if (!throwsCode<ParseException>([] { static_cast<void>(Term::parse("2024-4")); }, "E_PARSE")) return 8;
    if (!throwsCode<ParseException>([] { static_cast<void>(Term::parse("24-1")); }, "E_PARSE")) return 9;

    RegularCourse regular("REG", 3, CourseCategory::Core, {}, {});
    SuCourse satisfactory("SU", 0, CourseCategory::MandatorySu, {}, {});
    ThesisCourse thesis("TH", 2, CourseCategory::Open, {}, {});
    regular.validateGrade(Grade::create("A"));
    regular.validateGrade(Grade::create("I"));
    satisfactory.validateGrade(Grade::create("S"));
    satisfactory.validateGrade(Grade::create("X"));
    satisfactory.validateGrade(Grade::create("I"));
    thesis.validateGrade(Grade::create("S"));
    thesis.validateGrade(Grade::create("X"));
    if (!throwsCode<GradeTypeMismatchException>([&] { regular.validateGrade(Grade::create("S")); },
                                                "E_GRADE_TYPE")) return 10;
    if (!throwsCode<GradeTypeMismatchException>([&] { satisfactory.validateGrade(Grade::create("A")); },
                                                "E_GRADE_TYPE")) return 11;
    if (!throwsCode<GradeTypeMismatchException>([&] { thesis.validateGrade(Grade::create("A")); },
                                                "E_GRADE_TYPE")) return 12;
    if (!expect(regular.countsInGpa(Grade::create("A")) &&
                !regular.countsInGpa(Grade::create("I")) &&
                !satisfactory.countsInGpa(Grade::create("S")) &&
                thesis.countsInGpa(Grade::create("D")))) return 13;
    if (!expect(regular.earnsCredits(Grade::create("D")) &&
                !regular.earnsCredits(Grade::create("E")) &&
                satisfactory.earnsCredits(Grade::create("S")) &&
                !satisfactory.earnsCredits(Grade::create("X")) &&
                thesis.earnsCredits(Grade::create("D")) &&
                !thesis.earnsCredits(Grade::create("F")))) return 14;

    Attempt regular_attempt(regular, t2024_1);
    if (!expect(regular_attempt.isOpen() && !regular_attempt.hasFinalGrade() &&
                regular_attempt.finalGrade() == nullptr)) return 15;
    regular_attempt.recordGrade(Grade::create("I"));
    if (!throwsCode<GradeAlreadyFinalException>([&] { regular_attempt.recordGrade(Grade::create("I")); },
                                                "E_GRADE_FINAL")) return 16;
    regular_attempt.recordGrade(Grade::create("A"));
    if (!expect(!regular_attempt.isOpen() && regular_attempt.hasFinalGrade() &&
                regular_attempt.finalGrade()->symbol() == "A")) return 17;
    if (!throwsCode<GradeAlreadyFinalException>([&] { regular_attempt.recordGrade(Grade::create("B")); },
                                                "E_GRADE_FINAL")) return 18;

    Attempt thesis_attempt(thesis, t2024_1);
    if (!expect(thesis_attempt.thesisRecord() != nullptr && thesis_attempt.isOpen())) return 19;
    if (!throwsCode<GradeTypeMismatchException>([&] { thesis_attempt.recordGrade(Grade::create("S")); },
                                                "E_GRADE_TYPE")) return 20;
    thesis_attempt.thesisRecord()->submitProgress(t2024_1, Grade::create("S"));
    if (!expect(!thesis_attempt.hasFinalGrade() && thesis_attempt.isOpen() &&
                thesis_attempt.finalGrade() == nullptr)) return 40;
    thesis_attempt.thesisRecord()->convert(Grade::create("D"));
    if (!expect(thesis_attempt.hasFinalGrade() && !thesis_attempt.isOpen() &&
                thesis_attempt.finalGrade()->symbol() == "D")) return 41;

    ThesisRecord invalid_progress(t2024_1);
    if (!throwsCode<GradeTypeMismatchException>(
            [&] { invalid_progress.submitProgress(t2024_1, Grade::create("A")); }, "E_GRADE_TYPE")) return 42;
    ThesisRecord invalid_conversion(t2024_1);
    invalid_conversion.submitProgress(t2024_1, Grade::create("S"));
    if (!throwsCode<GradeTypeMismatchException>([&] { invalid_conversion.convert(Grade::create("S")); },
                                                "E_GRADE_TYPE")) return 43;

    ThesisRecord lifecycle(t2024_1);
    if (!throwsCode<InvalidThesisTransitionException>(
            [&] { lifecycle.submitProgress(Term::parse("2023-3"), Grade::create("X")); },
            "E_THESIS_TRANSITION")) return 21;
    lifecycle.submitProgress(t2024_1, Grade::create("X"));
    if (!expect(lifecycle.state() == ThesisState::AtRisk)) return 22;
    if (!throwsCode<InvalidThesisTransitionException>([&] { lifecycle.convert(Grade::create("A")); },
                                                       "E_THESIS_TRANSITION")) return 23;
    lifecycle.submitProgress(t2024_2, Grade::create("S"));
    if (!expect(lifecycle.state() == ThesisState::InProgress)) return 24;
    if (!throwsCode<GradeAlreadyFinalException>(
            [&] { lifecycle.submitProgress(t2024_2, Grade::create("X")); }, "E_GRADE_FINAL")) return 25;
    lifecycle.submitProgress(Term::parse("2024-3"), Grade::create("X"));
    lifecycle.submitProgress(Term::parse("2025-1"), Grade::create("X"));
    if (!expect(lifecycle.state() == ThesisState::Terminated)) return 26;
    if (!throwsCode<ThesisTerminatedException>(
            [&] { lifecycle.submitProgress(Term::parse("2025-2"), Grade::create("S")); },
            "E_THESIS_TERMINATED")) return 27;
    if (!throwsCode<ThesisTerminatedException>([&] { lifecycle.convert(Grade::create("A")); },
                                               "E_THESIS_TERMINATED")) return 28;

    ThesisRecord completed(t2024_1);
    completed.submitProgress(t2024_1, Grade::create("S"));
    completed.convert(Grade::create("D"));
    if (!expect(completed.state() == ThesisState::Final && completed.finalLetter().has_value() &&
                completed.finalLetter()->symbol() == "D")) return 29;
    if (!throwsCode<GradeAlreadyFinalException>(
            [&] { completed.submitProgress(t2024_2, Grade::create("S")); }, "E_GRADE_FINAL")) return 30;
    if (!throwsCode<GradeAlreadyFinalException>([&] { completed.convert(Grade::create("A")); },
                                                "E_GRADE_FINAL")) return 31;

    Student student("MODEL", "Test");
    if (!expect(student.latestAttempt("REG") == nullptr && student.latestOpenAttempt("REG") == nullptr &&
                student.countingAttempt("REG") == nullptr && !student.hasOpenAttempt("REG"))) return 32;
    student.addAttempt(std::make_unique<Attempt>(regular, t2024_1), false);
    if (!expect(student.findAttempt("REG", t2024_1) == student.attempts().back().get() &&
                student.findAttempt("REG", t2024_2) == nullptr)) return 44;
    if (!expect(student.latestAttempt("REG") != nullptr && student.latestOpenAttempt("REG") != nullptr &&
                student.hasOpenAttempt("REG") && !student.improvementUsed("REG"))) return 33;
    student.attempts().back()->recordGrade(Grade::create("D"));
    if (!expect(student.countingAttempt("REG") == student.attempts().back().get() &&
                !student.hasOpenAttempt("REG"))) return 34;
    student.addAttempt(std::make_unique<Attempt>(regular, t2024_2), true);
    if (!expect(student.improvementUsed("REG") && student.latestAttempt("REG") == student.attempts().back().get() &&
                student.latestOpenAttempt("REG") == student.attempts().back().get() &&
                student.countingAttempt("REG") == student.attempts().front().get())) return 35;
    student.attempts().back()->recordGrade(Grade::create("F"));
    const Student& const_student = student;
    if (!expect(const_student.latestAttempt("REG") == student.attempts().back().get() &&
                const_student.countingAttempt("REG") == student.attempts().back().get())) return 36;
    Student thesis_student("TH-MODEL", "ThesisTest");
    thesis_student.addAttempt(std::make_unique<Attempt>(thesis, t2024_2), false);
    if (!expect(thesis_student.findAttempt("TH", t2024_1) == thesis_student.attempts().front().get())) return 45;
    if (!throwsCode<InvalidThesisTransitionException>(
            [&] { thesis_student.findAttempt("TH", t2024_1)->thesisRecord()->submitProgress(
                      t2024_1, Grade::create("X")); }, "E_THESIS_TRANSITION")) return 46;

    if (!expect(toString(CourseType::Regular) == "REGULAR" && toString(CourseType::Satisfactory) == "SU" &&
                toString(CourseType::Thesis) == "THESIS")) return 37;
    if (!expect(toString(CourseCategory::Core) == "CORE" && toString(CourseCategory::Elective) == "ELECTIVE" &&
                toString(CourseCategory::MandatorySu) == "MANDATORY_SU" &&
                toString(CourseCategory::Open) == "OPEN")) return 38;
    if (!expect(toString(ThesisState::Registered) == "REGISTERED" &&
                toString(ThesisState::InProgress) == "IN_PROGRESS" &&
                toString(ThesisState::AtRisk) == "AT_RISK" &&
                toString(ThesisState::Terminated) == "TERMINATED" &&
                toString(ThesisState::Final) == "FINAL")) return 39;

    return 0;
}
