# Distributed Camera Test Environment - Console Version

cmake_minimum_required(VERSION 3.16)
project(DistributedCameraTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Mock framework sources
set(MOCK_SOURCES
    mock/src/mock_implementation.cpp
    mock/src/mock_module_manager.cpp
    mock/src/mock_video_source.cpp
    mock/src/data_buffer.cpp
)

# Services sources
set(SERVICES_SOURCES
    services/src/distributed_camera_source_impl.cpp
    services/src/distributed_camera_sink_impl.cpp
    services/src/distributed_camera_service_factory.cpp
)

# Create basic test executable
add_executable(DistributedCameraTest
    main.cpp
    ${MOCK_SOURCES}
    ${SERVICES_SOURCES}
)

# Create dual-end test executable
add_executable(DistributedCameraDualTest
    dual_test.cpp
    ${MOCK_SOURCES}
    ${SERVICES_SOURCES}
)

# Create enhanced dual-end test executable
add_executable(DistributedCameraEnhancedDualTest
    enhanced_dual_test.cpp
    ${MOCK_SOURCES}
    ${SERVICES_SOURCES}
)

# Test automation framework sources
set(TEST_AUTOMATION_SOURCES
    test_automation/main.cpp
    test_automation/framework/test_case_manager.cpp
    test_automation/framework/test_framework.cpp
    test_automation/protocol_validator/protocol_validator.cpp
    test_automation/stability_test/stability_tester.cpp
    test_automation/report_generator/html_report_generator.cpp
    test_automation/report_generator/xml_report_generator.cpp
)

# Create test automation executable
add_executable(DistributedCameraTestAutomation
    ${TEST_AUTOMATION_SOURCES}
)

# Include directories
target_include_directories(DistributedCameraTest PRIVATE
    mock/include
    services/include
)

target_include_directories(DistributedCameraDualTest PRIVATE
    mock/include
    services/include
)

target_include_directories(DistributedCameraEnhancedDualTest PRIVATE
    mock/include
    services/include
)

target_include_directories(DistributedCameraTestAutomation PRIVATE
    test_automation/framework
    test_automation/protocol_validator
    test_automation/stability_test
    test_automation/report_generator
    test_automation/test_cases
    mock/include
    services/include
)

# Link libraries
target_link_libraries(DistributedCameraTest
    Threads::Threads
)

target_link_libraries(DistributedCameraDualTest
    Threads::Threads
)

target_link_libraries(DistributedCameraEnhancedDualTest
    Threads::Threads
)

target_link_libraries(DistributedCameraTestAutomation
    Threads::Threads
)

# For Windows, add ws2_32 for socket support
if(WIN32)
    target_link_libraries(DistributedCameraTest ws2_32)
    target_link_libraries(DistributedCameraDualTest ws2_32)
    target_link_libraries(DistributedCameraEnhancedDualTest ws2_32)
    target_link_libraries(DistributedCameraTestAutomation ws2_32)
endif()