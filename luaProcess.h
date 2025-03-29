#ifndef LUA_PROCESS_H
#define LUA_PROCESS_H

#include "files.h"

#include "common.h"

#define BZLA_MAGIC IDENTIFIER_TO_U32('B', 'Z', 'L', 'A')
#define _LUA_MAGIC IDENTIFIER_TO_U32(0x1B, 'L', 'u', 'a')

#define LUA_VERSION_EXPECT (0x52)

typedef struct __attribute((packed)) {
    u32 magic; // Compare to _LUA_MAGIC

    u8 version; // Compare to LUA_VERSION_EXPECT

    u8 format; // Usually 0

    u8 littleEndian;

    u8 sizeofInt;
    u8 sizeofSizeT;
    u8 sizeofInstruction;
    u8 sizeofNumber;

    u8 integral;
} LuaBytecodeHeader;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to BZLA_MAGIC
    u8 sourceHash[16]; // MD5 hash of source file

    u8 bytecodeData[0];
    LuaBytecodeHeader bytecodeHeader[0];
} LuaFileHeader;

void LuaPreprocess(u8* luaData) {
    LuaFileHeader* fileHeader = (LuaFileHeader*)luaData;
    if (fileHeader->magic != BZLA_MAGIC)
        panic("LUA file header magic is nonmatching");

    LuaBytecodeHeader* bytecodeHeader = fileHeader->bytecodeHeader;   
    if (bytecodeHeader->magic != _LUA_MAGIC)
        panic("LUA bytecode header magic is nonmatching");

    if (bytecodeHeader->version != LUA_VERSION_EXPECT) {
        panic(
            "Expected LUA version to be 5.2, got v%u.%u instead",
            (unsigned)(bytecodeHeader->version >> 4),
            (unsigned)(bytecodeHeader->version & 0xF)
        );
    }
}

FileHandle LuaProduce(u8* luacData, u64 luacDataSize) {
    LuaBytecodeHeader* bytecodeHeader = (LuaBytecodeHeader*)luacData;

    if (bytecodeHeader->magic != _LUA_MAGIC)
        warn("LuaProduce: bytecode magic is nonmatching");
    if (bytecodeHeader->version != LUA_VERSION_EXPECT) {
        warn(
            "LuaProduce: expected bytecode version to be 5.2, got v%u.%u instead",
            (unsigned)(bytecodeHeader->version >> 4),
            (unsigned)(bytecodeHeader->version & 0xF)
        );
    }

    FileHandle hndl;
    hndl.size = sizeof(LuaFileHeader) + luacDataSize;
    hndl.data_void = malloc(hndl.size);

    LuaFileHeader* fileHeader = (LuaFileHeader*)hndl.data_void;
    fileHeader->magic = BZLA_MAGIC;
    
    memcpy(fileHeader->sourceHash, "NoHash\0\0\0\0\0\0\0\0\0", 16);
    memcpy(fileHeader->bytecodeData, luacData, luacDataSize);

    return hndl;
}

FileHandle LuaCreateLuacFile(u8* luaData, u32 luaDataSize) {
    FileHandle hndl;
    hndl.size = luaDataSize - sizeof(LuaFileHeader);
    hndl.data_void = malloc(hndl.size);

    memcpy(hndl.data_void, ((LuaFileHeader*)luaData)->bytecodeData, hndl.size);

    return hndl;
}

#endif // LUA_PROCESS_H
