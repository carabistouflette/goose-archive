/* SPDX-License-Identifier: Apache-2.0 */
#include "../native/archive.h"
#include <assert.h>
#include <stdlib.h>
#include <zlib.h>

static void inspect(const uint8_t *input, size_t size) {
    int64_t limits[] = {2097152,128,1048576,1048576,32,262144}, meta[3] = {0};
    uint8_t *out = malloc(262144);
    if (!out) abort();
    int64_t code = ga_call(0, (ga_input){input,(int64_t)size}, (ga_input){NULL,0},
        (ga_limits){limits,6}, (ga_output){out,262144}, (ga_meta){meta,3});
    assert(code >= 0 && code <= 5);
    if (code) assert(meta[1] == 0 && meta[2] == 0);
    else {
        size_t offset = 0;
        assert(meta[1] <= 262144 && meta[2] <= 128);
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
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!size || size > 1048576) return 0;
    inspect(data,size);
    /* Also reach tar parser without requiring mutations to repair gzip CRCs. */
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
    return 0;
}
