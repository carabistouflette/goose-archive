// SPDX-License-Identifier: Apache-2.0
import archive from "..";
import os;
import std;
fn main() {
    let argv = args();
    if argv.len != 2 { write_stderr("usage: inspect archive.tar.gz\n"); exit(1); }
    var bytes: u8[>..] = [];
    // This example reads a trusted local file; applications should bound I/O too.
    if !read_file(argv[1], bytes) { write_stderr("cannot read archive\n"); exit(1); }
    var entries: archive::Entry[>..] = [];
    let limits = archive::Limits { .. };
    let result = archive::inspect(bytes, limits, entries);
    if result.error != archive::Error.None { print("archive error: ", result.error); exit(1); }
    for entry in entries { print(entry.path, " ", entry.size); }
}
