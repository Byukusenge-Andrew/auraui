# ============================================================================
# AuraUI Sanitizers Configuration
# ============================================================================

function(aura_apply_sanitizers target_name)
    if(NOT AURA_SANITIZE)
        return()
    endif()

    if(AURA_SANITIZE STREQUAL "address")
        message(STATUS "AuraUI: Enabling AddressSanitizer + UndefinedBehaviorSanitizer on ${target_name}")
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
        endif()
    elseif(AURA_SANITIZE STREQUAL "thread")
        message(STATUS "AuraUI: Enabling ThreadSanitizer on ${target_name}")
        if(MSVC)
            message(FATAL_ERROR "ThreadSanitizer is not supported on MSVC.")
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
            target_link_options(${target_name} PRIVATE -fsanitize=thread)
        endif()
    else()
        message(FATAL_ERROR "Unknown sanitizer mode: ${AURA_SANITIZE}. Expected 'address' or 'thread'.")
    endif()
endfunction()
