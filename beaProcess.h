#ifndef BEA_PROCESS_H
#define BEA_PROCESS_H

#include "common.h"

#include "compression.h"

#define SCNE_MAGIC IDENTIFIER_TO_U32('S', 'C', 'N', 'E')
#define ASST_MAGIC IDENTIFIER_TO_U32('A', 'S', 'S', 'T')
#define _STR_MAGIC IDENTIFIER_TO_U32('_', 'S', 'T', 'R')
#define _RLT_MAGIC IDENTIFIER_TO_U32('_', 'R', 'L', 'T')

#define BEA_BOMARK_BIG 0xFFFE
#define BEA_BOMARK_LITTLE 0xFEFF

typedef struct __attribute((packed)) {
    u16 size;
    char content[]; // String is not always null-terminated.
} BeaString;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to ASST_MAGIC

    u32 nextBlockOffset; // Relative to start of this block
    u32 blockSize;

    u32 _pad32_0;

    // End of standard header
    // Begin of BEA-specific data

    u8 compressionAlgorithm; // 0 = none, 1 = zlib, 2 = zstd
    u8 _pad8;

    u16 alignmentBytes; // ??

    u32 compressedSize;
    u32 decompressedSize;

    u32 _pad32_1;

    u64 compressedDataOffset;
    BeaString* filename;
} BeaFileInfoBlock;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to SCNE_MAGIC
    u32 _pad32;

    u8  versionMicro;
    u8  versionMinor;
    u16 versionMajor;

    u16 boMark; // Byte Order Mark: compare to BEA_BOMARK_BIG and BEA_BOMARK_LITTLE

    u8 alignment; // ? for what

    u8 addressSize; // ??? what

    u32 filenameOffset; // Might be unused

    u16 relocated; // Has this file been relocated / mapped?

    u16 firstBlockOffset;
    u32 relocationTableOffset;

    u32 fileSize;

    // End of standard header
    // Begin of BEA-specific data

    u16 subFileCount; // 0x20

    u16 _unk02; // 0x22
    u32 _unk04; // 0x24

    BeaFileInfoBlock** fileInfoBlockPtrs; // 0x28
    void* dictionary; // 0x30

    u64 _unk18; // 0x38

    BeaString* archiveName; // 0x40
} BeaFileHeader;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to _STR_MAGIC

    u32 nextBlockOffset; // Relative to start of this block
    u32 blockSize;

    u32 _pad32_0;

    u32 stringCount;

    u8 data[]; // BeaString (all data is aligned to 0x2)
} BeaStringPool;

typedef struct __attribute((packed)) {
    u64 address;

    u32 fileOffset;
    u32 fileSize;

    u32 firstRelocationIndex;
    u32 relocationCount;
} BeaRelocationSection;

typedef struct __attribute((packed)) {
    u32 fileOffset; // 0xDAE8 is the first one

    u16 chunkCount;

    u8 relocatedWrdsPerChunk; // seems to be always 1
    u8 nonRelocatedWrdsPerChunk; // seems to be always 0
} BeaRelocation;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to _RLT_MAGIC

    u32 tableOffset; // Offset of this table

    //63672

    //34792

    u32 sectionCount;

    u32 _pad32;
} BeaRelocationTable;

void _BeaRelocate(u8* beaData) {
    BeaFileHeader* fileHeader = (BeaFileHeader*)beaData;

    if (fileHeader->relocated != 0)
        return;

    BeaRelocationTable* relocationTable = (BeaRelocationTable*)(beaData + fileHeader->relocationTableOffset);
    if (relocationTable->magic != _RLT_MAGIC)
        panic("BEA relocation table magic is nonmatching");

    for (unsigned i = 0; i < relocationTable->sectionCount; i++) {
        BeaRelocationSection* section = (BeaRelocationSection*)(relocationTable + 1) + i;
    
        for (unsigned j = 0; j < section->relocationCount; j++) {
            BeaRelocation* relocation = (BeaRelocation*)(section + 1) + j;

            printf("REL fo=%u,   cc=%u, rwpc=%u, nrwpc=%u\n",
                relocation->fileOffset,
                relocation->chunkCount,
                relocation->relocatedWrdsPerChunk,
                relocation->nonRelocatedWrdsPerChunk
            );

            u64* value = (u64*)(beaData + section->fileOffset + relocation->fileOffset);
            *value += (u64)beaData;
        }
    }

    fileHeader->relocated = 1;
}

void BeaPreprocess(u8* beaData) {
    BeaFileHeader* fileHeader = (BeaFileHeader*)beaData;
    if (fileHeader->magic != SCNE_MAGIC)
        panic("BEA file header magic is nonmatching");

    if (fileHeader->boMark == BEA_BOMARK_BIG)
        panic("BEA is big-endian");
    if (fileHeader->boMark != BEA_BOMARK_LITTLE)
        panic("BEA has invalid endian");

    _BeaRelocate(beaData);
}

void BeaExtractAll(u8* beaData) {
    BeaFileHeader* fileHeader = (BeaFileHeader*)beaData;

    BeaString* archiveName = fileHeader->archiveName;
    printf("Archive: %.*s\n", (int)archiveName->size, archiveName->content);

    printf("Extracting %u files: \n", fileHeader->subFileCount);
    for (unsigned i = 0; i < fileHeader->subFileCount; i++) {
        BeaFileInfoBlock* fileInfoBlock = fileHeader->fileInfoBlockPtrs[i];

        BeaString* name = fileInfoBlock->filename;
        printf("    - \"%.*s\" (%ukb).. ", (int)name->size, name->content, fileInfoBlock->decompressedSize / 1024);

        char fullPath[128];
        sprintf(
            fullPath,
            "%.*s" PATH_SEPARATOR_S "%.*s",
            (int)archiveName->size, archiveName->content,
            (int)name->size, name->content
        );

        char directoryPath[128];
        sprintf(
            directoryPath, "%.*s",
            (int)(getFilename(fullPath) - fullPath), fullPath    
        );

        createDirectoryTree(directoryPath);

        CompressionResult decompressResult;

        // for some reason fileInfoBlock->compressedData isn't included in the
        // relocation. We do it manually then

        u8* compressedData = beaData + fileInfoBlock->compressedDataOffset;

        switch (fileInfoBlock->compressionAlgorithm) {
        case 0:
            decompressResult.data_u8 = compressedData;
            decompressResult.size = fileInfoBlock->decompressedSize;
            break;

        case 1:
            decompressResult = decompressZlib(
                compressedData,
                fileInfoBlock->compressedSize,
                fileInfoBlock->decompressedSize
            );
            break;
        case 2:
            decompressResult = decompressZstd(
                compressedData,
                fileInfoBlock->compressedSize,
                fileInfoBlock->decompressedSize
            );
            break;
        
        default:
            break;
        }

        FileHandle hndl;
        hndl.data_void = decompressResult.data_void;
        hndl.size = decompressResult.size;

        FileWriteHandle(hndl, fullPath);

        free(decompressResult.data_void);

        LOG_OK;
    }
}

#endif // BEA_PROCESS_H