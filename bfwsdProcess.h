#ifndef BFWSD_PROCESS_H
#define BFWSD_PROCESS_H

#include "common.h"

#define FWSD_MAGIC IDENTIFIER_TO_U32('F', 'W', 'S', 'D')

#define BFWSD_BOMARK_BIG (0xFFFE)
#define BFWSD_BOMARK_LITTLE (0xFEFF)

#define BFWSD_MAJOR_VERSION (1)

#define INFO_MAGIC IDENTIFIER_TO_U32('I', 'N', 'F', 'O')

typedef struct __attribute((packed)) {
    u16 blockType; // BLOCK_TYPE_x
    u16 _pad16;
    u32 blockOffset; // Relative to start of file
    u32 blockSize;
} BfwsdLookupEntry;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to FWSD_MAGIC
    u16 boMark; // Byte Order Mark: compare to BFWSD_BOMARK_BIG and BFWSD_BOMARK_LITTLE

    u16 headerSize; // Including block lookup table

    u8 patchVersion;
    u8 minorVersion;
    u8 majorVersion; // Compare to BFWSD_MAJOR_VERSION
    u8 _pad8;

    u32 fileSize;

    u16 blockCount;
    u16 _pad16;

    BfwsdLookupEntry lookupEntries[0];
} BfwsdFileHeader;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u16 sectionType; // SECTION_TYPE_x or 0
    u16 _pad16;
    u32 sectionOffset; // Relative to parent offsetOrigin (in most cases)
} BfwsdSectionRef;

void BfwsdPreprocess(u8* bfwsdData) {
    BfwsdFileHeader* fileHeader = (BfwsdFileHeader*)bfwsdData;
    if (fileHeader->magic != FWSD_MAGIC)
        panic("BFWSD file header magic is nonmatching");

    if (fileHeader->majorVersion != BFWSD_MAJOR_VERSION)
        panic(
            "Expected BFWSD version %u.x.x, got %u.%u.%u instead",
            BFWSD_MAJOR_VERSION,
            fileHeader->majorVersion, fileHeader->minorVersion, fileHeader->patchVersion
        );

    if (fileHeader->boMark != BFWSD_BOMARK_LITTLE)
        panic("BFWSD invalid byte-order mark");

    if (fileHeader->boMark == BFWSD_BOMARK_BIG)
        panic("Big-endian BFWSD is not supported"); // TODO
}

typedef struct __attribute((packed)) {
    u32 magic; // Compare to INFO_MAGIC
    u32 headerSize;

    u8 offsetOrigin[0];

    BfwsdSectionRef waveIdTableRtRef; // BfwsdSectionRef (0x0100) > BfwsdWaveIdTable
    BfwsdSectionRef waveSoundDataRtRef; // BfwsdSectionRef (0x0101) > BfwsdWaveSoundDataTable
} BfwsdInfoBlock;

typedef struct __attribute((packed)) {
    u32 fileIndex : 24;
    u32 fileType : 8;
} BfwsdItemId;

typedef struct __attribute((packed)) {
    BfwsdItemId waveArchiveItemId; // Should be of type 5 (Wave Archive)
    u32 waveFileIndex; // Index into archive
} BfwsdWaveId;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u32 entryCount;
    BfwsdSectionRef references[0]; // BfwsdSectionRef > BfwsdWaveId
} BfwsdWaveIdTable;

typedef struct __attribute((packed)) {
    // 0x1 = 0x0000XXYY: XX = Surround pan, YY = Pan
    // 0x2 = Pitch (float)
    // 0x100 = Send value (4x1 byte)
    // 0x200 = Offset to reference to adshr curve (0x0000)
    u32 flags;
} BfwsdWaveSoundInfo;

typedef struct __attribute((packed)) {
    u32 waveIdIndex; // Index into Wave Id table
    
    // 0x1 = Original key
    // 0x2 = Volume
    // 0x4 = 0x0000XXYY: XX = Surround pan, YY = Pan
    // 0x8 = Pitch (float)
    // 0x200 = Offset to reference to adshr curve (0x0000)
    u32 flags;
} BfwsdNoteInfo;

typedef struct __attribute((packed)) {
    u32 entryCount;
    BfwsdSectionRef references[0]; // BfwsdSectionRef > BfwsdNoteInfo
} BfwsdNoteInfoTable;

typedef struct __attribute((packed)) {
    BfwsdSectionRef waveSoundInfoRef;
    BfwsdSectionRef _unk08_ref;
    BfwsdSectionRef noteInfoRtRef;
} BfwsdWaveSoundData;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u32 entryCount;
    BfwsdSectionRef references[0]; // BfwsdSectionRef > BfwsdWaveSoundData
} BfwsdWaveSoundDataTable;

void BfwsdProcess(u8* bfwsdData) {
    BfwsdFileHeader* fileHeader = (BfwsdFileHeader*)bfwsdData;

    BfwsdLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == 0x6800)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    BfwsdInfoBlock* infoBlock = (BfwsdInfoBlock*)(bfwsdData + lookupEntry->blockOffset);
    if (infoBlock->magic != INFO_MAGIC)
        panic("BFWSD info block magic is nonmatching");

    (void)infoBlock->waveIdTableRtRef; // TODO
}

#endif // BFWSD_PROCESS_H
