/* SPDX-License-Identifier: Apache-2.0 */
#define _GNU_SOURCE
#include "archive.h"
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#if !defined(__linux__)
#error "goose-archive currently supports Linux only"
#endif

enum { OK, INVALID, LIMIT, MALFORMED, UNSAFE, SYSTEM };
enum { GA_MAX_INPUT = 16777216, MAX_TAR = 67108864, MAX_ENTRIES = 4096,
       MAX_PATH = 1024, MAX_DEPTH = 32, MAX_MANIFEST = 4194304 };
struct item { char path[MAX_PATH + 1]; uint64_t size; bool directory, executable; };
struct context {
    uint8_t *tar;
    size_t length, count, packed;
    struct item *items;
    struct item **sorted;
};
static bool overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return false;
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}
static bool zeros(const uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; ++i) if (p[i]) return false;
    return true;
}
static int decode(ga_input input, size_t cap, struct context *c) {
    c->tar = malloc(cap + 1);
    if (!c->tar) { errno = ENOMEM; return SYSTEM; }
    z_stream stream = {0};
    int code = inflateInit2(&stream, 15 + 16);
    if (code != Z_OK) { errno = ENOMEM; return SYSTEM; }
    stream.next_in = (Bytef *)input.data;
    stream.avail_in = (uInt)input.len;
    stream.next_out = c->tar;
    stream.avail_out = (uInt)(cap + 1);
    code = inflate(&stream, Z_FINISH);
    c->length = stream.total_out;
    int result = c->length > cap ? LIMIT :
        code == Z_MEM_ERROR ? SYSTEM :
        code != Z_STREAM_END || stream.avail_in != 0 ? MALFORMED : OK;
    inflateEnd(&stream);
    if (result == SYSTEM) errno = ENOMEM;
    if (result != OK) return result;
    if (c->length < 1024 || c->length % 512 ||
        !zeros(c->tar + c->length - 1024, 1024)) return MALFORMED;
    return OK;
}
static struct archive *reader(const struct context *c) {
    struct archive *a = archive_read_new();
    if (!a) return NULL;
    if (archive_read_support_filter_none(a) != ARCHIVE_OK ||
        archive_read_support_format_tar(a) != ARCHIVE_OK ||
        archive_read_open_memory(a, c->tar, c->length) != ARCHIVE_OK) {
        archive_read_free(a); return NULL;
    }
    return a;
}
static int path_copy(const char *source, bool directory, int64_t depth_limit, char *out) {
    if (!source) return UNSAFE;
    size_t n = strnlen(source, MAX_PATH + 2);
    if (directory && n && source[n - 1] == '/') --n;
    if (!n || n > MAX_PATH) return UNSAFE;
    size_t start = 0, depth = 0;
    for (size_t i = 0; i <= n; ++i) {
        if (i < n) {
            unsigned char ch = (unsigned char)source[i];
            if (ch < 32 || ch >= 127 || ch == '\\' || ch == ':') return UNSAFE;
            if (ch != '/') continue;
        }
        size_t part = i - start;
        if (!part || (part == 1 && source[start] == '.') ||
            (part == 2 && source[start] == '.' && source[start + 1] == '.')) return UNSAFE;
        if (++depth > (size_t)depth_limit) return LIMIT;
        start = i + 1;
    }
    memcpy(out, source, n); out[n] = 0;
    return OK;
}
static int compare(const void *a, const void *b) {
    const struct item *const *x = a, *const *y = b;
    return strcmp((*x)->path, (*y)->path);
}
static bool file_ancestor(struct context *c, const char *path) {
    for (const char *p = path; *p; ++p) {
        if (*p != '/') continue;
        struct item key = {0};
        memcpy(key.path, path, (size_t)(p - path));
        struct item *pointer = &key;
        struct item **found = bsearch(&pointer, c->sorted, c->count, sizeof *c->sorted, compare);
        if (found && !(*found)->directory) return true;
    }
    return false;
}
/* Consume data blocks in both passes: no skip path that could hide truncation.
 * Sparse and non-contiguous data is rejected, not expanded into holes. */
