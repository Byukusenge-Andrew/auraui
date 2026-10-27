add_executable(auraui_tests
    main.cpp
    unit/test_utf8.cpp
    unit/test_envelope.cpp
    unit/test_url_policy.cpp
    unit/test_path_policy.cpp
    unit/test_validator.cpp
    unit/test_dispatcher.cpp
    unit/test_router_aliases.cpp
    unit/test_middleware_async.cpp
    unit/test_csp.cpp
    unit/test_store_atomic.cpp
    unit/test_mime.cpp
    unit/test_asset_server.cpp
    unit/test_platform.cpp
)

target_include_directories(auraui_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(auraui_tests PRIVATE
    auraui::auraui
    doctest::doctest
)

aura_apply_warnings(auraui_tests)
aura_apply_hardening(auraui_tests)
aura_apply_sanitizers(auraui_tests)

add_test(NAME unit_tests COMMAND auraui_tests)
