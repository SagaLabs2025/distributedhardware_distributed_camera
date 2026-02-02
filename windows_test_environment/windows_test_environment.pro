QT += core widgets

TARGET = DistributedCameraTest
TEMPLATE = app

CONFIG += c++17

# 定义目录
SINK_MODULE_DIR = $$PWD/../sink_module/include
SOURCE_MODULE_DIR = $$PWD/../source_module/include
COMMON_DIR = $$PWD/../common/include
VCPKG_ROOT = C:/Work/vcpkg/vcpkg/installed/x64-windows

SOURCES += \
    src/main.cpp \
    src/main_window.cpp \
    src/main_controller.cpp \
    src/automation_test_engine.cpp \
    src/log_redirector.cpp \
    ../common/src/dh_log_callback.cpp

HEADERS += \
    src/main_window.h \
    src/main_controller.h \
    src/automation_test_engine.h \
    src/log_redirector.h

INCLUDEPATH += \
    $$PWD/src \
    $$SINK_MODULE_DIR \
    $$SOURCE_MODULE_DIR \
    $$COMMON_DIR \
    $$VCPKG_ROOT/include

LIBS += \
    -L$$VCPKG_ROOT/lib \
    -lavcodec \
    -lavutil \
    -lswresample \
    -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
