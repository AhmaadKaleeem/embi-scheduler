################################################################################
# CMakeLists.txt — EMBI Event-Driven CPU Scheduling Simulator
#
# Supports:
#   cmake -DCMAKE_BUILD_TYPE=Debug      ..   (debug + assertions)
#   cmake -DCMAKE_BUILD_TYPE=Release    ..   (full optimizations)
#   cmake -DCMAKE_BUILD_TYPE=Asan       ..   (AddressSanitizer + UBSan)
#   cmake -DCMAKE_BUILD_TYPE=Tsan       ..   (ThreadSanitizer)
#   cmake -DCMAKE_BUILD_TYPE=Coverage   ..   (gcov/lcov coverage)
#
# Build:   cmake --build . --config Release
# Test:    ctest --output-on-failure
# Docs:    cmake --build . --target docs
################################################################################

cmake_minimum_required(VERSION 3.20)

project(EMBISchedulingSimulator
    VERSION     1.0.0
    DESCRIPTION "Event-driven CPU scheduling simulator"
    HOMEPAGE_URL "https://github.com/ahmaadkaleeem/embi-simulator"
    LANGUAGES   CXX
)

# ─── Language standard ──────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ─── Default build type ─────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel Asan Tsan Coverage)
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# ─── Compiler flags ─────────────────────────────────────────────────────────
if(MSVC)
    # MSVC: warnings as errors, C++17 conformance
    add_compile_options(
        /W4
        /WX
        /utf-8
        /permissive-
        /Zc:__cplusplus
        /wd4324   # structure padded due to alignment specifier (expected)
    )
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # GCC / Clang
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wshadow
        -Wno-unknown-pragmas
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)
    endif()

    # Sanitizer / coverage build types
    if(CMAKE_BUILD_TYPE STREQUAL "Asan")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1)
        add_link_options(-fsanitize=address,undefined)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Tsan")
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g -O1)
        add_link_options(-fsanitize=thread)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

# ─── Dependencies (FetchContent) ─────────────────────────────────────────────
include(FetchContent)

# GoogleTest ─────────────────────────────────────────────────────────────────
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
# Prevent GTest from hijacking our install target
set(INSTALL_GTEST       OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# nlohmann/json ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ─── Source files ────────────────────────────────────────────────────────────
set(EMBI_CORE_SOURCES
    src/core/Config.cpp
    src/core/EventQueue.cpp
    src/core/LockManager.cpp
    src/core/Process.cpp
    src/core/OnlineMetrics.cpp
    src/core/OfflineMetrics.cpp
    src/core/EventLoop.cpp
    src/core/Simulator.cpp
    src/core/Experiment.cpp
)

set(EMBI_SCHEDULER_SOURCES
    src/schedulers/BaseScheduler.cpp
    src/schedulers/EMBIScheduler.cpp
    src/schedulers/MaxWeightScheduler.cpp
    src/schedulers/CmuScheduler.cpp
    src/schedulers/RoundRobinScheduler.cpp
    src/schedulers/FCFSScheduler.cpp
)

set(EMBI_WORKLOAD_SOURCES
    src/workloads/UniformWorkload.cpp
    src/workloads/PoissonWorkload.cpp
    src/workloads/BurstyWorkload.cpp
    src/workloads/HeavyTailWorkload.cpp
    src/workloads/LockContentionWorkload.cpp
    src/workloads/TraceLoader.cpp
    src/workloads/WorkloadProfile.cpp
)

set(EMBI_LOGGING_SOURCES
    src/logging/CSVLogger.cpp
    src/logging/BinaryLogger.cpp
    src/logging/Statistics.cpp
    src/logging/StatisticsDatabase.cpp
    src/logging/SummaryWriter.cpp
)

set(EMBI_UTILS_SOURCES
    src/utils/Random.cpp
    src/utils/ConfigLoader.cpp
    src/utils/FileUtils.cpp
    src/CLI.cpp
)

set(EMBI_ALL_SOURCES
    ${EMBI_CORE_SOURCES}
    ${EMBI_SCHEDULER_SOURCES}
    ${EMBI_WORKLOAD_SOURCES}
    ${EMBI_LOGGING_SOURCES}
    ${EMBI_UTILS_SOURCES}
)

# ─── Static library (shared between executable and tests) ────────────────────
add_library(embi_lib STATIC ${EMBI_ALL_SOURCES})

target_include_directories(embi_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(embi_lib
    PUBLIC nlohmann_json::nlohmann_json
)

# Propagate compile features
target_compile_features(embi_lib PUBLIC cxx_std_17)

# ─── Executable ──────────────────────────────────────────────────────────────
add_executable(embi_sim src/main.cpp)
target_link_libraries(embi_sim PRIVATE embi_lib)
set_target_properties(embi_sim PROPERTIES
    OUTPUT_NAME "embi_sim"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ─── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)

# ─── Doxygen documentation ───────────────────────────────────────────────────
find_package(Doxygen OPTIONAL_COMPONENTS dot mscgen dia)

if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME        "EMBI Scheduling Simulator")
    set(DOXYGEN_PROJECT_NUMBER      "${PROJECT_VERSION}")
    set(DOXYGEN_PROJECT_BRIEF       "${PROJECT_DESCRIPTION}")
    set(DOXYGEN_OUTPUT_DIRECTORY    "${CMAKE_CURRENT_SOURCE_DIR}/docs")
    set(DOXYGEN_RECURSIVE           YES)
    set(DOXYGEN_EXTRACT_ALL         YES)
    set(DOXYGEN_EXTRACT_PRIVATE     YES)
    set(DOXYGEN_GENERATE_HTML       YES)
    set(DOXYGEN_GENERATE_LATEX      NO)
    set(DOXYGEN_HAVE_DOT            YES)
    set(DOXYGEN_CALL_GRAPH          YES)
    set(DOXYGEN_CALLER_GRAPH        YES)
    set(DOXYGEN_WARN_AS_ERROR       YES)

    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        COMMENT "Generating Doxygen documentation"
    )
else()
    message(STATUS "Doxygen not found; 'docs' target unavailable")
endif()

# ─── Install ──────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS embi_sim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY scripts/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/embi_sim/scripts
    FILES_MATCHING PATTERN "*.py"
)

install(DIRECTORY examples/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/embi_sim/examples
)

# ─── Summary ─────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════╗")
message(STATUS "║   EMBI Scheduling Simulator v${PROJECT_VERSION}              ║")
message(STATUS "╠══════════════════════════════════════════════════╣")
message(STATUS "║  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "║  Install dir : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "║  Doxygen     : ${DOXYGEN_FOUND}")
message(STATUS "╚══════════════════════════════════════════════════╝")
message(STATUS "")
