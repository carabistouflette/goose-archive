/* SPDX-License-Identifier: Apache-2.0 */
#include "../native/archive.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static void inspect(const uint8_t *input, size_t size) {
    int64_t limits[] = {2097152,128,1048576,1048576,32,262144}, meta[3] = {0};
    size_t capacity = 262144;
    if (size >= 4) {
        uint64_t hint = 0;
        for (unsigned b = 0; b < 4; ++b) hint |= (uint64_t)input[size - 4 + b] << (b * 8);
        uint64_t bound = (hint / 512 < 128 ? hint / 512 : 128) * 1034;
        if (bound < capacity) capacity = (size_t)bound;
    }
    uint8_t *out = malloc(capacity + 16);
    if (!out) abort();
    memset(out + capacity, 0xa5, 16);
    int64_t code = ga_call(0, (ga_input){input,(int64_t)size}, (ga_input){NULL,0},
        (ga_limits){limits,6}, (ga_output){out,(int64_t)capacity}, (ga_meta){meta,3});
    for (size_t i = 0; i < 16; ++i) assert(out[capacity + i] == 0xa5);
    assert(code >= 0 && code <= 5);
    if (code) assert(meta[1] == 0 && meta[2] == 0);
    else {
        size_t offset = 0;
        assert(meta[1] >= 0 && (uint64_t)meta[1] <= capacity && meta[2] >= 0 && meta[2] <= 128);
        for (int64_t i = 0; i < meta[2]; ++i) {
            assert(offset + 10 <= (size_t)meta[1]);
            assert(out[offset] <= 1);
            offset += 9;
            size_t start = offset;
            while (offset < (size_t)meta[1] && out[offset]) ++offset;
            assert(offset > start && offset < (size_t)meta[1]);
            ++offset;
        }
        assert(offset == (size_t)meta[1]);
    }
    free(out);
}
static void wrapped(const uint8_t *data, size_t size) {
    /* Reach tar parsing without requiring mutations to repair gzip CRCs. */
    z_stream stream = {0};
    if (deflateInit2(&stream, 1, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK) abort();
    size_t cap = deflateBound(&stream, (uLong)size);
    uint8_t *packed = malloc(cap);
    if (!packed) abort();
    stream.next_in = (Bytef *)data; stream.avail_in = (uInt)size;
    stream.next_out = packed; stream.avail_out = (uInt)cap;
    if (deflate(&stream, Z_FINISH) == Z_STREAM_END) inspect(packed, stream.total_out);
    deflateEnd(&stream);
    free(packed);
}
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!size || size > 1048576) return 0;
    inspect(data, size);
    wrapped(data, size);
    if (size >= 512) {
        /* Repair only the first tar header checksum, without interpreting fields.
         * Mutated paths/types/sizes can then exercise policy beyond checksum errors. */
        uint8_t *header = malloc(size);
        if (!header) abort();
        memcpy(header, data, size);
        memset(header + 148, ' ', 8);
        unsigned sum = 0;
        for (size_t i = 0; i < 512; ++i) sum += header[i];
        for (unsigned i = 0; i < 6; ++i) { header[153 - i] = (uint8_t)('0' + (sum & 7)); sum >>= 3; }
        header[154] = 0; header[155] = ' ';
        wrapped(header, size);
        free(header);
    }
    return 0;
}
