<!-- group-73 -->

# AcadERP: Academic Records and Graduation Audit System

**Group ID:** 73  
**Student roll number:** MT26056

AcadERP is a command-driven C++17 academic records system. It supports regular, satisfactory (SU), and thesis courses; student registration and grade submission; and the GPA, average-grade, graduation, eligibility, and transcript queries defined in `OOPD_26_ERP_Project.pdf`.

The program reads one command per line from standard input and writes query results and fixed-format command errors to standard output. Human-readable exception messages are written to standard error. A failed command is handled at the command loop, and processing continues with the next input line.

## Build and run

### Requirements

- A C++17-capable `g++` compiler
- GNU Make
- A POSIX-compatible shell

Build the executable:

```sh
make
```

This creates `acaderp` in the project directory. Run a command script through standard input:

```sh
./acaderp < input.txt
```

The equivalent warning-clean compiler command required by the project brief is:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic -Werror \
    src/grp_73_main.cpp src/grp_73_CommandParser.cpp src/grp_73_Model.cpp src/grp_73_Registrar.cpp \
    -Isrc -o acaderp
```

Remove the generated executable with:

```sh
make clean
```

## Tests

Run the complete included test suite from this directory:

```sh
./run_tests.sh
```

or:

```sh
make test
```

The script builds `acaderp`, runs all **forty-three command-script pairs** (`tests/T01.in` through `tests/T43.in`), and compares standard output with each corresponding `.out` file using `diff`. It normalizes a trailing carriage return in generated output so the same comparisons work with Linux LF output and MSYS2/Windows CRLF output. It then compiles and runs two independent C++ unit tests. The expected total is **45 checks**: forty-three command-level regression tests, one CourseMaterial deep-copy test, and one direct model-contract test.

The command-level tests cover grade validation and replacement, every average-grade band and its inclusive lower boundary, exact GPA rounding, no-GPA behavior, thesis progress ordering and state transitions, pre-registration progress rejection, final-state immutability, repeated attempts and one-time grade improvement, all E1 to E7 eligibility checks and corresponding typed registration errors, graduation rules, transcript output for regular/SU/thesis states, malformed commands and course definitions, registration rollback, and default PROGRAM values. The direct `grp_73_model_contract_test.cpp` exercises Grade values and factories, Term parsing and ordering, course-specific grade/GPA/credit rules, Attempt behavior, ThesisRecord transitions, Student attempt indexing, and enum string conversions. In particular:

- `T13` checks that a thesis registration rejected by eligibility does not create a registration visible in the transcript.
- `T14` checks that a negative GPA threshold is rejected as a parse/input error and that processing continues with subsequent commands.
- `T15` checks that a grade aimed at a terminated thesis remains an `E_THESIS_TERMINATED` error after a later thesis retry has been registered.
- `T16` checks that a successful regular-course improvement cannot be used a second time.
- `T17` checks that another thesis being open or passed blocks a new thesis registration under E7.
- `T18` checks that a thesis contributes to the term credit load only in its start term.
- `T19` checks `AVG` without GPA data and a complete successful graduation audit.
- `T20` exercises typed `REGISTER` exceptions and verifies that rejected registrations do not appear in transcripts.
- `T21` checks duplicate entities, invalid course definitions, and malformed command syntax.
- `T22` checks all nine average-grade outcomes; `T23` checks the inclusive lower edges of all average-grade bands and the half-up rounding boundary `8.495` to `8.50`.
- `T24` checks conversion before registration, invalid thesis grades, final-state immutability, conversion of a non-thesis, and transcript conversion history.
- `T25` verifies that failed conversions from `REGISTERED` and `AT_RISK` do not change thesis state or progress history.
- `T26` and `T27` exercise PROGRAM parsing/value errors and prerequisite-definition validation; `T28` covers anti-requisite-definition errors.
- `T29` checks the default `S` prerequisite rule for SU courses; `T30` checks invalid term formats.
- `T31` covers unknown entities, missing registrations, invalid grade symbols, and course-type mismatches.
- `T32` verifies that an `I` can be replaced once and that final regular/SU grades cannot be changed.
- `T33` covers thesis progress before registration (`E_THESIS_TRANSITION`), one-grade-per-term enforcement, chronological ordering, termination, and rejection after termination.
- `T34` checks transcript status/history for pending, incomplete, final, registered, at-risk, and terminated attempts across separate students.
- `T35` checks E2/E7 eligibility output and the matching typed `REGISTER` exceptions; `T36` tests prerequisite default and explicit grade thresholds.
- `T37` checks that the counting attempt is the latest final attempt; `T38` checks exact term-load acceptance and one-credit-over rejection.
- `T39` isolates graduation failures R3, R4, and R5; `T40` tests the rounded GPA graduation threshold and no-GPA failure; `T41` tests R7 for pending/incomplete regular/SU attempts and distinguishes open theses; `T42` checks transcript histories and GPA after regular, SU, and thesis outcomes.
- `T43` checks the default PROGRAM thresholds for total credits, elective credits, minimum GPA, thesis grade/credits/GPA, and maximum term load.

## Design and implementation

- **Course polymorphism:** `Course` is abstract. `RegularCourse`, `SuCourse`, and `ThesisCourse` implement course-specific grade validation, GPA inclusion, and credit-earning behavior. The registrar delegates these rules to the course objects.
- **Immutable grades:** `Grade` is created through a factory that validates the exact, case-sensitive grade symbols and assigns their point values. Grade history is kept on each `Attempt`; an `I` may be followed by one final replacement where the course type allows it.
- **Exact GPA arithmetic:** GPA calculations use integer grade points and credits. The result is rounded half-up to two decimal places using integer arithmetic, as specified in the brief. Floating-point arithmetic is not used in the GPA calculation. For example, `65/8 = 8.125` is printed as `8.13`.
- **Thesis lifecycle encapsulation:** `ThesisRecord` owns thesis progress, conversion, and state transitions. A thesis is registered once; later progress submissions are associated with that registration and must obey the term-order and state rules. Progress grades do not enter GPA or earned credits before conversion to a letter grade.
- **Registrar facade:** `Registrar` coordinates registration, grade submission, thesis conversion, eligibility, GPA, average-grade, graduation, and transcript operations while keeping the command loop separate from domain rules.
- **Typed exceptions:** Domain errors derive from `ErpException` and carry fixed error codes. The command loop catches `ErpException` by reference for each command, prints `ERROR <line> <CODE>` to standard output, writes the explanatory message to standard error, and continues processing.
- **Counting attempts:** For each course, the counting attempt is the latest attempt that has a final grade. GPA, earned credits, prerequisites, and graduation use that attempt. A later failed attempt is not replaced by an earlier pass.
- **Course materials:** `CourseMaterial` is an abstract cloneable type. A course exposes its own materials for setup and `teachingCopy()` returns a deep copy of the syllabus, lecture notes, and assignments so an instructor can modify a copy without changing the original. `tests/grp_73_material_clone_test.cpp` populates a course's materials, clones them, mutates the copy, and checks that the original is unchanged.
- **Transcript format:** `TRANSCRIPT` produces one output line. Its format is `TRANSCRIPT <roll> | <course> <term> <type> <credits> <category> <status> <history> ...`. Regular and SU histories are comma-separated grade symbols. Thesis histories are comma-separated `term:symbol` entries, followed by `CONVERT:<letter>` when converted. An empty history is shown as `-`.

## Assumptions

The project brief leaves some details open. The implementation uses the following rules:

- `PROGRAM` is optional. If present, it can override any subset of the seven settings; omitted settings keep their defaults. Multiple `PROGRAM` commands may update settings until the first `COURSE` is defined. A later `PROGRAM` command is rejected with `E_PARSE`.
- Credits and configured credit thresholds must be non-negative. Per-course credits must be in the specified range of 1 through 24, except that an SU course may have zero credits. GPA settings must be between `0.00` and `10.00`, with exactly two decimal digits.
- Terms use exactly four year digits and one semester digit (`YYYY-1`, `YYYY-2`, or `YYYY-3`); year `0000` is rejected. Thesis progress may be submitted in its registration term and then only in strictly later terms. A second progress submission in the same term is rejected with `E_GRADE_FINAL`.
- A prerequisite's default minimum grade is `D` for regular and thesis courses, and `S` for an SU course. An explicit minimum must be a letter grade for regular/thesis courses or `S` for an SU course. Prerequisite and anti-requisite courses must already exist; self-references and duplicate entries are rejected as course-definition errors.
- A regular or SU `I` grade can be replaced once by a final grade. Replacing it with another `I` is rejected with `E_GRADE_FINAL`. A final grade cannot be changed.
- Repeated registrations are ordered by the order of accepted `REGISTER` commands. A regular or SU course may be registered again after a failed final grade, or for the one permitted regular-course improvement. A thesis that reaches `TERMINATED` is treated as a failed attempt and may be registered again. When multiple regular/SU attempts for the same course and term exist, a later `GRADE` command targets the newest matching registration first. For thesis progress, the command's term is associated with the newest thesis attempt whose start term is not later than that progress term; this preserves the terminated-attempt error for an earlier thesis timeline after a later retry is registered.
- Registration load is calculated from the credits of attempts already registered in that term. A thesis contributes to the load in its start term only.
- The thesis graduation requirement is met when a thesis has a counting final letter at or above `thesis_min_grade`. A terminated thesis has no final letter and does not satisfy this requirement.
- A passed SU course earns its catalogue credits but never contributes to GPA. A failed regular course or converted thesis remains in GPA, including an `F` worth zero points.
- `ELIGIBLE` throws the appropriate E1 exception for an unknown student or course. For E2 through E7, it reports the first failure as `NO E<n>` rather than throwing. `REGISTER` throws the matching typed exception.

## Performance

Students and courses are stored in ordered maps, giving logarithmic entity lookup in the number of records. Each student also maintains a per-course attempt index. Finding the counting attempt therefore requires an indexed lookup plus a scan of retries for that course, rather than scanning every attempt belonging to the student. GPA and graduation scan the course catalogue and use those indexed lookups; transcript generation scans the student's attempts once. Prerequisite checks scale with the number of prerequisites on the requested course.

For a workload of approximately 3,000 students, the program processes each input line immediately and retains the current model in memory instead of repeatedly loading or rewriting the entire student population. The dominant cost is proportional to the number of GPA/graduation-style queries multiplied by the course-catalogue size, with additional work for retries and prerequisite lists. No benchmark has been run for this implementation, so the one-hour performance target has not been empirically demonstrated. If catalogue size or query volume grows substantially, per-student summaries could be cached and updated after successful mutations; such a cache should be checked against the uncached calculations with regression tests.
