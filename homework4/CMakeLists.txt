cmake_minimum_required(VERSION 3.16)
project(QT3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.7.3 REQUIRED COMPONENTS Core Sql Network Widgets)

qt_add_executable(QT3
    main.cpp
    database.cpp
    database.h
)

qt_add_resources(QT3 database_schema
    PREFIX "/"
    FILES sql/schema.sql
)

target_link_libraries(QT3 PRIVATE Qt6::Core Qt6::Sql Qt6::Network)
target_compile_options(QT3 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

qt_add_executable(QT3Ui WIN32 MACOSX_BUNDLE
    preview/main.cpp
    preview/navigationcontroller.cpp
    preview/navigationcontroller.h
    preview/previewwindow.cpp
    preview/previewwindow.h
)
set_target_properties(QT3Ui PROPERTIES AUTOMOC ON)

qt_add_resources(QT3Ui preview_theme
    PREFIX "/"
    FILES
        preview/theme.qss
        preview/down-arrow.png
)

target_link_libraries(QT3Ui PRIVATE Qt6::Widgets)
target_compile_options(QT3Ui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
