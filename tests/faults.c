/* SPDX-License-Identifier: Apache-2.0 */
/* Test-only link wrappers. Never built into the published CMake target. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

static int mode, trigger, calls;
static size_t bytes;
void ga_test_fault(int value, int at) {
    mode = value; trigger = at; calls = 0; bytes = 0;
}
ssize_t __real_write(int, const void *, size_t);
int __real_close(int);
int __real_openat(int, const char *, int, ...);
int __real_mkdirat(int, const char *, mode_t);
int __real_fcntl(int, int, ...);

ssize_t __wrap_write(int fd, const void *data, size_t count) {
    if (mode == 1 && count > 3) count = 3;
    if (mode == 2 && calls++ == 0) { errno = EINTR; return -1; }
    if (mode == 3) {
        if (bytes >= 7) { errno = ENOSPC; return -1; }
        if (count > 7 - bytes) count = 7 - bytes;
    }
    if (mode == 4) return 0;
    ssize_t result = __real_write(fd, data, count);
    if (result > 0) bytes += (size_t)result;
    return result;
}
int __wrap_close(int fd) {
    struct stat st;
    int regular = mode == 5 && fstat(fd, &st) == 0 && S_ISREG(st.st_mode);
    int result = __real_close(fd);
    if (regular && result == 0) { errno = EIO; return -1; }
    return result;
}
int __wrap_openat(int fd, const char *path, int flags, ...) {
    mode_t permissions = 0;
    if (flags & O_CREAT) {
        va_list args; va_start(args, flags);
        permissions = (mode_t)va_arg(args, int); va_end(args);
    }
    if (mode == 6 && ++calls == trigger) { errno = EMFILE; return -1; }
    return __real_openat(fd, path, flags, permissions);
}
int __wrap_mkdirat(int fd, const char *path, mode_t permissions) {
    if (mode == 7 && ++calls == trigger) { errno = ENOSPC; return -1; }
    return __real_mkdirat(fd, path, permissions);
}
int __wrap_fcntl(int fd, int command, ...) {
    /* The adapter's only fcntl call uses F_DUPFD_CLOEXEC with an int argument. */
    va_list args; va_start(args, command);
    int arg = va_arg(args, int); va_end(args);
    if (mode == 8 && command == F_DUPFD_CLOEXEC) { errno = EMFILE; return -1; }
    return __real_fcntl(fd, command, arg);
}
