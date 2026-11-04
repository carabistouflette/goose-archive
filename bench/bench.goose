// SPDX-License-Identifier: Apache-2.0
import archive from "..";
import os;
import std;
fn main() {
    let argv = args();
    assert(argv.len == 4);
    let count, ok = parse_int(argv[2]);
    assert(ok && count >= 1 && count <= 100000);
    var bytes: u8[>..] = [];
    assert(read_file(argv[1], bytes));
    var entries: archive::Entry[>..] = [];
    var limits = archive::Limits { .. };
    if argv[3] == "tight" { limits.entries = 2; limits.tar_bytes = 10240; limits.manifest_bytes = 64; }
    if argv[3] == "max" { limits.tar_bytes = 67108864; limits.manifest_bytes = 4194304; }
    var checksum: i64 = 0;
    let start = clock_ns();
    for _ in count {
        let result = archive::inspect(bytes, limits, entries);
        assert(result.error == archive::Error.None);
        for entry in entries { checksum += entry.path.len + entry.size + 1; }
    }
    print(clock_ns() - start, " ", checksum);
}
