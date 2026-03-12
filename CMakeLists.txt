cmake_minimum_required(VERSION 3.16)

project(GenPreCVSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Concurrent Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Concurrent Svg)

set(TS_FILES src/resources/GenPreCVSystem_en_AS.ts)

# Core
set(CORE_SOURCES
    src/core/main.cpp
    src/core/splashscreen.h
    src/core/splashscreen.cpp
)

# Config
set(CONFIG_SOURCES
    src/config/appsettings.h
    src/config/appsettings.cpp
    src/config/recentfilesmanager.h
    src/config/recentfilesmanager.cpp
    src/config/tasktypes.h
)

 # Models
set(MODEL_SOURCES
    src/models/tabdata.h
    src/models/undostack.h
    src/models/pythonenvironment.h
)

# Services
set(SERVICES_SOURCES
    # Environment services
    src/services/environment/environmentcachemanager.h
    src/services/environment/environmentcachemanager.cpp
    src/services/environment/environmentscanner.h
    src/services/environment/environmentscanner.cpp
    # Image services
    src/services/image/imageprocessor.h
    src/services/image/imageprocessor.cpp
    src/services/image/imageprocessservice.h
    src/services/image/imageprocessservice.cpp
    # Inference services
    src/services/inference/dlservice.h
    src/services/inference/dlservice.cpp
    # IO services
    src/services/io/fileutils.h
    src/services/io/fileutils.cpp
    src/services/io/exportservice.h
    src/services/io/exportservice.cpp
    # System services
    src/services/system/clipboardhelper.h
    src/services/system/clipboardhelper.cpp
    src/services/system/exceptions.h
    src/services/system/exceptions.cpp
    src/services/system/errordialog.h
    src/services/system/errordialog.cpp
)

# Views
set(VIEWS_SOURCES
    src/views/mainwindow.h
    src/views/mainwindow.cpp
    src/views/mainwindow.ui
    src/views/imageview.h
    src/views/imageview.cpp
    src/views/imagefilefilterproxy.h
    src/views/imagefilefilterproxy.cpp
    # Dialogs
    src/views/dialogs/batchprocessdialog.h
    src/views/dialogs/batchprocessdialog.cpp
    src/views/dialogs/detectionresultdialog.h
    src/views/dialogs/detectionresultdialog.cpp
    src/views/dialogs/settingsdialog.h
    src/views/dialogs/settingsdialog.cpp
    # Components
    src/views/components/filetreeview.h
    src/views/components/filetreeview.cpp
    src/views/components/environmentservicewidget.h
    src/views/components/environmentservicewidget.cpp
)

# Controllers
set(CONTROLLERS_SOURCES
    src/controllers/tabcontroller.h
    src/controllers/tabcontroller.cpp
    src/controllers/imagecontroller.h
    src/controllers/imagecontroller.cpp
    src/controllers/filecontroller.h
    src/controllers/filecontroller.cpp
    src/controllers/taskcontroller.h
    src/controllers/taskcontroller.cpp
    src/controllers/parameterpanelfactory.h
    src/controllers/parameterpanelfactory.cpp
)

# 资源文件
set(RESOURCE_FILES
    src/resources/resources.qrc
)

list(APPEND PROJECT_SOURCES
    ${CORE_SOURCES}
    ${CONFIG_SOURCES}
    ${MODEL_SOURCES}
    ${SERVICES_SOURCES}
    ${VIEWS_SOURCES}
    ${CONTROLLERS_SOURCES}
    ${TS_FILES}
    ${RESOURCE_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GenPreCVSystem
        MANUAL_FINALIZATION
        WIN32
        ${PROJECT_SOURCES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(GenPreCVSystem SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(GenPreCVSystem
            WIN32
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(GenPreCVSystem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg)

# 添加源目录和所有子目录到包含路径，使子目录的头文件可以相互引用
# 使用PUBLIC确保所有源文件都能访问
target_include_directories(GenPreCVSystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/environment
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/image
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/inference
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/system
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
)

# 添加源目录到AUTOUIC和AUTOMOC的搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/dialogs
)
set(CMAKE_AUTOMOC_MACRO_NAMES ${CMAKE_AUTOMOC_MACRO_NAMES} ${CMAKE_AUTOMOC_MACRO_NAMES})

# 复制模型文件到构建目录
add_custom_command(TARGET GenPreCVSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/models"
    "$<TARGET_FILE_DIR:GenPreCVSystem>/models"
    COMMENT "Copying models to output directory"
)

# 复制 Python 脚本到构建目录
add_custom_command(TARGET GenPreCVSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/inference/python"
    "$<TARGET_FILE_DIR:GenPreCVSystem>/python"
    COMMENT "Copying Python scripts to output directory"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GenPreCVSystem)
endif()

# ============================================
# 测试配置
# ============================================
option(BUILD_TESTS "Build tests" OFF)

message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")

if(BUILD_TESTS)
    message(STATUS "Building tests...")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    enable_testing()

    # 测试源文件
    set(TEST_SOURCES
        tests/test_main.cpp
        tests/unit/test_environmentcachemanager.cpp
        tests/unit/test_yoloservice.cpp
        tests/unit/test_environmentscanner.cpp
        tests/integration/test_environmentworkflow.cpp
    )

    # 被测试的源文件 - 需要链接到测试
    set(TEST_OBJ_SOURCES
        ${CONFIG_SOURCES}
        ${MODEL_SOURCES}
        ${SERVICES_SOURCES}
    )

    add_executable(GenPreCVSystemTests ${TEST_SOURCES} ${TEST_OBJ_SOURCES})

    target_link_libraries(GenPreCVSystemTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
    )

    target_include_directories(GenPreCVSystemTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/config
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services/environment
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services/image
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services/inference
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services/io
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services/system
    )

    add_test(NAME GenPreCVSystemTests COMMAND GenPreCVSystemTests)
endif()
