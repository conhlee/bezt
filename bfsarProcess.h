#ifndef BFSAR_PROCESS_H
#define BFSAR_PROCESS_H

#include "common.h"

#define FSAR_MAGIC IDENTIFIER_TO_U32('F', 'S', 'A', 'R')

#define BFSAR_BOMARK_BIG (0xFFFE)
#define BFSAR_BOMARK_LITTLE (0xFEFF)

#define BFSAR_MAJOR_VERSION (2)

#define BLOCK_TYPE_STRING (0x2000)
#define BLOCK_TYPE_INFO   (0x2001)
#define BLOCK_TYPE_FILE   (0x2002)

#define SECTION_TYPE_STRING (0x2400)
#define SECTION_TYPE_SEARCH (0x2401)

#define REF_TYPE_SOUND_INFO_RT        (0x2100)
#define REF_TYPE_SOUND_GRP_INFO_RT    (0x2104)
#define REF_TYPE_BANK_INFO_RT         (0x2101)
#define REF_TYPE_WAVE_ARCHIVE_INFO_RT (0x2103)
#define REF_TYPE_GROUP_INFO_RT        (0x2105)
#define REF_TYPE_PLAYER_INFO_RT       (0x2102)
#define REF_TYPE_FILE_INFO_RT         (0x2106)

#define REF_TYPE_SOUND_INFO                (0x2200)
#define REF_TYPE_SOUND_GRP_INFO            (0x2204)
#define REF_TYPE_BANK_INFO                 (0x2206)
#define REF_TYPE_WAVE_ARCHIVE_INFO         (0x2207)
#define REF_TYPE_GROUP_INFO                (0x2208)
#define REF_TYPE_PLAYER_INFO               (0x2209)
#define REF_TYPE_FILE_INFO                 (0x220A)
#define REF_TYPE_SOUND_ARCHIVE_PLAYER_INFO (0x220B)

#define STRG_MAGIC IDENTIFIER_TO_U32('S', 'T', 'R', 'G')

#define ITEMID_TYPE_SOUND        (1)
#define ITEMID_TYPE_SOUND_GROUP  (2)
#define ITEMID_TYPE_BANK         (3)
#define ITEMID_TYPE_PLAYER       (4)
#define ITEMID_TYPE_WAVE_ARCHIVE (5)
#define ITEMID_TYPE_GROUP        (6)

#define INFO_MAGIC IDENTIFIER_TO_U32('I', 'N', 'F', 'O')

#define FILE_MAGIC IDENTIFIER_TO_U32('F', 'I', 'L', 'E')

typedef struct __attribute((packed)) {
    u16 blockType; // BLOCK_TYPE_x
    u16 _pad16;
    u32 blockOffset; // Relative to start of file
    u32 blockSize;
} BfsarLookupEntry;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to FSAR_MAGIC
    u16 boMark; // Byte Order Mark: compare to BFSAR_BOMARK_BIG and BFSAR_BOMARK_LITTLE

    u16 headerSize; // Including block lookup table

    u8 patchVersion;
    u8 minorVersion;
    u8 majorVersion; // Compare to BFSAR_MAJOR_VERSION
    u8 _pad8;

    u32 fileSize;

    u16 blockCount;
    u16 _pad16;

    BfsarLookupEntry lookupEntries[0];
} BfsarFileHeader;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u16 sectionType; // SECTION_TYPE_x or 0
    u16 _pad16;

    // Relative to parent offsetOrigin (in most cases).
    // If offset to file data, relative to fileBlock->data
    u32 sectionOffset;
} BfsarSectionRef;

typedef struct __attribute((packed)) {
    BfsarSectionRef stringReference;
    u32 stringSize; // Including null terminator
} BfsarStringTableEntry;

typedef struct __attribute((packed)) {
    u32 entryCount;
    BfsarStringTableEntry entries[0];
} BfsarStringTable;

typedef struct __attribute((packed)) {
    u32 fileIndex : 24;
    u32 fileType : 8;
} BfsarItemId;

typedef struct __attribute((packed)) {
    u16 isLeaf;
    u16 stringBitIndex;
    u32 leftChildIndex; // Go here when the bit is 0
    u32 rightChildIndex; // Go here when the bit is 1
    u32 stringTableIndex;
    BfsarItemId itemId;
} BfsarSearchTreeNode;

