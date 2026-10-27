# ============================================================================
# AuraUI Dependencies (Pinned commit SHAs for supply chain security)
# Supports offline builds (FETCHCONTENT_FULLY_DISCONNECTED) via vendored headers
# ============================================================================

# 1. nlohmann/json
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp")
    if(NOT TARGET nlohmann_json::nlohmann_json)
        add_library(nlohmann_json_lib INTERFACE)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_lib)
        target_include_directories(nlohmann_json_lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()
endif()

# 2. tl::expected
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/tl/expected.hpp")
    if(NOT TARGET tl::expected)
        add_library(tl_expected_lib INTERFACE)
        add_library(tl::expected ALIAS tl_expected_lib)
        target_include_directories(tl_expected_lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()
endif()

# 3. cpp-httplib (Phase 1 Loopback Asset Server)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/httplib/httplib.h")
    if(NOT TARGET httplib)
        add_library(httplib INTERFACE)
        target_include_directories(httplib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/httplib")
    endif()
endif()

# 4. webview/webview
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/webview/webview.h")
    if(NOT TARGET webview::webview)
        add_library(webview_lib INTERFACE)
        add_library(webview::webview ALIAS webview_lib)
        target_include_directories(webview_lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/webview")
    endif()
endif()

# 5. doctest (Unit Testing)
if(AURA_BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/doctest/doctest.h")
        if(NOT TARGET doctest::doctest)
            add_library(doctest_lib INTERFACE)
            add_library(doctest::doctest ALIAS doctest_lib)
            target_include_directories(doctest_lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        endif()
    endif()
endif()

# Fallback to FetchContent if vendored headers are missing
if(NOT TARGET nlohmann_json::nlohmann_json OR NOT TARGET tl::expected)
    include(FetchContent)
    if(NOT TARGET nlohmann_json::nlohmann_json)
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        9cca280a4d0ccf0f084f16723d81604932bee213
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()

    if(NOT TARGET tl::expected)
        FetchContent_Declare(
            tl_expected
            GIT_REPOSITORY https://github.com/TartanLlama/expected.git
            GIT_TAG        221516e3b50fa740cf726ff2c7f56477b94db22f
        )
        FetchContent_MakeAvailable(tl_expected)
        if(NOT TARGET tl::expected AND TARGET expected)
            add_library(tl::expected ALIAS expected)
        endif()
    endif()
endif()
