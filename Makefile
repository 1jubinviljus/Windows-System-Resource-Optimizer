CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src/data_collection -I./src/resource_management
LDFLAGS = -lpsapi -lsqlite3

# Source directories
DATA_COLLECTION_DIR = src/data_collection
RESOURCE_MANAGEMENT_DIR = src/resource_management

# Object files
OBJS = $(DATA_COLLECTION_DIR)/main.o \
       $(DATA_COLLECTION_DIR)/system_metrics.o \
       $(DATA_COLLECTION_DIR)/process_metrics.o \
       $(DATA_COLLECTION_DIR)/sqlite3.o \
       $(RESOURCE_MANAGEMENT_DIR)/process_optimizer.o

# Target executable
TARGET = system_monitor.exe

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile data collection objects
$(DATA_COLLECTION_DIR)/main.o: $(DATA_COLLECTION_DIR)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DATA_COLLECTION_DIR)/system_metrics.o: $(DATA_COLLECTION_DIR)/system_metrics.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DATA_COLLECTION_DIR)/process_metrics.o: $(DATA_COLLECTION_DIR)/process_metrics.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DATA_COLLECTION_DIR)/sqlite3.o: $(DATA_COLLECTION_DIR)/sqlite3.c
	$(CC) $(CFLAGS) -DSQLITE_ENABLE_FTS5 -c $< -o $@

# Compile resource management objects
$(RESOURCE_MANAGEMENT_DIR)/process_optimizer.o: $(RESOURCE_MANAGEMENT_DIR)/process_optimizer.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	del /Q $(DATA_COLLECTION_DIR)\*.o
	del /Q $(RESOURCE_MANAGEMENT_DIR)\*.o
	del /Q $(TARGET)

# Install dependencies (if needed)
install-deps:
	pip install -r requirements.txt

# Run the system
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install-deps run 