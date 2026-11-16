// group-73
#include "grp_73_Model.h"

int main() {
    acaderp::RegularCourse course("COPY101", 3, acaderp::CourseCategory::Open, {}, {});
    acaderp::CourseMaterial& original = course.materials();
    original.setSyllabus("original syllabus");
    original.addLectureNote("lecture 1");
    original.addAssignment("assignment 1");

    std::unique_ptr<acaderp::CourseMaterial> first_copy = course.teachingCopy();
    std::unique_ptr<acaderp::CourseMaterial> second_copy = course.teachingCopy();
    if (first_copy == nullptr || second_copy == nullptr || first_copy.get() == second_copy.get() ||
        dynamic_cast<acaderp::TeachingMaterials*>(first_copy.get()) == nullptr ||
        dynamic_cast<acaderp::TeachingMaterials*>(second_copy.get()) == nullptr) {
        return 1;
    }

    first_copy->setSyllabus("instructor one syllabus");
    first_copy->addLectureNote("lecture 2");
    first_copy->addAssignment("assignment 2");
    second_copy->setSyllabus("instructor two syllabus");

    if (course.materials().syllabus() != "original syllabus" ||
        course.materials().lectureNotes().size() != 1U ||
        course.materials().assignments().size() != 1U) {
        return 1;
    }
    if (first_copy->syllabus() != "instructor one syllabus" ||
        first_copy->lectureNotes().size() != 2U || first_copy->assignments().size() != 2U) {
        return 1;
    }
    if (second_copy->syllabus() != "instructor two syllabus" ||
        second_copy->lectureNotes().size() != 1U || second_copy->assignments().size() != 1U) {
        return 1;
    }
    return 0;
}
