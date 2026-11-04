// SPDX-License-Identifier: Apache-2.0
namespace archive;
import std;

enum Error { None, InvalidInput, LimitExceeded, Malformed, UnsafeEntry, SystemError }
enum Kind { File, Directory }
struct Entry { kind: Kind = Kind.File, size: i64 = 0, path: u8[] }
struct Status { error: Error = Error.None, os_error: i64 = 0 }
struct Limits {
    tar_bytes:      i64 = 16777216,
    entries:        i64 = 4096,
    file_bytes:     i64 = 16777216,
    total_bytes:    i64 = 16777216,
    depth:          i64 = 32,
    manifest_bytes: i64 = 1048576,
}
extern fn native_call(op: i64, input: const u8[:], destination: const u8[:],
    limits: const i64[:], manifest: u8[:], meta: i64[:]) -> i64 = "ga_call";

fn valid(limits: const Limits&) -> bool {
    return limits.tar_bytes >= 0 && limits.tar_bytes <= 67108864 &&
        limits.entries >= 0 && limits.entries <= 4096 &&
        limits.file_bytes >= 0 && limits.file_bytes <= 67108864 &&
        limits.total_bytes >= 0 && limits.total_bytes <= 67108864 &&
        limits.depth >= 0 && limits.depth <= 32 &&
        limits.manifest_bytes >= 0 && limits.manifest_bytes <= 4194304;
}
fn status(code: i64, os_error: i64) -> Status {
    let error = if code == 0 { Error.None } else if code == 1 { Error.InvalidInput }
        else if code == 2 { Error.LimitExceeded } else if code == 3 { Error.Malformed }
        else if code == 4 { Error.UnsafeEntry } else { Error.SystemError };
    return Status { error: error, os_error: os_error };
}
fn owned_path(bytes: const u8[:]) -> u8[] {
    var out: u8[>..] = [];
    out.append(bytes);
    return out;
}
// ISIZE is only a preparation hint. Native decoding validates it and independently
// bounds actual output before writing. A tar entry needs at least one 512-byte
// header and its manifest record has at most 1024 path bytes + 10 bytes.
fn manifest_capacity(input: const u8[:], limits: const Limits&) -> i64 {
    if input.len < 4 { return 0; }
    var tar_hint: i64 = 0;
    for b in 4 { tar_hint |= (input[input.len - 4 + b] as i64) << (b * 8); }
    return min(limits.manifest_bytes, min(limits.entries, tar_hint / 512) * 1034);
}
fn inspect(input: const u8[:], limits: const Limits&, out: Entry[>..]&) -> Status {
    out.clear();
    if !valid(limits) || input.len < 1 || input.len > 16777216 { return Status { error: Error.InvalidInput }; }
    let options = [limits.tar_bytes, limits.entries, limits.file_bytes, limits.total_bytes, limits.depth, limits.manifest_bytes];
    var packed: u8[>..] = [];
    packed.resize(manifest_capacity(input, limits), 0);
    var meta: i64[3] = [0, 0, 0];
    let code = native_call(0, input, "", options, packed[..], meta[..]);
    if code != 0 { return status(code, meta[0]); }
    var pos: i64 = 0;
    for _ in meta[2] {
        let kind = if packed[pos] == 1 { Kind.Directory } else { Kind.File };
        pos++;
        var size: i64 = 0;
        for b in 8 { size |= (packed[pos] as i64) << (b * 8); pos++; }
        let start = pos;
        while pos < meta[1] && packed[pos] != 0 { pos++; }
        out.push(Entry { kind: kind, size: size, path: owned_path(packed[start..pos]) });
        pos++;
    }
    return Status { .. };
}
fn extract(input: const u8[:], destination: const u8[:], limits: const Limits&) -> Status {
    if !valid(limits) || input.len < 1 || input.len > 16777216 { return Status { error: Error.InvalidInput }; }
    let options = [limits.tar_bytes, limits.entries, limits.file_bytes, limits.total_bytes, limits.depth, limits.manifest_bytes];
    var unused: u8[0]  = [];
    var meta:   i64[3] = [0, 0, 0];
    let code = native_call(1, input, destination, options, unused[..], meta[..]);
    return status(code, meta[0]);
}
