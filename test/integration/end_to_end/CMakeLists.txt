# Copyright (c) 2021-2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)
project(dcamera_complete_e2e_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

# 测试可执行文件
add_executable(dcamera_complete_test
    test_complete_dcamera.cpp
)

# 包含目录 - 指向项目根目录的相对路径
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

target_include_directories(dcamera_complete_test PRIVATE
    ${PROJECT_ROOT}/services/cameraservice/sourceservice/include
    ${PROJECT_ROOT}/services/cameraservice/sinkservice/include
    ${PROJECT_ROOT}/services/cameraservice/base/include
    ${PROJECT_ROOT}/interfaces/inner_kits/native_cpp/camera_source/include
    ${PROJECT_ROOT}/interfaces/inner_kits/native_cpp/camera_sink/include
    ${PROJECT_ROOT}/common/include/utils
    ${PROJECT_ROOT}/common/include/constants
)

# GTest相关 - 使用系统安装的或third_party目录的
find_package(GTest REQUIRED)

target_link_libraries(dcamera_complete_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
)

# 启用测试
enable_testing()
add_test(NAME dcamera_complete_test COMMAND dcamera_complete_test)

# 安装规则
install(TARGETS dcamera_complete_test DESTINATION bin)

# 自定义目标：运行测试并生成报告
add_custom_target(run_e2e_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dcamera_complete_test
    DEPENDS dcamera_complete_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running distributed camera complete workflow test..."
)

add_custom_target(e2e_test_report
    COMMAND ${CMAKE_COMMAND} -E echo "Generating test report..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/COMPLETE_TEST_REPORT.md
        ${CMAKE_CURRENT_BINARY_DIR}/TEST_REPORT.md
    DEPENDS run_e2e_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Test report generated"
)

# 打印配置信息
message(STATUS "")
message(STATUS "==================================================")
message(STATUS "  分布式相机完整流程测试配置")
message(STATUS "  DCamera Complete Workflow Test Configuration")
message(STATUS "==================================================")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Project root:      ${PROJECT_ROOT}")
message(STATUS "  Binary output:     ${CMAKE_CURRENT_BINARY_DIR}/dcamera_complete_test")
message(STATUS "  Test report:       ${CMAKE_CURRENT_BINARY_DIR}/TEST_REPORT.md")
message(STATUS "")
message(STATUS "  构建命令:")
message(STATUS "  Build commands:")
message(STATUS "    mkdir -p build && cd build")
message(STATUS "    cmake ..")
message(STATUS "    make")
message(STATUS "")
message(STATUS "  运行测试:")
message(STATUS "  Run tests:")
message(STATUS "    make run_e2e_test")
message(STATUS "    或直接运行: ./dcamera_complete_test")
message(STATUS "")
message(STATUS "  生成报告:")
message(STATUS "  Generate report:")
message(STATUS "    make e2e_test_report")
message(STATUS "==================================================")
message(STATUS "")
