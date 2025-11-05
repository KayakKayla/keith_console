find_package(spdlog CONFIG REQUIRED)

qt_add_library(terminal_core STATIC
    config_loader.cc
    terminal_bridge.cc
    terminal_session.cc
    logger.cc
)

target_include_directories(terminal_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(terminal_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        spdlog::spdlog
)

find_library(UTIL_LIB util)
if(UTIL_LIB)
    target_link_libraries(terminal_core PRIVATE ${UTIL_LIB})
endif()