static int data(struct archive *a, uint64_t expected, int fd) {
    uint64_t size = 0;
    for (;;) {
        const void *block; size_t n; la_int64_t offset;
        int code = archive_read_data_block(a, &block, &n, &offset);
        if (code == ARCHIVE_EOF) return size == expected ? OK : MALFORMED;
        if (code != ARCHIVE_OK || offset < 0 || (uint64_t)offset != size ||
            n > expected - size) return MALFORMED;
        if (fd >= 0) {
            size_t done = 0;
            while (done < n) {
                size_t chunk = n - done > 65536 ? 65536 : n - done;
                ssize_t written = write(fd, (const uint8_t *)block + done, chunk);
                if (written < 0 && errno == EINTR) continue;
                if (written <= 0) { if (!written) errno = EIO; return SYSTEM; }
                done += (size_t)written;
            }
        }
        size += n;
    }
}
static int validate(struct context *c, ga_limits limits) {
    struct archive *a = reader(c);
    if (!a) return MALFORMED;
    struct archive_entry *entry;
    uint64_t total = 0;
    la_int64_t payload_end = 0;
    int result = OK, code;
    while ((code = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (c->count == (size_t)limits.data[1]) { result = LIMIT; break; }
        struct item *item = &c->items[c->count];
        mode_t type = archive_entry_filetype(entry);
        if ((type != AE_IFREG && type != AE_IFDIR) || archive_entry_hardlink(entry) ||
            archive_entry_symlink(entry) || archive_entry_sparse_count(entry) != 0) { result = UNSAFE; break; }
        item->directory = type == AE_IFDIR;
        result = path_copy(archive_entry_pathname(entry), item->directory, limits.data[4], item->path);
        if (result) break;
        la_int64_t size = archive_entry_size(entry);
        if (!archive_entry_size_is_set(entry) || size < 0 || (item->directory && size)) { result = MALFORMED; break; }
        if ((uint64_t)size > (uint64_t)limits.data[2] ||
            (uint64_t)size > (uint64_t)limits.data[3] - total) { result = LIMIT; break; }
        item->size = (uint64_t)size;
        item->executable = (archive_entry_perm(entry) & 0111) != 0;
        total += item->size;
        size_t packed = strlen(item->path) + 10;
        if (packed > (size_t)limits.data[5] - c->packed) { result = LIMIT; break; }
        c->packed += packed;
        result = data(a, item->size, -1);
        if (result) break;
        payload_end = archive_filter_bytes(a, 0);
        c->sorted[c->count] = item;
        ++c->count;
    }
    if (!result && code != ARCHIVE_EOF) result = MALFORMED;
    if (!result) {
        la_int64_t used = archive_filter_bytes(a, 0);
        if (payload_end < 0 || (uint64_t)payload_end > c->length ||
            c->length - (size_t)payload_end < 1024 ||
            !zeros(c->tar + payload_end, c->length - (size_t)payload_end) ||
            used < payload_end || (uint64_t)used > c->length) result = MALFORMED;
    }
    archive_read_free(a);
    if (result) return result;
    qsort(c->sorted, c->count, sizeof *c->sorted, compare);
    for (size_t i = 1; i < c->count; ++i)
        if (!strcmp(c->sorted[i - 1]->path, c->sorted[i]->path)) return UNSAFE;
    for (size_t i = 0; i < c->count; ++i)
        if (file_ancestor(c, c->items[i].path)) return UNSAFE;
    return OK;
}
static int open_directory(int parent, const char *name) {
    return openat(parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
}
/* Parent is caller-controlled; each ancestor is opened without following links. */
static int create_root(ga_input destination) {
    char path[4096];
    memcpy(path, destination.data, (size_t)destination.len);
    path[destination.len] = 0;
    int fd = open_directory(AT_FDCWD, "/");
    if (fd < 0) return -1;
    char *part = path + 1, *slash;
    while ((slash = strchr(part, '/'))) {
        *slash = 0;
        int next = open_directory(fd, part), saved = errno;
        close(fd);
        if (next < 0) { errno = saved; return -1; }
        fd = next; part = slash + 1;
    }
    int next = -1;
    if (mkdirat(fd, part, 0700) == 0) next = open_directory(fd, part);
    int saved = errno;
    close(fd); errno = saved;
    return next;
}
static int create_entry(int root, const struct item *item) {
    char path[MAX_PATH + 1];
    memcpy(path, item->path, strlen(item->path) + 1);
    int fd = fcntl(root, F_DUPFD_CLOEXEC, 0);
    if (fd < 0) return -1;
    char *part = path, *slash;
    while ((slash = strchr(part, '/'))) {
        *slash = 0;
        if (mkdirat(fd, part, 0700) < 0 && errno != EEXIST) {
            int saved = errno; close(fd); errno = saved; return -1;
        }
        int next = open_directory(fd, part), saved = errno;
        close(fd);
        if (next < 0) { errno = saved; return -1; }
        fd = next; part = slash + 1;
    }
    int next;
    if (item->directory) {
        if (mkdirat(fd, part, 0700) < 0 && errno != EEXIST) next = -1;
        else next = open_directory(fd, part);
    } else next = openat(fd, part, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                         item->executable ? 0700 : 0600);
    int saved = errno;
    close(fd); errno = saved;
    return next;
}
static int extract(struct context *c, ga_input destination) {
    struct archive *a = reader(c);
    if (!a) return MALFORMED;
    int root = create_root(destination);
    if (root < 0) { int saved = errno; archive_read_free(a); errno = saved; return SYSTEM; }
    int result = OK;
    for (size_t i = 0; i < c->count; ++i) {
        struct archive_entry *entry;
        if (archive_read_next_header(a, &entry) != ARCHIVE_OK) { result = MALFORMED; break; }
        int fd = create_entry(root, &c->items[i]);
        if (fd < 0) { result = SYSTEM; break; }
        result = data(a, c->items[i].size, c->items[i].directory ? -1 : fd);
        int saved = errno;
        if (close(fd) < 0 && !result) { result = SYSTEM; saved = errno; }
        errno = saved;
        if (result) break;
    }
    int saved = errno;
    close(root); archive_read_free(a); errno = saved;
    return result;
}
static bool destination_valid(ga_input path) {
    if (!path.data || path.len < 2 || path.len > 4095 || path.data[0] != '/') return false;
    size_t start = 1;
    for (size_t i = 1; i <= (size_t)path.len; ++i) {
        if (i < (size_t)path.len && path.data[i] != '/') { if (!path.data[i]) return false; continue; }
        size_t n = i - start;
        if (!n || (n == 1 && path.data[start] == '.') ||
            (n == 2 && path.data[start] == '.' && path.data[start + 1] == '.')) return false;
        start = i + 1;
    }
    return true;
}
int64_t ga_call(int64_t op, ga_input input, ga_input destination, ga_limits limits,
                ga_output manifest, ga_meta meta) {
    if (!meta.data || meta.len != 3 || !limits.data || limits.len != 6 ||
        !input.data || input.len < 1 || input.len > GA_MAX_INPUT ||
        destination.len < 0 || destination.len > 4095 ||
        manifest.len < 0 || manifest.len > MAX_MANIFEST || (manifest.len && !manifest.data)) return INVALID;
    /* Validate aliasing before writing metadata or reading options. */
    const void *regions[] = {input.data, destination.data, limits.data, manifest.data, meta.data};
    size_t sizes[] = {(size_t)input.len, (size_t)destination.len, 48, (size_t)manifest.len, 24};
    for (size_t i = 0; i < 5; ++i) for (size_t j = i + 1; j < 5; ++j)
        if (overlap(regions[i], sizes[i], regions[j], sizes[j])) return INVALID;
    memset(meta.data, 0, 24);
    const int64_t caps[] = {MAX_TAR, MAX_ENTRIES, MAX_TAR, MAX_TAR, MAX_DEPTH, MAX_MANIFEST};
    for (size_t i = 0; i < 6; ++i) if (limits.data[i] < 0 || limits.data[i] > caps[i]) return INVALID;
    if ((op != 0 && op != 1) || (op == 0 && destination.len) ||
        (op == 1 && !destination_valid(destination))) return INVALID;
    struct context c = {0};
    c.items = malloc(((size_t)limits.data[1] + 1) * sizeof *c.items);
    c.sorted = malloc(((size_t)limits.data[1] + 1) * sizeof *c.sorted);
    int result;
    if (!c.items || !c.sorted) { errno = ENOMEM; result = SYSTEM; }
    else {
        result = decode(input, (size_t)limits.data[0], &c);
        if (!result) result = validate(&c, limits);
        if (!result && op == 0 && c.packed > (size_t)manifest.len) result = LIMIT;
        if (!result && op == 1) result = extract(&c, destination);
        if (!result && op == 0) {
            size_t offset = 0;
            for (size_t i = 0; i < c.count; ++i) {
                struct item *item = &c.items[i];
                manifest.data[offset++] = item->directory ? 1 : 0;
                for (unsigned b = 0; b < 8; ++b) manifest.data[offset++] = (uint8_t)(item->size >> (8 * b));
                size_t n = strlen(item->path) + 1;
                memcpy(manifest.data + offset, item->path, n); offset += n;
            }
            meta.data[1] = (int64_t)c.packed;
        }
        if (!result) meta.data[2] = (int64_t)c.count;
    }
    if (result == SYSTEM) meta.data[0] = errno;
    free(c.sorted); free(c.items); free(c.tar);
    return result;
}
