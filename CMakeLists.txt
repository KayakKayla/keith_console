cmake_minimum_required(VERSION 3.16)

project(keith_console VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

add_subdirectory(qml)
enable_testing()
add_subdirectory(tests)

include(GNUInstallDirs)
install(TARGETS appkeith_console
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
