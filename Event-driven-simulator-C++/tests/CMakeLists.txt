################################################################################
# tests/CMakeLists.txt — GoogleTest suite for the EMBI Simulator
################################################################################

# Helper macro: create a test executable, link embi_lib + GTest, register
macro(embi_add_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            embi_lib
            GTest::gtest_main
            GTest::gmock_main
    )
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        PROPERTIES TIMEOUT 120
    )
endmacro()

include(GoogleTest)

embi_add_test(test_process         test_process.cpp)
embi_add_test(test_schedulers      test_schedulers.cpp)
embi_add_test(test_workloads       test_workloads.cpp)
embi_add_test(test_metrics_online  test_metrics_online.cpp)
embi_add_test(test_metrics_offline test_metrics_offline.cpp)
embi_add_test(test_statistics_db   test_statistics_db.cpp)
embi_add_test(test_cli             test_cli.cpp)
embi_add_test(test_config_loader   test_config_loader.cpp)
embi_add_test(test_random          test_random.cpp)
embi_add_test(test_event_queue     test_event_queue.cpp)
embi_add_test(test_experiment      test_experiment.cpp)
embi_add_test(test_regression      test_regression.cpp)
