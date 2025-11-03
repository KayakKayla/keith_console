set(UI_SOURCES
    application.cc
)

qt_add_executable(appkeith_console
    ${UI_SOURCES}
)

target_include_directories(appkeith_console
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_qml_module(appkeith_console
    URI keith_console
    VERSION 1.0
    QML_FILES
        ../../qml/Main.qml
)

set_source_files_properties(
    ../../qml/Main.qml
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

set_target_properties(appkeith_console PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appkeith_console
    PRIVATE Qt6::Quick
)
