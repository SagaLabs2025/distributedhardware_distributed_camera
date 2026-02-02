# Source Module - Windows本地分布式相机测试环境
cmake_minimum_required(VERSION 3.16)
project(SourceModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg FFmpeg 路径
set(VCPKG_ROOT "C:/Work/vcpkg/vcpkg/installed/x64-windows")
set(FFMPEG_INCLUDE_DIR "${VCPKG_ROOT}/include")
set(FFMPEG_LIB_DIR "${VCPKG_ROOT}/lib")

# 源文件
set(SOURCE_SOURCES
    src/source_service_impl.cpp
    src/socket_receiver.cpp
    ../common/src/dh_log_callback.cpp
)

# 头文件
set(SOURCE_HEADERS
    include/distributed_camera_source.h
    src/source_service_impl.h
    src/socket_receiver.h
)

# 创建动态库
add_library(${PROJECT_NAME} SHARED
    ${SOURCE_SOURCES}
    ${SOURCE_HEADERS}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${FFMPEG_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ws2_32  # Windows Socket
)

# Windows特定配置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".dll"
        OUTPUT_NAME "Source"
    )
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
