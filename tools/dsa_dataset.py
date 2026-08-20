import random
import os
from collections import defaultdict
import pandas as pd

# ============================================================
# CONFIGURATION
# ============================================================

RANDOM_SEED = 42
random.seed(RANDOM_SEED)

OUTPUT_DIR = "../data"  # write straight into the C project's data/ folder

# Number of students in each class
STUDENTS_PER_CLASS = 20

# Probability that a student takes an Honors subject
HONORS_PROBABILITY = 0.40


# ============================================================
# 1. DEFINE CLASSES AND CORE SUBJECTS
# ============================================================

# Deliberately share some subjects between classes.
# This creates cross-class conflicts.

class_core_subjects = {
    "A": [
        "MAT",
        "DB",
        "CN"
    ],

    "B": [
        "MAT",
        "AI",
        "SE"
    ],

    "C": [
        "MAT",
        "AI",
        "CN"
    ]
}


# ============================================================
# 2. DEFINE HONORS SUBJECTS
# ============================================================

honors_subjects = [
    "AI_HONORS",
    "DATA_SCIENCE_HONORS",
    "CYBERSECURITY_HONORS"
]


# ============================================================
# 3. DEFINE ELECTIVES
# ============================================================

# Every student chooses exactly ONE elective.

elective_subjects = [
    "CLOUD",
    "DATA_MINING",
    "BLOCKCHAIN"
]


# ============================================================
# 4. SUBJECT INFORMATION
# ============================================================

subject_names = {

    "MAT": "Mathematics",
    "DB": "Database Systems",
    "CN": "Computer Networks",
    "AI": "Artificial Intelligence",
    "SE": "Software Engineering",

    "AI_HONORS": "Artificial Intelligence Honors",
    "DATA_SCIENCE_HONORS": "Data Science Honors",
    "CYBERSECURITY_HONORS": "Cybersecurity Honors",

    "CLOUD": "Cloud Computing",
    "DATA_MINING": "Data Mining",
    "BLOCKCHAIN": "Blockchain Technology"
}


subject_types = {

    "MAT": "Core",
    "DB": "Core",
    "CN": "Core",
    "AI": "Core",
    "SE": "Core",

    "AI_HONORS": "Honors",
    "DATA_SCIENCE_HONORS": "Honors",
    "CYBERSECURITY_HONORS": "Honors",

    "CLOUD": "Elective",
    "DATA_MINING": "Elective",
    "BLOCKCHAIN": "Elective"
}


# ============================================================
# 5. CREATE OUTPUT DIRECTORY
# ============================================================

os.makedirs(OUTPUT_DIR, exist_ok=True)


# ============================================================
# 6. CREATE STUDENTS
# ============================================================

students = []

for class_name in ["A", "B", "C"]:

    for number in range(1, STUDENTS_PER_CLASS + 1):

        student_id = f"{class_name}{number:02d}"

        students.append({
            "student_id": student_id,
            "class": class_name
        })


# ============================================================
# 7. ASSIGN SUBJECTS TO STUDENTS
# ============================================================

student_subject_records = []

for student in students:

    student_id = student["student_id"]
    class_name = student["class"]

    # --------------------------------------------------------
    # CORE SUBJECTS
    # --------------------------------------------------------

    core_subjects = class_core_subjects[class_name]

    for subject in core_subjects:

        student_subject_records.append({
            "student_id": student_id,
            "class": class_name,
            "subject_id": subject,
            "subject_name": subject_names[subject],
            "subject_type": "Core"
        })

    # --------------------------------------------------------
    # HONORS SUBJECT
    # --------------------------------------------------------

    # Some students don't take Honors.

    if random.random() < HONORS_PROBABILITY:

        honors = random.choice(honors_subjects)

        student_subject_records.append({
            "student_id": student_id,
            "class": class_name,
            "subject_id": honors,
            "subject_name": subject_names[honors],
            "subject_type": "Honors"
        })

    # --------------------------------------------------------
    # ELECTIVE
    # --------------------------------------------------------

    # Every student chooses exactly ONE.

    elective = random.choice(elective_subjects)

    student_subject_records.append({
        "student_id": student_id,
        "class": class_name,
        "subject_id": elective,
        "subject_name": subject_names[elective],
        "subject_type": "Elective"
    })


