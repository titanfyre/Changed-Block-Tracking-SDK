#pragma once
#ifdef _KERNEL_MODE
#include <ntddk.h>
typedef ULONG CB_U32;typedef ULONGLONG CB_U64;typedef UCHAR CB_U8;typedef USHORT CB_U16;
#else
#include <windows.h>
#include <winioctl.h>
#include <cstdint>
typedef uint32_t CB_U32;typedef uint64_t CB_U64;typedef uint8_t CB_U8;typedef uint16_t CB_U16;
#endif
#define CB_VERSION 1u
#define CB_MAX_DEVICES 16u
#define CB_NAME 160u
#define CB_BLOCK 65536u
#define CB_PAGE 16384u
#define CB_MAX_CAPACITY (1024ULL*1024*1024*1024)
#define CB_ENUM CTL_CODE(FILE_DEVICE_UNKNOWN,0x960,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_START CTL_CODE(FILE_DEVICE_UNKNOWN,0x961,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_STOP CTL_CODE(FILE_DEVICE_UNKNOWN,0x962,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN,0x963,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_CHECKPOINT CTL_CODE(FILE_DEVICE_UNKNOWN,0x964,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_READ_MAP CTL_CODE(FILE_DEVICE_UNKNOWN,0x965,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_ACK CTL_CODE(FILE_DEVICE_UNKNOWN,0x966,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define CB_FLAG_ACTIVE 1u
#define CB_FLAG_INVALID 2u
#define CB_FLAG_FROZEN 4u
#define CB_FLAG_ACCESS_EXPIRED 8u
typedef struct CB_REQUEST {CB_U32 size,version;CB_U64 device,generation,offset,capacity;} CB_REQUEST;
typedef struct CB_DEVICE {CB_U64 id;CB_U16 pdo_name[CB_NAME];} CB_DEVICE;
typedef struct CB_INVENTORY {CB_U32 size,version,count,reserved;CB_DEVICE devices[CB_MAX_DEVICES];} CB_INVENTORY;
typedef struct CB_STATE {
 CB_U32 size,version,flags,block_bytes;
 CB_U64 device,session,generation,capacity,bitmap_bytes,write_requests,inflight,changed_blocks;
} CB_STATE;
typedef struct CB_BITMAP_PAGE {CB_STATE state;CB_U64 offset;CB_U32 bytes,reserved;CB_U8 data[CB_PAGE];} CB_BITMAP_PAGE;
#ifdef __cplusplus
static_assert(sizeof(CB_REQUEST)==40);
static_assert(sizeof(CB_STATE)==80);
#else
C_ASSERT(sizeof(CB_REQUEST)==40);
C_ASSERT(sizeof(CB_STATE)==80);
#endif
