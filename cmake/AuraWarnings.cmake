# ============================================================================
# AuraUI Compiler Warnings Configuration
# ============================================================================

function(aura_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /WX
            /permissive-
            /utf-8
            /EHsc
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wold-style-cast
            -Werror
        )
    endif()
endfunction()
