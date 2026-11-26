cmake_minimum_required(VERSION 3.16)
project(QT3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.7.3 REQUIRED COMPONENTS Core Sql Network Widgets Concurrent)

qt_add_library(QT3Database STATIC
    database.cpp
    database.h
    patientrecord.h
    passworddigest.cpp
    passworddigest.h
    staffrecord.h
)

qt_add_resources(QT3Database database_schema
    PREFIX "/"
    FILES sql/schema.sql
)

target_include_directories(QT3Database PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(QT3Database PUBLIC Qt6::Sql PRIVATE Qt6::Network)
target_compile_options(QT3Database PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

qt_add_executable(QT3 main.cpp)
target_link_libraries(QT3 PRIVATE QT3Database)
target_compile_options(QT3 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

qt_add_executable(QT3Ui WIN32 MACOSX_BUNDLE
    preview/main.cpp
    preview/navigationcontroller.cpp
    preview/navigationcontroller.h
    preview/patientsexdelegate.cpp
    preview/patientsexdelegate.h
    preview/patienttablemodel.cpp
    preview/patienttablemodel.h
    preview/previewwindow.cpp
    preview/previewwindow.h
    preview/selectmodel.cpp
    preview/selectmodel.h
)
set_target_properties(QT3Ui PROPERTIES AUTOMOC ON)

qt_add_resources(QT3Ui preview_theme
    PREFIX "/"
    FILES
        preview/theme.qss
        preview/down-arrow.png
)

target_link_libraries(QT3Ui PRIVATE Qt6::Widgets Qt6::Concurrent QT3Database)
target_compile_options(QT3Ui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
