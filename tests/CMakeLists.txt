cmake_minimum_required(VERSION 3.10)

# Buffer performance test executable
add_executable(buffer_performance_test
    buffer_performance_test.cpp
    ${CMAKE_SOURCE_DIR}/src/core/gap_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/sqrt_decomposition.cpp
    ${CMAKE_SOURCE_DIR}/src/core/rope.cpp
    ${CMAKE_SOURCE_DIR}/src/core/piece_table.cpp
)

target_include_directories(buffer_performance_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pnana
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(buffer_performance_test PRIVATE cxx_std_17)

set_target_properties(buffer_performance_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Add convenience target for running the test
add_custom_target(run_buffer_perf_test
    COMMAND buffer_performance_test
    DEPENDS buffer_performance_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running buffer performance benchmark..."
)

# Myers Diff performance test executable
add_executable(myers_diff_performance_test
    myers_diff_performance_test.cpp
    ${CMAKE_SOURCE_DIR}/src/features/diff/myers_diff.cpp
)

target_include_directories(myers_diff_performance_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pnana
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(myers_diff_performance_test PRIVATE cxx_std_17)

set_target_properties(myers_diff_performance_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Add convenience target for running the test
add_custom_target(run_myers_diff_perf_test
    COMMAND myers_diff_performance_test
    DEPENDS myers_diff_performance_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Myers diff performance benchmark..."
)

# Bracket match & highlight performance test executable
add_executable(bracket_match_highlight_perf_test
    bracket_match_highlight_perf_test.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/bracket_matcher.cpp
)

target_include_directories(bracket_match_highlight_perf_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pnana
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(bracket_match_highlight_perf_test PRIVATE cxx_std_17)

set_target_properties(bracket_match_highlight_perf_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Add convenience target for running the test
add_custom_target(run_bracket_match_highlight_perf_test
    COMMAND bracket_match_highlight_perf_test
    DEPENDS bracket_match_highlight_perf_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running bracket match & highlight performance benchmark..."
)
