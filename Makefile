CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Isrc
SRC_DIR = src
BUILD_DIR = build

SOURCES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

.PHONY: all clean test run

all: exam_timetable

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

exam_timetable: $(BUILD_DIR)/main.o $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_main.c $(OBJECTS) -o $(BUILD_DIR)/test_main
	./$(BUILD_DIR)/test_main

run: exam_timetable
	./exam_timetable

clean:
	rm -rf $(BUILD_DIR) exam_timetable
