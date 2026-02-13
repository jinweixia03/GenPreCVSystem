cmake_minimum_required(VERSION 3.16)

project(GenPreCVSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# LibTorch 配置
# ============================================
set(TORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libtorch")

# 检查 LibTorch 是否存在
if(EXISTS "${TORCH_DIR}")
    message(STATUS "Found LibTorch at: ${TORCH_DIR}")

    # 手动配置 LibTorch（绕过 CUDA 编译器检测）
    # VS2022 与 CUDA 11.8 的 STL 不兼容，但我们不需要编译 CUDA 代码
    set(Torch_FOUND TRUE)
    set(TORCH_LIBS torch c10 torch_cpu)

    # 检查 CUDA 库是否存在
    if(EXISTS "${TORCH_DIR}/lib/torch_cuda.lib")
        list(APPEND TORCH_LIBS torch_cuda)
        message(STATUS "CUDA version detected - GPU support enabled")
    else()
        message(STATUS "CPU-only version - will use CUDA if available at runtime")
    endif()

    # 设置 LibTorch 的 include 目录
    set(TORCH_INCLUDE_DIRS
        "${TORCH_DIR}/include"
        "${TORCH_DIR}/include/torch/csrc/api/include"
    )

    message(STATUS "LibTorch include dirs: ${TORCH_INCLUDE_DIRS}")
    message(STATUS "LibTorch libraries: ${TORCH_LIBS}")

    # 如果使用 MSVC，需要设置编译选项
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    endif()

    # 添加 LibTorch 库目录
    link_directories("${TORCH_DIR}/lib")

    set(LIBTORCH_FOUND TRUE)
    message(STATUS "LibTorch linked successfully (will use CPU at runtime if CUDA not available)")
else()
    message(WARNING "LibTorch not found at: ${TORCH_DIR}")
    message(WARNING "Please run: third_party/download_libtorch.ps1")
    set(LIBTORCH_FOUND FALSE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES src/resources/GenPreCVSystem_en_AS.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/ui/mainwindow.cpp
        src/ui/mainwindow.h
        src/ui/mainwindow.ui
        src/ui/splashscreen.h
        src/ui/splashscreen.cpp
        ${TS_FILES}
)

# Models
set(MODEL_SOURCES
    src/models/tabdata.h
    src/models/tasktypes.h
    src/models/undostack.h
    src/models/modelinference.h
    src/models/modelinference.cpp
)

# Utils
set(UTILS_SOURCES
    src/utils/imageprocessor.h
    src/utils/imageprocessor.cpp
    src/utils/clipboardhelper.h
    src/utils/clipboardhelper.cpp
    src/utils/fileutils.h
    src/utils/fileutils.cpp
    src/utils/yoloservice.h
    src/utils/yoloservice.cpp
    src/utils/imageprocessservice.h
    src/utils/imageprocessservice.cpp
    src/utils/exceptions.h
    src/utils/exceptions.cpp
    src/utils/errordialog.h
    src/utils/errordialog.cpp
    src/utils/appsettings.h
    src/utils/appsettings.cpp
    src/utils/recentfilesmanager.h
    src/utils/recentfilesmanager.cpp
    src/utils/exportservice.h
    src/utils/exportservice.cpp
)

# Views
set(VIEWS_SOURCES
    src/views/imageview.h
    src/views/imageview.cpp
    src/views/filetreeview.h
    src/views/filetreeview.cpp
    src/views/imagefilefilterproxy.h
    src/views/imagefilefilterproxy.cpp
    src/views/detectionresultdialog.h
    src/views/detectionresultdialog.cpp
    src/views/settingsdialog.h
    src/views/settingsdialog.cpp
    src/views/batchprocessdialog.h
    src/views/batchprocessdialog.cpp
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

# Widgets
set(WIDGETS_SOURCES
    src/widgets/environmentservicewidget.h
    src/widgets/environmentservicewidget.cpp
)

list(APPEND PROJECT_SOURCES
    ${MODEL_SOURCES}
    ${UTILS_SOURCES}
    ${VIEWS_SOURCES}
    ${CONTROLLERS_SOURCES}
    ${WIDGETS_SOURCES}
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

target_link_libraries(GenPreCVSystem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# 链接 LibTorch
if(LIBTORCH_FOUND)
    target_link_libraries(GenPreCVSystem PRIVATE "${TORCH_LIBS}")
    target_include_directories(GenPreCVSystem PRIVATE ${TORCH_INCLUDE_DIRS})

    # 设置 LibTorch DLL 的输出目录，确保运行时可以找到
    add_custom_command(TARGET GenPreCVSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TORCH_DIR}/bin"
        $<TARGET_FILE_DIR:GenPreCVSystem>
        COMMENT "Copying LibTorch DLLs to output directory"
    )

    message(STATUS "LibTorch linked successfully")
endif()

# 添加源目录和所有子目录到包含路径，使子目录的头文件可以相互引用
# 使用PUBLIC确保所有源文件都能访问
target_include_directories(GenPreCVSystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
)

# 添加源目录到AUTOUIC和AUTOMOC的搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
set(CMAKE_AUTOMOC_MACRO_NAMES ${CMAKE_AUTOMOC_MACRO_NAMES} ${CMAKE_AUTOMOC_MACRO_NAMES})

# 复制模型文件到构建目录
add_custom_command(TARGET GenPreCVSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/models"
    "$<TARGET_FILE_DIR:GenPreCVSystem>/models"
    COMMENT "Copying models to output directory"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GenPreCVSystem)
endif()
