cmake_minimum_required(VERSION 3.16)

project(GenPreCVSystemTests VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Core Widgets)

# 测试源文件
set(TEST_SOURCES
    test_main.cpp
    unit/test_environmentcachemanager.cpp
    unit/test_yoloservice.cpp
    unit/test_environmentscanner.cpp
    integration/test_environmentworkflow.cpp
)

# 被测试的源文件（从主项目复制）
set(SOURCES_UNDER_TEST
    ../src/utils/yoloservice.cpp
    ../src/utils/environmentcachemanager.cpp
    ../src/utils/environmentscanner.cpp
)

set(HEADERS_UNDER_TEST
    ../src/utils/yoloservice.h
    ../src/utils/environmentcachemanager.h
    ../src/utils/environmentscanner.h
    ../src/dl/inference/pythonenvironment.h
)

add_executable(GenPreCVSystemTests
    ${TEST_SOURCES}
    ${SOURCES_UNDER_TEST}
    ${HEADERS_UNDER_TEST}
)

target_link_libraries(GenPreCVSystemTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(GenPreCVSystemTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/dl/inference
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models
)

# 添加测试
add_test(NAME GenPreCVSystemTests COMMAND GenPreCVSystemTests)

# 测试输出格式 - 设置环境变量避免 GUI 依赖
set_property(TEST GenPreCVSystemTests PROPERTY
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
