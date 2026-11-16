// group-73
#ifndef ACADERP_ERP_EXCEPTION_HPP
#define ACADERP_ERP_EXCEPTION_HPP

#include <stdexcept>
#include <string>
#include <utility>

namespace acaderp {

class ErpException : public std::runtime_error {
public:
    ErpException(std::string code, std::string message)
        : std::runtime_error(std::move(message)), code_(std::move(code)) {}

    const std::string& code() const noexcept { return code_; }

private:
    std::string code_;
};

class InputException : public ErpException {
protected:
    InputException(std::string code, std::string message)
        : ErpException(std::move(code), std::move(message)) {}
};

class EntityException : public ErpException {
protected:
    EntityException(std::string code, std::string message)
        : ErpException(std::move(code), std::move(message)) {}
};

class GradeException : public ErpException {
protected:
    GradeException(std::string code, std::string message)
        : ErpException(std::move(code), std::move(message)) {}
};

class RegistrationException : public ErpException {
protected:
    RegistrationException(std::string code, std::string message)
        : ErpException(std::move(code), std::move(message)) {}
};

class ComputationException : public ErpException {
protected:
    ComputationException(std::string code, std::string message)
        : ErpException(std::move(code), std::move(message)) {}
};

#define ACADERP_EXCEPTION(name, base, error_code, default_message)          \
    class name final : public base {                                       \
    public:                                                                \
        explicit name(std::string message = default_message)               \
            : base(error_code, std::move(message)) {}                       \
    }

ACADERP_EXCEPTION(ParseException, InputException, "E_PARSE", "invalid input");
ACADERP_EXCEPTION(DuplicateEntityException, EntityException, "E_DUPLICATE", "entity already exists");
ACADERP_EXCEPTION(UnknownStudentException, EntityException, "E_UNKNOWN_STUDENT", "unknown student");
ACADERP_EXCEPTION(UnknownCourseException, EntityException, "E_UNKNOWN_COURSE", "unknown course");
ACADERP_EXCEPTION(InvalidCourseException, EntityException, "E_COURSE_DEF", "invalid course definition");
ACADERP_EXCEPTION(InvalidGradeSymbolException, GradeException, "E_GRADE_SYMBOL", "invalid grade symbol");
ACADERP_EXCEPTION(GradeTypeMismatchException, GradeException, "E_GRADE_TYPE", "grade is not valid for this course type");
ACADERP_EXCEPTION(NotRegisteredException, GradeException, "E_NOT_REGISTERED", "no matching registration");
ACADERP_EXCEPTION(GradeAlreadyFinalException, GradeException, "E_GRADE_FINAL", "grade is already final");
ACADERP_EXCEPTION(InvalidThesisTransitionException, GradeException, "E_THESIS_TRANSITION", "invalid thesis transition");
ACADERP_EXCEPTION(ThesisTerminatedException, GradeException, "E_THESIS_TERMINATED", "thesis attempt is terminated");
ACADERP_EXCEPTION(AlreadyPassedException, RegistrationException, "E_ALREADY_PASSED", "course is already passed");
ACADERP_EXCEPTION(DuplicateRegistrationException, RegistrationException, "E_DUP_REG", "course already has an open registration");
ACADERP_EXCEPTION(PrerequisiteNotMetException, RegistrationException, "E_PREREQ", "prerequisite is not met");
ACADERP_EXCEPTION(AntiRequisiteException, RegistrationException, "E_ANTIREQ", "an anti-requisite is already passed");
ACADERP_EXCEPTION(CreditLimitExceededException, RegistrationException, "E_CREDIT_LIMIT", "term credit limit would be exceeded");
ACADERP_EXCEPTION(ThesisNotAllowedException, RegistrationException, "E_THESIS_NOT_ALLOWED", "thesis eligibility requirements are not met");
ACADERP_EXCEPTION(NoGpaDataException, ComputationException, "E_NO_GPA", "no GPA-eligible grades are available");

#undef ACADERP_EXCEPTION

}  // namespace acaderp

#endif  // ACADERP_ERP_EXCEPTION_HPP
