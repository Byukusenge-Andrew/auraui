# ============================================================================
# AuraUI Binary Hardening and Exploit Mitigation Flags
# ============================================================================

function(aura_apply_hardening target_name)
    if(MSVC)
        # MSVC Security Hardening
        target_compile_options(${target_name} PRIVATE
            /GS         # Buffer Security Check
            /sdl        # Additional Security Development Lifecycle checks
            /guard:cf   # Control Flow Guard
        )

        # Static runtime linkage (/MT in Release, /MTd in Debug)
        set_property(TARGET ${target_name} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )

        # MSVC Linker Hardening
        target_link_options(${target_name} PRIVATE
            /DYNAMICBASE      # ASLR
            /HIGHENTROPYVA    # 64-bit ASLR
            /NXCOMPAT         # Data Execution Prevention (DEP)
            /guard:cf         # Control Flow Guard
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        # GCC & Clang Hardening
        target_compile_options(${target_name} PRIVATE
            -fstack-protector-strong
            -fstack-clash-protection
            -D_FORTIFY_SOURCE=2
        )

        # Control Flow Protection for x86/x86_64
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
            target_compile_options(${target_name} PRIVATE -fcf-protection=full)
        endif()

        # Position Independent Executables
        set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

        # Linux Specific ELF Linker Hardening
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_options(${target_name} PRIVATE
                -pie
                -Wl,-z,relro
                -Wl,-z,now
                -Wl,-z,noexecstack
            )
        endif()
    endif()
endfunction()