typedef struct __attribute((packed)) {
    u32 rootNodeIndex;
    u32 nodeCount;
    BfsarSearchTreeNode nodes[0];
} BfsarStringSearchTree;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to STRG_MAGIC
    u32 blockSize;
    BfsarSectionRef stringTableRef; // BfsarStringTable
    BfsarSectionRef searchTreeRef;
} BfsarStringBlock;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u32 entryCount;
    BfsarSectionRef references[0];
} BfsarInfoReferenceTable;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to INFO_MAGIC
    u32 blockSize;

    u8 offsetOrigin[0];

    BfsarSectionRef soundInfoRtRef; // BfsarSectionRef (0x2100) > BfsarInfoReferenceTable > BfsarSoundInfo (0x2200)
    BfsarSectionRef soundGroupInfoRtRef; // BfsarSectionRef (0x2104) > BfsarInfoReferenceTable > (0x2204)
    BfsarSectionRef bankInfoRtRef; // BfsarSectionRef (0x2101) > BfsarInfoReferenceTable > (0x2206)
    BfsarSectionRef waveArchiveInfoRtRef; // BfsarSectionRef (0x2103) > BfsarInfoReferenceTable > (0x2207)
    BfsarSectionRef groupInfoRtRef; // BfsarSectionRef (0x2105) > BfsarInfoReferenceTable >  (0x2208)
    BfsarSectionRef playerInfoRtRef; // BfsarSectionRef (0x2102) > BfsarInfoReferenceTable > (0x2209)
    BfsarSectionRef fileInfoRtRef; // BfsarSectionRef (0x2106) > BfsarInfoReferenceTable > BfsarSectionRef (0x220A) > BfsarInternalFileInfo (0x220C) / BfsarExternalFileInfo (0x220D)
    BfsarSectionRef soundArchivePlayerInfoRef; // TODO
} BfsarInfoBlock;

typedef struct __attribute((packed)) {
    u32 fileIndex;
    BfsarItemId playerItemId;
    u8 initialVolume; // 0 is 0, 1 is 127, 2 is 256
    u8 remoteFilter;
    u16 _pad16;
    BfsarSectionRef dynamicRef; // Ref to stream (0x2201), wave (0x2202), or sequence (0x2203) info
    u32 flags; // TODO
} BfsarSoundInfo;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to FILE_MAGIC
    u32 blockSize;

    u8 data[0];
} BfsarFileBlock;

// If this file is stored in a group file instead of the
// file block, offset and filesize are set to 0xFFFFFFFF.
typedef struct __attribute((packed)) {
    BfsarSectionRef dataRef; // (0x1F00)
    u32 fileSize;
} BfsarInternalFileInfo;

typedef struct __attribute((packed)) {
    char filename[0]; // Null-terminated path to file
} BfsarExternalFileInfo;

void BfsarPreprocess(u8* bfsarData) {
    BfsarFileHeader* fileHeader = (BfsarFileHeader*)bfsarData;
    if (fileHeader->magic != FSAR_MAGIC)
        panic("BFSAR file header magic is nonmatching");

    if (fileHeader->majorVersion != BFSAR_MAJOR_VERSION)
        panic(
            "Expected BFSAR version %u.x.x, got %u.%u.%u instead",
            BFSAR_MAJOR_VERSION,
            fileHeader->majorVersion, fileHeader->minorVersion, fileHeader->patchVersion
        );

    if (fileHeader->boMark != BFSAR_BOMARK_LITTLE)
        panic("BFSAR invalid byte-order mark");

    if (fileHeader->boMark == BFSAR_BOMARK_BIG)
        panic("Big-endian BFSAR is not supported"); // TODO
}

void BfsarProcessFileInfo(u8* bfsarData) {
    BfsarFileHeader* fileHeader = (BfsarFileHeader*)bfsarData;

    BfsarLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == BLOCK_TYPE_INFO)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    BfsarInfoBlock* infoBlock = bfsarData + lookupEntry->blockOffset;
    if (infoBlock->magic != INFO_MAGIC)
        panic("BFSAR info block magic is nonmatching");
    
    BfsarInfoReferenceTable* referenceTable = infoBlock->offsetOrigin + infoBlock->fileInfoRtRef.sectionOffset;
    for (unsigned i = 0; i < referenceTable->entryCount; i++) {
        if (
            referenceTable->references[i].sectionType == 0 ||
            referenceTable->references[i].sectionOffset == 0xFFFFFFFF
        )
            continue;

        BfsarSectionRef* secondRef = referenceTable->offsetOrigin + referenceTable->references[i].sectionOffset;

        if (secondRef->sectionType == 0x220C) {
            BfsarInternalFileInfo* fileInfo = secondRef->offsetOrigin + secondRef->sectionOffset;

            printf("%u. Internal, size=%u\n", i+1, fileInfo->fileSize);
        }
        else if (secondRef->sectionType == 0x220D) {
            BfsarExternalFileInfo* fileInfo = secondRef->offsetOrigin + secondRef->sectionOffset;

            printf("%u. External, path=%s\n", i+1, fileInfo->filename);
        }
        else {
            printf("%u. Strange type, type=%u\n", i+1, secondRef->sectionType);
        }
    }
}

