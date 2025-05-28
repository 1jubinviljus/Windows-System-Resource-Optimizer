cmake_minimum_required(VERSION 3.10)
project(WindowsSystemResourceOptimizer C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/data_collection/main.c
    src/data_collection/system_metrics.c
    src/data_collection/process_metrics.c
    src/data_collection/sqlite3.c
)

# Add header files
set(HEADERS
    src/data_collection/system_metrics.h
    src/data_collection/process_metrics.h
    src/data_collection/sqlite3.h
)

# Create executable
add_executable(system_monitor ${SOURCES} ${HEADERS})

# Add Windows-specific libraries
target_link_libraries(system_monitor
    pdh
    psapi
    wbemuuid
)

# Include directories
target_include_directories(system_monitor PRIVATE
    ${CMAKE_SOURCE_DIR}/src/data_collection
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 