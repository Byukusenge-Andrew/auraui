# ============================================================================
# AuraUI CPack Packaging Configuration
# ============================================================================

set(CPACK_PACKAGE_NAME "AuraUI")
set(CPACK_PACKAGE_VENDOR "AuraUI Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AuraUI Desktop Application")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Byukusenge Andrew <maintainers@auraui.dev>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libwebkit2gtk-4.1-0")
endif()

include(CPack)