BfsarSectionRef* _BfsarGetFileFromIndex(BfsarInfoBlock* infoBlock, u32 index) {
    BfsarInfoReferenceTable* referenceTable = infoBlock->offsetOrigin + infoBlock->fileInfoRtRef.sectionOffset;

    return (BfsarSectionRef*)(referenceTable->offsetOrigin + referenceTable->references[index].sectionOffset);
}

BfsarFileBlock* _BfsarGetFileBlock(u8* bfsarData) {
    BfsarFileHeader* fileHeader = (BfsarFileHeader*)bfsarData;

    BfsarLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == BLOCK_TYPE_FILE)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    return (BfsarFileBlock*)(bfsarData + lookupEntry->blockOffset);
}

void BfsarProcess(u8* bfsarData) {
    BfsarFileHeader* fileHeader = (BfsarFileHeader*)bfsarData;

    BfsarLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == BLOCK_TYPE_INFO)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    BfsarInfoBlock* infoBlock = bfsarData + lookupEntry->blockOffset;
    if (infoBlock->magic != INFO_MAGIC)
        panic("BFSAR info block magic is nonmatching");

    BfsarFileBlock* fileBlock = _BfsarGetFileBlock(bfsarData);

    unsigned grpCnt = 0, bnkCnt = 0, warCnt = 0, wavCnt = 0, wsdCnt = 0, seqCnt = 0, stmCnt = 0;

    BfsarInfoReferenceTable* referenceTable = infoBlock->offsetOrigin + infoBlock->soundInfoRtRef.sectionOffset;
    for (unsigned i = 0; i < referenceTable->entryCount; i++) {
        if (
            referenceTable->references[i].sectionType == 0 ||
            referenceTable->references[i].sectionOffset == 0xFFFFFFFF
        )
            continue;

        BfsarSoundInfo* soundInfo = referenceTable->offsetOrigin + referenceTable->references[i].sectionOffset;

        char* fileName = "N/A";
        char* fileType = "N/A";

        BfsarSectionRef* fileRef = _BfsarGetFileFromIndex(infoBlock, soundInfo->fileIndex);
        if (fileRef->sectionType == 0x220D) {
            BfsarExternalFileInfo* fileInfo = fileRef->offsetOrigin + fileRef->sectionOffset;
            fileName = fileInfo->filename;
        }
        else if (fileRef->sectionType == 0x220C) {
            BfsarInternalFileInfo* fileInfo = fileRef->offsetOrigin + fileRef->sectionOffset;

            u32 fileMagic = *(u32*)(fileBlock->data + fileInfo->dataRef.sectionOffset);

            switch (fileMagic) {
            case IDENTIFIER_TO_U32('F', 'G', 'R', 'P'):
                fileType = "FGRP";
                grpCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'B', 'N', 'K'):
                fileType = "FBNK";
                bnkCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'W', 'A', 'R'):
                fileType = "FWAR";
                warCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'W', 'A', 'V'):
                fileType = "FWAV";
                wavCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'W', 'S', 'D'):
                fileType = "FWSD";
                wsdCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'S', 'E', 'Q'):
                fileType = "FSEQ";
                seqCnt++;
                break;
            case IDENTIFIER_TO_U32('F', 'S', 'T', 'M'):
                fileType = "FSTM";
                stmCnt++;
                break;
            default:
                break;
            }

            char path[64];
            sprintf(path, "dmp/RADO%u.B%s", i, fileType);

            // DUMP FILE
            FileHandle hndl;
            hndl.data_u8 = fileBlock->data + fileInfo->dataRef.sectionOffset;
            hndl.size = fileInfo->fileSize;

            if (strcasecmp(fileType, "fwav") == 0)
                FileWriteHandle(hndl, path);
        }

        printf(
            "Sound no. %u:\n"
            "   - File Index: %u\n"
            "   - Player File Type: %u\n"
            "   - Player File Index: %u\n"
            "   - Initial Volume: %u\n"
            "   - Remote Filter: %u\n"
            "   - External File: %s\n"
            "   - File Type: %s\n",
            i+1,
            soundInfo->fileIndex,
            soundInfo->playerItemId.fileType,
            soundInfo->playerItemId.fileIndex,
            (unsigned)soundInfo->initialVolume,
            (unsigned)soundInfo->remoteFilter,
            fileName,
            fileType
        );
    }

    //unsigned grpCnt = 0, bnkCnt = 0, warCnt = 0, wavCnt = 0, wsdCnt = 0, seqCnt = 0, stmCnt = 0;

    printf(
        "grpCnt : %u, bnkCnt : %u, warCnt : %u\n"
        "wavCnt : %u, wsdCnt : %u, seqCnt : %u\n"
        "stmCnt : %u\n",

        grpCnt, bnkCnt, warCnt, wavCnt,
        wsdCnt, seqCnt, stmCnt
    );
}

#endif // BFSAR_PROCESS_H
