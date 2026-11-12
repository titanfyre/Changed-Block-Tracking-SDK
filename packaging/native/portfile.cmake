vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/titanfyre/Changed-Block-Tracking-SDK/releases/download/sdk-v0.1.1/changed-block-tracking-sdk-sdk-0.1.1.tar.gz"
    FILENAME "changed-block-tracking-sdk-sdk-0.1.1.tar.gz"
    SHA512 70939ae4dd7418d22651d356e0cc2aa4385cda9883d9f295854851d6a4bdb7e7dc65a5b93316b8c88b31a709cd39fb0127f990ab0a54e4f2f12ee711d7b88b5c
)
vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}" NO_REMOVE_ONE_LEVEL)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ChangedBlockTrackingSDK CONFIG_PATH lib/cmake/ChangedBlockTrackingSDK)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
