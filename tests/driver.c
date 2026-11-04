/* SPDX-License-Identifier: Apache-2.0 */
#include "../native/archive.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ga_test_fault(int mode, int at);
static int descriptors(void) {
    DIR *dir = opendir("/proc/self/fd");
    assert(dir);
    int count = 0;
    while (readdir(dir)) ++count;
    assert(closedir(dir) == 0);
    return count;
}
static void exact(void *data, size_t size) {
    if (fread(data, 1, size, stdin) != size) exit(2);
}
static void selftest(void) {
    int64_t limits[] = {10240, 1, 10, 10, 2, 100}, meta[3] = {11,22,33};
    uint8_t bytes[100] = {0}, output[100];
    ga_input input = {bytes, 100}, empty = {NULL, 0};
    ga_limits options = {limits, 6};
    ga_output out = {output, 100};
    ga_meta status = {meta, 3};
    assert(ga_call(0, input, empty, options, (ga_output){bytes,100}, status) == 1);
    assert(meta[0] == 11);
    assert(ga_call(0, input, empty, options, out, (ga_meta){(int64_t *)bytes,3}) == 1);
    assert(ga_call(0, input, empty, options, out, (ga_meta){NULL,3}) == 1);
    assert(ga_call(0, (ga_input){NULL,100}, empty, options, out, status) == 1);
    assert(ga_call(0, input, empty, (ga_limits){limits,5}, out, status) == 1);
    for (int i = 0; i < 6; ++i) {
        int64_t saved = limits[i]; limits[i] = -1;
        assert(ga_call(0, input, empty, options, out, status) == 1);
        limits[i] = saved;
    }
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--selftest")) { selftest(); return 0; }
    int64_t request[10]; exact(request, sizeof request);
    if (request[0] < 0 || request[0] > 1 || request[1] < 0 || request[1] > 16777216 ||
        request[2] < 0 || request[2] > 4095 || request[9] < 0 || request[9] > 4194304) return 2;
    uint8_t *input = malloc((size_t)request[1] + 1), *dest = malloc((size_t)request[2] + 1);
    uint8_t *out = malloc(4194304);
    if (!input || !dest || !out) return 2;
    exact(input, (size_t)request[1]); exact(dest, (size_t)request[2]);
    int64_t response[4] = {0};
    const char *fault = getenv("GA_TEST_FAULT"), *at = getenv("GA_TEST_AT");
    ga_test_fault(fault ? atoi(fault) : 0, at ? atoi(at) : 1);
    int before = descriptors();
    response[0] = ga_call(request[0], (ga_input){input,request[1]}, (ga_input){dest,request[2]},
        (ga_limits){request + 3,6}, (ga_output){out,request[0] ? 0 : request[9]}, (ga_meta){response + 1,3});
    assert(descriptors() == before);
    if (fwrite(response, 1, sizeof response, stdout) != sizeof response) return 2;
    if (response[2] && fwrite(out, 1, (size_t)response[2], stdout) != (size_t)response[2]) return 2;
    free(out); free(dest); free(input);
    return 0;
}
