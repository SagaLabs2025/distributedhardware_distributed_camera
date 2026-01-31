# Windows本地分布式相机测试环境
cmake_minimum_required(VERSION 3.16)
project(DistributedCameraTest VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg FFmpeg 路径
set(VCPKG_ROOT "C:/Work/vcpkg/vcpkg/installed/x64-windows")
set(FFMPEG_INCLUDE_DIR "${VCPKG_ROOT}/include")
set(FFMPEG_LIB_DIR "${VCPKG_ROOT}/lib")

# Qt配置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 源文件
set(PROJECT_SOURCES
    src/main.cpp
    src/main_window.cpp
    src/main_controller.cpp
    src/automation_test_engine.cpp
    src/log_redirector.cpp
)

# 头文件
set(PROJECT_HEADERS
    src/main_window.h
    src/main_controller.h
    src/automation_test_engine.h
    src/log_redirector.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sink_module/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../source_module/include
    ${FFMPEG_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
    ${FFMPEG_LIB_DIR}/swresample.lib
    ws2_32  # Windows Socket
)

# Windows特定配置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
