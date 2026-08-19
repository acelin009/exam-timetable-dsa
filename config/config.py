"""Configuration settings for the exam timetable generator."""

from datetime import datetime, timedelta

# ============================================================
# DATE/TIME CONFIGURATION
# ============================================================

# Starting date for the examination period
EXAM_START_DATE = datetime(2026, 5, 18)  # Monday

# Available exam days
EXAM_DAYS = [
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday"
]

# Slots per day (time slots)
SLOTS_PER_DAY = {
    0: "09:00",
    1: "14:00",
}

# Maximum exams per student per day
MAX_EXAMS_PER_DAY = 2

# Minimum gap between exams (in hours)
MIN_GAP_HOURS = 4

# ============================================================
# ALGORITHM CONFIGURATION
# ============================================================

# Maximum number of slots available
MAX_SLOTS = len(EXAM_DAYS) * len(SLOTS_PER_DAY)

# DSATUR tie-breaking: 'enrollment' or 'conflict_degree'
DSATUR_TIE_BREAKER = "enrollment"

# Enable backtracking
ENABLE_BACKTRACKING = True

# Backtracking recursion limit
BACKTRACKING_LIMIT = 10000

# ============================================================
# SOFT CONSTRAINT WEIGHTS
# ============================================================

# Weights for soft constraint penalties
SOFT_CONSTRAINT_WEIGHTS = {
    "same_day_concentration": 5,    # Penalty per extra exam beyond 2
    "uneven_distribution": 10,      # Penalty for imbalance across days
    "consecutive_exams": 20,        # Penalty per consecutive exam pair
    "unused_slots": 2,              # Penalty per unused slot
    "exam_gap": 15,                  # Penalty for exams too close
}

# ============================================================
# VISUALIZATION
# ============================================================

# Enable graph visualization
ENABLE_VISUALIZATION = False  # Set to True if networkx/matplotlib installed

# ============================================================
# FILE PATHS
# ============================================================

DATA_DIR = "data"
OUTPUT_DIR = "output"