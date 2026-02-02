# Distributed Camera Mock Framework - Independent C++ Project

cmake_minimum_required(VERSION 3.16)
project(DistributedCameraMock)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Source files
set(MOCK_SOURCES
    src/mock_implementation.cpp
    src/mock_module_manager.cpp
    src/mock_video_source.cpp
    src/data_buffer.cpp
    src/surface_mock.cpp
    src/avcodec_mock.cpp
    src/hdi_mock.cpp
    src/softbus_mock.cpp
    src/camera_mock.cpp
    log_capture.cpp
)

set(MOCK_HEADERS
    include/mock_interface.h
    include/mock_implementation.h
    include/mock_module_manager.h
    include/mock_video_source.h
    include/data_buffer.h
    include/surface_mock.h
    include/avcodec_mock.h
    include/hdi_mock.h
    include/softbus_mock.h
    include/camera_mock.h
)

# Create mock library
add_library(distributed_camera_mock STATIC
    ${MOCK_SOURCES}
    ${MOCK_HEADERS}
)

# Include directories
target_include_directories(distributed_camera_mock PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/channel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/channel/test/unittest/common/channel
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/utils/include
)

# Link libraries
target_link_libraries(distributed_camera_mock
    Threads::Threads
)

# For Windows, add ws2_32 for socket support
if(WIN32)
    target_link_libraries(distributed_camera_mock ws2_32)
endif()

# Create test executable
add_executable(mock_test test/mock_test.cpp)
target_link_libraries(mock_test distributed_camera_mock)

# Create surface test executable
add_executable(surface_mock_test test/surface_mock_test.cpp)
target_link_libraries(surface_mock_test distributed_camera_mock)

# Create AVCodec test executable
add_executable(avcodec_test test/avcodec_test.cpp)
target_link_libraries(avcodec_test distributed_camera_mock)

# Create HDI test executable
add_executable(hdi_mock_test test/hdi_mock_test.cpp)
target_link_libraries(hdi_mock_test distributed_camera_mock)

# Create Camera Mock test executable
add_executable(camera_mock_test test/camera_mock_test.cpp)
target_link_libraries(camera_mock_test distributed_camera_mock)

# Create SoftBus Mock test executable
add_executable(softbus_mock_test test/softbus_mock_test.cpp)
target_link_libraries(softbus_mock_test distributed_camera_mock)

# Create SoftBus Mock integration test executable
add_executable(softbus_mock_integration test/softbus_mock_integration.cpp)
target_link_libraries(softbus_mock_integration distributed_camera_mock)

# Create LogCapture test executable
add_executable(log_capture_test log_capture_test.cpp)
target_link_libraries(log_capture_test distributed_camera_mock)

# Create Surface basic validation test executable
add_executable(test_surface_basic test_surface_basic.cpp)
target_link_libraries(test_surface_basic distributed_camera_mock)

# Create AVCodec basic validation test executable
add_executable(test_avcodec_basic test_avcodec_basic.cpp)
target_link_libraries(test_avcodec_basic distributed_camera_mock)

# Install rules (optional)
install(TARGETS distributed_camera_mock
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)