# ============================================================
# 8. CREATE STUDENTS DATASET
# ============================================================

students_df = pd.DataFrame(students)

students_df.to_csv(
    os.path.join(OUTPUT_DIR, "students.csv"),
    index=False
)


# ============================================================
# 9. CREATE SUBJECTS DATASET
# ============================================================

subjects = []

for subject_id in subject_names:

    subjects.append({
        "subject_id": subject_id,
        "subject_name": subject_names[subject_id],
        "subject_type": subject_types[subject_id]
    })


subjects_df = pd.DataFrame(subjects)

subjects_df.to_csv(
    os.path.join(OUTPUT_DIR, "subjects.csv"),
    index=False
)


# ============================================================
# 10. CREATE STUDENT-SUBJECT DATASET
# ============================================================

student_subjects_df = pd.DataFrame(
    student_subject_records
)

student_subjects_df.to_csv(
    os.path.join(OUTPUT_DIR, "student_subjects.csv"),
    index=False
)


# ============================================================
# 11. CREATE CLASS-SUBJECT DATASET
# ============================================================

class_subject_records = []

for class_name, subjects_list in class_core_subjects.items():

    for subject in subjects_list:

        class_subject_records.append({
            "class": class_name,
            "subject_id": subject,
            "subject_name": subject_names[subject],
            "subject_type": "Core"
        })


# Find Honors and electives actually taken by each class.

for class_name in ["A", "B", "C"]:

    class_students = students_df[
        students_df["class"] == class_name
    ]["student_id"].tolist()

    class_records = student_subjects_df[
        student_subjects_df["student_id"].isin(class_students)
    ]

    for _, row in class_records.iterrows():

        if row["subject_type"] != "Core":

            record = {
                "class": class_name,
                "subject_id": row["subject_id"],
                "subject_name": row["subject_name"],
                "subject_type": row["subject_type"]
            }

            # Avoid duplicates
            if record not in class_subject_records:
                class_subject_records.append(record)


class_subjects_df = pd.DataFrame(
    class_subject_records
)

class_subjects_df.to_csv(
    os.path.join(OUTPUT_DIR, "class_subjects.csv"),
    index=False
)


# ============================================================
# 12. CALCULATE SUBJECT ENROLLMENT
# ============================================================

enrollment = (
    student_subjects_df
    .groupby(
        ["subject_id", "subject_name", "subject_type"]
    )
    .size()
    .reset_index(name="number_of_students")
)

enrollment = enrollment.sort_values(
    "number_of_students",
    ascending=False
)

enrollment.to_csv(
    os.path.join(OUTPUT_DIR, "subject_enrollment.csv"),
    index=False
)


# ============================================================
# 13. BUILD CONFLICT GRAPH
# ============================================================

# If a student takes Subject A and Subject B,
# then A and B cannot happen at the same time.

conflict_students = defaultdict(set)

for student_id, group in student_subjects_df.groupby(
    "student_id"
):

    subjects_taken = group["subject_id"].tolist()

    for i in range(len(subjects_taken)):

        for j in range(i + 1, len(subjects_taken)):

            subject_a = subjects_taken[i]
            subject_b = subjects_taken[j]

            # Sort so A-B and B-A are treated as the same pair.
            pair = tuple(
                sorted([subject_a, subject_b])
            )

            conflict_students[pair].add(student_id)


# ============================================================
# 14. CREATE CONFLICT DATASET
# ============================================================

conflict_records = []

