add_executable(showcase_app
    src/main.cpp
)

target_include_directories(showcase_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(showcase_app PRIVATE
    auraui::auraui
)

aura_apply_warnings(showcase_app)
aura_apply_hardening(showcase_app)
