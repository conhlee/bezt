#ifndef BFWAV_PROCESS_H
#define BFWAV_PROCESS_H

#include "common.h"

#include <stdlib.h>

#define FWAV_MAGIC IDENTIFIER_TO_U32('F', 'W', 'A', 'V')

#define BFWAV_BOMARK_BIG (0xFFFE)
#define BFWAV_BOMARK_LITTLE (0xFEFF)

#define BFWAV_MAJOR_VERSION (1)

#define INFO_MAGIC IDENTIFIER_TO_U32('I', 'N', 'F', 'O')

#define BFWAV_BLOCK_TYPE_INFO (0x7000)
#define BFWAV_BLOCK_TYPE_DATA (0x7001)

typedef struct __attribute((packed)) {
    u16 blockType; // BLOCK_TYPE_x
    u16 _pad16;
    u32 blockOffset; // Relative to start of file
    u32 blockSize;
} BfwavLookupEntry;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to FWAV_MAGIC
    u16 boMark; // Byte Order Mark: compare to BFWAV_BOMARK_BIG and BFWAV_BOMARK_LITTLE

    u16 headerSize; // Including block lookup table

    u8 patchVersion;
    u8 minorVersion;
    u8 majorVersion; // Compare to BFWAV_MAJOR_VERSION
    u8 _pad8;

    u32 fileSize;

    u16 blockCount;
    u16 _pad16;

    BfwavLookupEntry lookupEntries[0];
} BfwavFileHeader;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    u16 sectionType; // SECTION_TYPE_x or 0
    u16 _pad16;
    u32 sectionOffset; // Relative to parent offsetOrigin (in most cases)
} BfwavSectionRef;

typedef struct __attribute((packed)) {
    u8 offsetOrigin[0];
    BfwavSectionRef dataBlockRef;
    BfwavSectionRef dsppadpcmInfoRef;
    u32 _unk10;
} BfwavInfoTable;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to INFO_MAGIC
    u32 headerSize;

    u8 sampleFormat; // 0 = PCM8, 1 = PCM16, 2 = DSPADPCM

    u8 isLooped;

    u16 _pad16;

    u32 sampleRate;
    u32 loopStartSampleIdx;
    u32 sampleCount;

    /*
        The adjusted loop start sample is always less than the real loop start sample. If the adjusted loop start sample is X less than the real loop start sample, the last X samples of the wave file are skipped when the wave file is looped.
    */
    u32 adjustLoopStartSampleIdx;

    u8 offsetOrigin[0];

    u32 channelCount;

    BfwavSectionRef infoRef; // BfwavSectionRef (0x7100) > BfwavInfoTable
} BfwavInfoBlock;

void BfwavPreprocess(u8* bfwavData) {
    BfwavFileHeader* fileHeader = (BfwavFileHeader*)bfwavData;
    if (fileHeader->magic != FWAV_MAGIC)
        panic("BFWAV file header magic is nonmatching");

    if (fileHeader->majorVersion != BFWAV_MAJOR_VERSION)
        panic(
            "Expected BFWAV version %u.x.x, got %u.%u.%u instead",
            BFWAV_MAJOR_VERSION,
            fileHeader->majorVersion, fileHeader->minorVersion, fileHeader->patchVersion
        );

    if (fileHeader->boMark == BFWAV_BOMARK_BIG)
        panic("Big-endian BFWAV is not supported"); // TODO
    if (fileHeader->boMark != BFWAV_BOMARK_LITTLE)
        panic("BFWAV invalid byte-order mark");
}

BfwavInfoBlock* _BfwavGetInfoBlock(u8* bfwavData) {
    BfwavFileHeader* fileHeader = (BfwavFileHeader*)bfwavData;

    BfwavLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == BFWAV_BLOCK_TYPE_INFO)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    BfwavInfoBlock* infoBlock = (BfwavInfoBlock*)(bfwavData + lookupEntry->blockOffset);
    if (infoBlock->magic != INFO_MAGIC)
        panic("BFWAV info block magic is nonmatching");

    return infoBlock;
}

u32 BfwavGetSampleRate(u8* bfwavData) {
    return _BfwavGetInfoBlock(bfwavData)->sampleRate;
}
u32 BfwavGetChannelCount(u8* bfwavData) {
    return _BfwavGetInfoBlock(bfwavData)->channelCount;
}

// Dynamically allocated
s16* BfwavGetSamples(u8* bfwavData) {
    BfwavFileHeader* fileHeader = (BfwavFileHeader*)bfwavData;

    BfwavLookupEntry* lookupEntry = NULL;
    for (unsigned i = 0; i < fileHeader->blockCount; i++) {
        if (fileHeader->lookupEntries[i].blockType == BFWAV_BLOCK_TYPE_INFO)
            lookupEntry = fileHeader->lookupEntries + i;
    }
    if (lookupEntry == NULL)
        panic("Lookup Entry not found");

    BfwavInfoBlock* infoBlock = (BfwavInfoBlock*)(bfwavData + lookupEntry->blockOffset);
    if (infoBlock->magic != INFO_MAGIC)
        panic("BFWAV info block magic is nonmatching");
    
    if (infoBlock->channelCount > 2)
        panic("BFWAV channels > 2");
    
    s16* samples = (s16*)malloc(infoBlock->sampleCount * sizeof(s16));

    printf("format: %u\n", (unsigned)infoBlock->sampleFormat);

    return samples;
}

#endif // BFWAV_PROCESS_H
