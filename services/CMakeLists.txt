# Distributed Camera Services - Independent C++ Project

cmake_minimum_required(VERSION 3.16)
project(DistributedCameraServices)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Source files
set(SERVICES_SOURCES
    src/distributed_camera_source_impl.cpp
    src/distributed_camera_sink_impl.cpp
    src/distributed_camera_service_factory.cpp
)

set(SERVICES_HEADERS
    include/distributed_camera_service.h
    include/distributed_camera_source_impl.h
    include/distributed_camera_sink_impl.h
)

# Create services library
add_library(distributed_camera_services STATIC
    ${SERVICES_SOURCES}
    ${SERVICES_HEADERS}
)

# Include directories
target_include_directories(distributed_camera_services PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mock/include
)

# Link libraries
target_link_libraries(distributed_camera_services
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../mock/build/Debug/distributed_camera_mock.lib
    Threads::Threads
)

# For Windows, add ws2_32 for socket support
if(WIN32)
    target_link_libraries(distributed_camera_services ws2_32)
endif()

# Create test executable
add_executable(services_test test/services_test.cpp)
target_link_libraries(services_test distributed_camera_services)

# Install rules
install(TARGETS distributed_camera_services
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)