for (subject_a, subject_b), students_in_conflict in conflict_students.items():

    conflict_records.append({

        "subject_1": subject_a,

        "subject_1_name":
            subject_names[subject_a],

        "subject_2": subject_b,

        "subject_2_name":
            subject_names[subject_b],

        "number_of_conflicting_students":
            len(students_in_conflict),

        "conflicting_students":
            ",".join(
                sorted(students_in_conflict)
            )
    })


conflicts_df = pd.DataFrame(
    conflict_records
)


# Sort strongest conflicts first.

if not conflicts_df.empty:

    conflicts_df = conflicts_df.sort_values(
        "number_of_conflicting_students",
        ascending=False
    )


conflicts_df.to_csv(
    os.path.join(
        OUTPUT_DIR,
        "subject_conflicts.csv"
    ),
    index=False
)


# ============================================================
# 15. CREATE DATASET SUMMARY
# ============================================================

number_of_students = len(students_df)

number_of_subjects = len(subjects_df)

number_of_registrations = len(
    student_subjects_df
)

number_of_conflicts = len(
    conflicts_df
)


# Number of conflicts involving each subject.

conflict_degree = defaultdict(int)

for _, row in conflicts_df.iterrows():

    conflict_degree[row["subject_1"]] += 1
    conflict_degree[row["subject_2"]] += 1


most_connected_subject = None

if conflict_degree:

    most_connected_subject = max(
        conflict_degree,
        key=conflict_degree.get
    )


summary = f"""
EXAM TIMETABLE DATASET
======================

Random Seed:
{RANDOM_SEED}

Classes:
3

Students per Class:
{STUDENTS_PER_CLASS}

Total Students:
{number_of_students}

Total Subjects:
{number_of_subjects}

Total Student-Subject Registrations:
{number_of_registrations}

Total Subject Conflicts:
{number_of_conflicts}

Most Connected Subject:
{most_connected_subject}

Subjects
--------
Core:
Mathematics
Database Systems
Computer Networks
Artificial Intelligence
Software Engineering

Honors:
Artificial Intelligence Honors
Data Science Honors
Cybersecurity Honors

Electives:
Cloud Computing
Data Mining
Blockchain Technology

Rules Used
----------

1. Every student belongs to exactly one class.

2. Every student takes exactly three compulsory subjects.

3. Every student chooses exactly one elective.

4. A student has a 40% probability of taking one Honors subject.

5. A student can take only one Honors subject.

6. If two subjects have at least one student in common,
   those subjects cannot be scheduled in the same exam slot.

7. Every subject is assumed to have one common exam
   for all students registered for that subject.

Generated Files
---------------

students.csv
    Student ID and class.

subjects.csv
    Complete subject list and subject types.

student_subjects.csv
    Which subjects each student takes.

class_subjects.csv
    Subjects associated with each class.

subject_enrollment.csv
    Number of students taking each subject.

subject_conflicts.csv
    Subject pairs that cannot occur simultaneously,
    including the number of students causing each conflict.
"""


with open(
    os.path.join(
        OUTPUT_DIR,
        "dataset_summary.txt"
    ),
    "w",
    encoding="utf-8"
) as file:

    file.write(summary)


# ============================================================
# 16. PRINT RESULTS
# ============================================================

print()
print("=" * 60)
print("EXAM TIMETABLE DATASET GENERATED")
print("=" * 60)

print()

print(f"Students:       {number_of_students}")
print(f"Subjects:       {number_of_subjects}")
print(f"Registrations:  {number_of_registrations}")
print(f"Conflicts:      {number_of_conflicts}")

print()

print("Files created:")

for filename in sorted(os.listdir(OUTPUT_DIR)):

    print(
        f"  ✓ {filename}"
    )

print()

print("Top 10 subject conflicts:")

if not conflicts_df.empty:

    print(
        conflicts_df[
            [
                "subject_1",
                "subject_2",
                "number_of_conflicting_students"
            ]
        ]
        .head(10)
        .to_string(index=False)
    )

print()

print(f"Dataset saved in: ./{OUTPUT_DIR}/")

print()