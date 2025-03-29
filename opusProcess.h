#ifndef OPUS_PROCESS_H
#define OPUS_PROCESS_H

#include <stdlib.h>

#include <string.h>

#include <opus.h>

#include "list.h"

#include "common.h"

#define CHUNK_FILE_HEADER_ID (0x80000001)
#define OPUS_VERSION (0)

//#define CHUNK_CONTEXT_ID (0x80000003)

#define CHUNK_DATA_ID (0x80000004)

typedef struct __attribute((packed)) {
    u32 chunkId; // Compare to CHUNK_FILE_HEADER_ID
    u32 chunkSize; // Not including chunkId and chunkSize. Usually 0x24

    u8 version; // Compare to OPUS_VERSION

    u8 channelCount;

    u16 frameSize; // Frame size if CBR, 0 if VBR

    u32 sampleRate;

    u32 dataOffset;
    u32 _unk14; // 'frame data offset' (seek table? not seen)
    u32 contextOffset;

    u16 preSkipSamples; // Pre-skip sample count

    u16 _pad16;
} OpusFileHeader;

typedef struct __attribute((packed)) {
    u32 chunkId; // Compare to CHUNK_FILE_HEADER_ID
    u32 chunkSize; // Not including chunkId and chunkSize

    u8 data[0]; // OPUS data
} OpusDataChunk;

typedef struct __attribute((packed)) {
    u32 length;
    u32 finalRange;
    u8 data[0];
} OpusFrame;

#define RIFF_MAGIC IDENTIFIER_TO_U32('R', 'I', 'F', 'F')
#define WAVE_MAGIC IDENTIFIER_TO_U32('W', 'A', 'V', 'E')
#define FMT__MAGIC IDENTIFIER_TO_U32('f', 'm', 't', ' ')
#define DATA_MAGIC IDENTIFIER_TO_U32('d', 'a', 't', 'a')

typedef struct __attribute((packed)) {
    u32 riffMagic; // Compare to RIFF_MAGIC
    u32 fileSize; // Not including magic and fileSize

    u32 waveMagic; // Compare to WAVE_MAGIC
} WavFileHeader;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to FMT__MAGIC
    u32 chunkSize; // 16 in our case, but could be different

    u16 format; // 0x0001 = PCM, 0x0003 = Float, 0x0006 = A-Law, 0x0007 = Mu-Law

    u16 channelCount;
    u32 sampleRate;

    u32 dataRate; // Bytes per second (sampleRate * sampleSize * channelCount)

    u16 blockSize; // sampleSize * channelCount

    u16 bitsPerSample; // 8 * sampleSize
} WavFmtChunk;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to DATA_MAGIC
    u32 chunkSize;
} WavDataChunk;

void OpusPreprocess(u8* opusData) {
    OpusFileHeader* fileHeader = (OpusFileHeader*)opusData;
    if (fileHeader->chunkId != CHUNK_FILE_HEADER_ID)
        panic("OPUS file header ID is nonmatching");

    //if (fileHeader->sampleRate != 48000)
    //    panic("Expected OPUS sample rate to be 48000, got %u instead", fileHeader->sampleRate);

    if (fileHeader->contextOffset != 0)
        warn("OPUS context is present but will be ignored");

    OpusDataChunk* dataChunk = (OpusDataChunk*)(opusData + fileHeader->dataOffset);
    if (dataChunk->chunkId != CHUNK_DATA_ID)
        panic("OPUS data chunk ID is nonmatching");
}

void OpusProcess(u8* opusData, const char* path) {
    OpusFileHeader* fileHeader = (OpusFileHeader*)opusData;
    OpusDataChunk* dataChunk = (OpusDataChunk*)(opusData + fileHeader->dataOffset);

    int error;
    OpusDecoder* decoder = opus_decoder_create(fileHeader->sampleRate, fileHeader->channelCount, &error);
    if (error != OPUS_OK)
        panic("OpusProcess: opus_decoder_create fail : %s", opus_strerror(error));

    const unsigned coFrameSize =
        fileHeader->frameSize ? fileHeader->frameSize :
        (fileHeader->sampleRate / 100 * fileHeader->sampleRate);

    s16* pcmOut = (s16*)malloc(coFrameSize * sizeof(s16));
    if (!pcmOut)
        panic("OpusProcess: malloc fail");
    
    ListData sampleList;
    ListInit(&sampleList, sizeof(s16), coFrameSize);

    unsigned offset = 0;

    while (offset < dataChunk->chunkSize) {
        OpusFrame* opusFrame = (OpusFrame*)(dataChunk->data + offset);
        u32 length = __builtin_bswap32(opusFrame->length);

        int frameSize = opus_decode(decoder, opusFrame->data, length, pcmOut, coFrameSize, 0);
        if (frameSize < 0)
            panic("OpusProcess: opus_decode fail : %s", opus_strerror(frameSize));

        ListAddRange(&sampleList, pcmOut, frameSize * fileHeader->channelCount);

        offset += sizeof(OpusFrame) + length;

        //printf("Frame OK\n");
    }

    free(pcmOut);

    opus_decoder_destroy(decoder);

    {
        FILE* fp = fopen(path, "wb");
        if (fp == NULL)
            panic("OpusProcess: fopen failed (path : %s)", path);

        WavFileHeader wavFileHeader;
        WavFmtChunk wavFmtChunk;
        WavDataChunk wavDataChunk;

        wavFileHeader.riffMagic = RIFF_MAGIC;
        wavFileHeader.waveMagic = WAVE_MAGIC;

        wavFmtChunk.magic = FMT__MAGIC;
        wavFmtChunk.chunkSize = 16; // Size of the fmt chunk (16 for PCM)

        wavFmtChunk.format = 0x0001; // PCM format
        wavFmtChunk.channelCount = fileHeader->channelCount;
        wavFmtChunk.sampleRate = fileHeader->sampleRate;
        wavFmtChunk.dataRate = fileHeader->sampleRate * sizeof(s16) * fileHeader->channelCount;
        wavFmtChunk.blockSize = sizeof(s16) * fileHeader->channelCount;
        wavFmtChunk.bitsPerSample = 8 * sizeof(s16);

        wavDataChunk.magic = DATA_MAGIC;
        wavDataChunk.chunkSize = sampleList.elementCount * sizeof(s16);

        wavFileHeader.fileSize =
            sizeof(WavFileHeader) + sizeof(WavFmtChunk) +
            sizeof(WavDataChunk) + wavDataChunk.chunkSize - 8; // Subtract 8 for RIFF header size

        fwrite(&wavFileHeader, sizeof(WavFileHeader), 1, fp);
        fwrite(&wavFmtChunk, sizeof(WavFmtChunk), 1, fp);
        fwrite(&wavDataChunk, sizeof(WavDataChunk), 1, fp);

        fwrite(sampleList.data, sampleList.elementSize, sampleList.elementCount, fp);

        fclose(fp);
    }

    ListDestroy(&sampleList);
}


#endif // OPUS_PROCESS_H
