#include "compression.h"

#include <stdlib.h>

#include <zlib.h>
#include <zstd.h>

CompressionResult decompressZlib(u8* deflatedData, u32 deflatedSize, u32 inflatedSize) {
    CompressionResult result;

    result.size = inflatedSize;

    result.data_void = malloc(inflatedSize);
    if (result.data_void == NULL)
        panic("decompressZlib: malloc failed");

    z_stream sInflate;
    sInflate.zalloc = Z_NULL;
    sInflate.zfree = Z_NULL;
    sInflate.opaque = Z_NULL;

    sInflate.avail_in = deflatedSize;
    sInflate.next_in = deflatedData;
    sInflate.avail_out = result.size;
    sInflate.next_out = result.data_u8;

    if (inflateInit(&sInflate) != Z_OK)
        panic("decompressZlib: inflateInit failed");
    if (inflate(&sInflate, Z_NO_FLUSH) != Z_STREAM_END)
        panic("decompressZlib: inflate fail");
    inflateEnd(&sInflate);

    return result;
}

CompressionResult decompressZstd(u8* deflatedData, u32 deflatedSize, u32 inflatedSize) {
    CompressionResult result;

    result.size = inflatedSize;

    result.data_void = malloc(inflatedSize);
    if (result.data_void == NULL)
        panic("decompressZstd: malloc failed");

    u64 zstdResult = ZSTD_decompress(
        result.data_void, inflatedSize,
        deflatedData, deflatedSize
    );

    if (ZSTD_isError(zstdResult)) {
        free(result.data_void);
        panic("decompressZstd: ZSTD_decompress returned error");
    }

    return result;
}