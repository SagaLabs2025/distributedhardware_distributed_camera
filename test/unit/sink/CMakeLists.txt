# CMakeLists.txt for Sink端工作流测试套件
# Copyright (c) 2026 Huawei Device Co., Ltd.

cmake_minimum_required(VERSION 3.10)
project(SinkWorkflowTests VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用测试
enable_testing()

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# ========== 查找依赖包 ==========

# 查找GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 设置Mock组件包含路径
set(MOCK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mock/include" CACHE PATH "Mock组件头文件目录")
set(SINK_MOCK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mock" CACHE PATH "Sink端Mock组件目录")

# 项目根目录（用于访问源代码）
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." CACHE PATH "项目根目录")

# ========== 包含目录 ==========

include_directories(
    ${MOCK_INCLUDE_DIR}
    ${SINK_MOCK_DIR}
    ${PROJECT_ROOT}/common/utils/include
    ${PROJECT_ROOT}/interfaces/inner_kits/native_cpp/include
    ${PROJECT_ROOT}/services/cameraservice/sinkservice/include
    ${PROJECT_ROOT}/services/cameraservice/base/include
    ${PROJECT_ROOT}/services/channel/include
)

# ========== 源文件 ==========

# 测试源文件
set(TEST_SOURCES
    test_sink_workflow.cpp
)

# Mock组件源文件
set(MOCK_SOURCES
    mock/mock_sink_channel.cpp
    mock/mock_camera_client_sink.cpp
)

# 共享Mock组件源文件（如果存在）
set(SHARED_MOCK_SOURCES
    ${MOCK_INCLUDE_DIR}/../log_capture.cpp
)

# 检查共享Mock源文件是否存在
if(EXISTS "${MOCK_INCLUDE_DIR}/../log_capture.cpp")
    list(APPEND MOCK_SOURCES "${MOCK_INCLUDE_DIR}/../log_capture.cpp")
endif()

# ========== 创建测试可执行文件 ==========

add_executable(test_sink_workflow
    ${TEST_SOURCES}
    ${MOCK_SOURCES}
)

# ========== 链接库 ==========

target_link_libraries(test_sink_workflow
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# 如果有其他依赖库，可以添加：
# target_link_libraries(test_sink_workflow
#     distributed_camera_sink
#     distributed_camera_utils
#     ...
# )

# ========== 编译定义 ==========

target_compile_definitions(test_sink_workflow
    PRIVATE
    DH_LOG_TAG="SinkWorkflowTest"
)

# ========== 安装配置 ==========

install(TARGETS test_sink_workflow
    RUNTIME DESTINATION bin/tests
)

# ========== 测试发现 ==========

# 添加测试到CTest
add_test(NAME SinkWorkflowTests COMMAND test_sink_workflow)

# 设置测试属性
set_tests_properties(SinkWorkflowTests PROPERTIES
    TIMEOUT 300  # 5分钟超时
    LABELS "sink;workflow;integration"
)

# ========== 自定义目标 ==========

# 运行测试并生成XML报告
add_custom_target(run_sink_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_sink_workflow
            --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/sink_test_results.xml
    DEPENDS test_sink_workflow
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "运行Sink端工作流测试并生成XML报告"
)

# 生成测试报告
add_custom_target(sink_test_report
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py
            ${CMAKE_CURRENT_BINARY_DIR}/sink_test_results.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/SINK_TEST_REPORT.md
    DEPENDS run_sink_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "生成Sink端测试报告"
)

# 打印配置信息
message(STATUS "")
message(STATUS "========== Sink端工作流测试配置 ==========")
message(STATUS "测试可执行文件: test_sink_workflow")
message(STATUS "Mock组件目录: ${SINK_MOCK_DIR}")
message(STATUS "项目根目录: ${PROJECT_ROOT}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "===========================================")
message(STATUS "")
