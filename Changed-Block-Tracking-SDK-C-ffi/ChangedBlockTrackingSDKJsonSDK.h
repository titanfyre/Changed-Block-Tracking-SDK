/* SPDX-License-Identifier: MIT */
#ifndef CHANGED_BLOCK_TRACKING_SDK_CSDK_H
#define CHANGED_BLOCK_TRACKING_SDK_CSDK_H
#include <stdint.h>
#if !defined(_WIN32) || !defined(_WIN64)
#error This SDK requires Windows x64.
#endif
#ifdef __cplusplus
#define CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT noexcept
extern "C" {
#else
#define CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT
#endif
#if defined(CHANGED_BLOCK_TRACKING_SDK_CSDK_BUILD)
#define CHANGED_BLOCK_TRACKING_SDK_CSDK_API __declspec(dllexport)
#else
#define CHANGED_BLOCK_TRACKING_SDK_CSDK_API __declspec(dllimport)
#endif
/* open is inert. One active session per loaded product DLL.
   call accepts length-delimited UTF-8 JSON and returns an allocated UTF-8
   response, or NULL on allocation failure. Always free with this same DLL.
   Session/operation handles are scoped to the client that returned them. */
CHANGED_BLOCK_TRACKING_SDK_CSDK_API uint32_t changed_block_tracking_sdk_json_open(uint64_t *session) CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT;
CHANGED_BLOCK_TRACKING_SDK_CSDK_API uint32_t changed_block_tracking_sdk_json_close(uint64_t session) CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT;
CHANGED_BLOCK_TRACKING_SDK_CSDK_API char *changed_block_tracking_sdk_json_call(uint64_t session,const char *request,uint32_t length) CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT;
CHANGED_BLOCK_TRACKING_SDK_CSDK_API void changed_block_tracking_sdk_json_free(char *response) CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT;
#ifdef __cplusplus
}
#endif
#undef CHANGED_BLOCK_TRACKING_SDK_CSDK_NOEXCEPT
#endif
