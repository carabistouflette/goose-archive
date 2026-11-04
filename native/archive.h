/* SPDX-License-Identifier: Apache-2.0 */
#ifndef GOOSE_ARCHIVE_H
#define GOOSE_ARCHIVE_H
#include <stdint.h>
typedef struct { const uint8_t *data; int64_t len; } ga_input;
typedef struct { uint8_t *data; int64_t len; } ga_output;
typedef struct { const int64_t *data; int64_t len; } ga_limits;
typedef struct { int64_t *data; int64_t len; } ga_meta;
/* limits: tar bytes, entries, file bytes, total bytes, depth, manifest bytes.
 * meta: errno, manifest length, entry count. No pointers retained.
 * op 0 inspect, 1 extract; manifest records: kind byte, LE u64 size, path NUL.
 * Manifest capacity may be below the configured cap; actual records must fit.
 * All input/output regions must be disjoint and live throughout the call. */
int64_t ga_call(int64_t op, ga_input input, ga_input destination,
                ga_limits limits, ga_output manifest, ga_meta meta);
#endif
