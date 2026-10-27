cmake_minimum_required(VERSION 3.24)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AURA_VERSION_RAW)
string(STRIP "${AURA_VERSION_RAW}" AURA_VERSION)

project(AuraUI
    VERSION ${AURA_VERSION}
    DESCRIPTION "Cross-Platform C++20 Desktop Application Framework with Web UI"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(AURA_BUILD_TESTS "Build AuraUI unit tests and test suite" OFF)
option(AURA_BUILD_EXAMPLES "Build AuraUI showcase and example applications" OFF)
set(AURA_SANITIZE "" CACHE STRING "Sanitizer mode: 'address' (ASan+UBSan) or 'thread' (TSan)")
set(AURA_BACKEND "webview" CACHE STRING "Window backend: 'webview' (cross-platform) or 'native'")

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AuraWarnings)
include(AuraHardening)
include(AuraSanitizers)
include(AuraEmbed)
include(AuraPackage)
include(Deps)

# Core AuraUI Library Target
add_library(auraui)
add_library(auraui::auraui ALIAS auraui)

# Generate version.hpp from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/auraui/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/auraui/version.hpp"
    @ONLY
)

target_include_directories(auraui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/auraui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/auraui>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Third-Party Dependencies
target_link_libraries(auraui
    PUBLIC
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
        $<BUILD_INTERFACE:tl::expected>
    PRIVATE
        $<BUILD_INTERFACE:httplib>
        $<BUILD_INTERFACE:webview::webview>
)

# Platform-specific link libraries
if(WIN32)
    target_link_libraries(auraui PRIVATE
        version
        shlwapi
        shell32
        ole32
        uuid
        comctl32
    )
elseif(APPLE)
    target_link_libraries(auraui PRIVATE
        "-framework WebKit"
        "-framework Cocoa"
    )
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
    target_link_libraries(auraui PRIVATE PkgConfig::WEBKIT)
endif()

# Apply compiler warnings, binary hardening, and sanitizers
aura_apply_warnings(auraui)
aura_apply_hardening(auraui)
aura_apply_sanitizers(auraui)

# Add Source Subdirectories
add_subdirectory(src)

if(AURA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(AURA_BUILD_EXAMPLES)
    add_subdirectory(showcase)
    add_subdirectory(sample-app)
endif()

# Installation Rules & Export Package Config
include(GNUInstallDirs)

install(TARGETS auraui
    EXPORT aurauiTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT aurauiTargets
    FILE aurauiTargets.cmake
    NAMESPACE auraui::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/auraui
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/aurauiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/aurauiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/aurauiConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/aurauiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/aurauiConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/auraui
)
