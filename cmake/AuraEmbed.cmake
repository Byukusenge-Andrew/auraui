# ============================================================================
# AuraUI Asset Embedding Helper
# ============================================================================

function(aura_embed_assets target_name asset_directory)
    message(STATUS "AuraUI: Embedding UI assets from ${asset_directory} into ${target_name}")

    if(NOT EXISTS "${asset_directory}")
        message(FATAL_ERROR "Asset directory does not exist: ${asset_directory}")
    endif()

    # Find all UI asset files
    file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
        "${asset_directory}/*"
    )

    set(MAX_FILE_SIZE_BYTES 10485760) # 10 MiB limit per asset file
    set(DISALLOWED_EXTENSIONS "exe" "dll" "so" "dylib" "sh" "bat" "cmd" "ps1" "vbs")

    foreach(asset_file ${ASSET_FILES})
        if(IS_DIRECTORY "${asset_file}")
            continue()
        endif()

        # Check disallowed extensions
        get_filename_component(asset_ext "${asset_file}" LAST_EXT)
        string(TOLOWER "${asset_ext}" asset_ext_lower)
        string(REGEX REPLACE "^\\." "" asset_ext_clean "${asset_ext_lower}")

        if(asset_ext_clean IN_LIST DISALLOWED_EXTENSIONS)
            message(FATAL_ERROR "Security violation: Disallowed asset file extension '${asset_ext_clean}' found in ${asset_file}")
        endif()

        # Check file size limit
        file(SIZE "${asset_file}" file_size)
        if(file_size GREATER MAX_FILE_SIZE_BYTES)
            message(FATAL_ERROR "Security violation: Asset file ${asset_file} exceeds 10 MiB limit (${file_size} bytes)")
        endif()
    endforeach()
endfunction()
