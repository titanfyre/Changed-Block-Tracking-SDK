cmake_minimum_required(VERSION 3.24)
project(ChangedBlockTrackingSDK VERSION 0.1.1 LANGUAGES C CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This SDK requires Windows x64. No driver is installed by this build.")
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
add_library(TitanChangedBlockTrackingSDK STATIC
  "sdk/SDK/ChangedBlocks.cpp"
  "sdk/SDK/Inventory.cpp"
  "sdk/SDK/License.cpp"
)
add_library(ChangedBlockTrackingSDK::sdk ALIAS TitanChangedBlockTrackingSDK)
set_target_properties(TitanChangedBlockTrackingSDK PROPERTIES EXPORT_NAME sdk)
target_compile_features(TitanChangedBlockTrackingSDK PUBLIC cxx_std_17)
target_compile_definitions(TitanChangedBlockTrackingSDK PUBLIC )
target_compile_definitions(TitanChangedBlockTrackingSDK PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
if(MSVC)
  target_compile_options(TitanChangedBlockTrackingSDK PRIVATE /utf-8 /EHsc /W4)
endif()
target_include_directories(TitanChangedBlockTrackingSDK PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk>"
  "$<INSTALL_INTERFACE:include/>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/SDK>"
  "$<INSTALL_INTERFACE:include/SDK>"
)
target_link_libraries(TitanChangedBlockTrackingSDK PUBLIC advapi32 cfgmgr32 comctl32 comdlg32 crypt32 gdi32 newdev ole32 setupapi shell32 user32 winhttp wintrust)
install(TARGETS TitanChangedBlockTrackingSDK EXPORT ChangedBlockTrackingSDKTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY sdk/ DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl")
install(FILES LICENSE DESTINATION share/changed-block-tracking-sdk)
install(EXPORT ChangedBlockTrackingSDKTargets NAMESPACE ChangedBlockTrackingSDK:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ChangedBlockTrackingSDK)
configure_file("ChangedBlockTrackingSDKConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/ChangedBlockTrackingSDKConfig.cmake" @ONLY)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/ChangedBlockTrackingSDKConfigVersion.cmake" VERSION 0.1.1 COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ChangedBlockTrackingSDKConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/ChangedBlockTrackingSDKConfigVersion.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ChangedBlockTrackingSDK)
