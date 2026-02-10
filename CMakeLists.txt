cmake_minimum_required(VERSION 3.16)

project(GenPreCVSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES src/GenPreCVSystem_en_AS.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        ${TS_FILES}
)

# Models
set(MODEL_SOURCES
    src/models/tabdata.h
    src/models/tasktypes.h
    src/models/undostack.h
)

# Utils
set(UTILS_SOURCES
    src/utils/imageprocessor.h
    src/utils/imageprocessor.cpp
    src/utils/clipboardhelper.h
    src/utils/clipboardhelper.cpp
    src/utils/fileutils.h
    src/utils/fileutils.cpp
)

# Views
set(VIEWS_SOURCES
    src/views/imageview.h
    src/views/imageview.cpp
    src/views/filetreeview.h
    src/views/filetreeview.cpp
    src/views/imagefilefilterproxy.h
    src/views/imagefilefilterproxy.cpp
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

list(APPEND PROJECT_SOURCES
    ${MODEL_SOURCES}
    ${UTILS_SOURCES}
    ${VIEWS_SOURCES}
    ${CONTROLLERS_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GenPreCVSystem
        MANUAL_FINALIZATION
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
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(GenPreCVSystem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GenPreCVSystem)
endif()
