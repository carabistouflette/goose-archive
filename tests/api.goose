// SPDX-License-Identifier: Apache-2.0
import archive from "..";
import os;
import std;

fn rejected(fixture: const u8[:], destination: const u8[:], expected: archive::Error) {
    var bytes:   u8[>..]             = [];
    var entries: archive::Entry[>..] = [];
    let limits = archive::Limits { .. };
    assert(read_file(".build/fixtures/small.tar.gz", bytes));
    var result = archive::inspect(bytes, limits, entries);
    assert(result.error == archive::Error.None && entries.len == 2);
    bytes.clear();
    assert(read_file(str(".build/fixtures/", fixture, ".tar.gz"), bytes));
    result = archive::inspect(bytes, limits, entries);
    assert(result.error == expected && entries.len == 0);
    result = archive::extract(bytes, destination, limits);
    assert(result.error == expected);
}
fn main() {
    let argv = args();
    assert(argv.len == 2);
    var bytes: u8[>..] = [];
    assert(read_file(".build/fixtures/small.tar.gz", bytes));
    var entries: archive::Entry[>..] = [];
    let limits = archive::Limits { .. };
    var result = archive::inspect(bytes, limits, entries);
    assert(result.error == archive::Error.None && entries.len == 2);
    for entry, i in entries {
        if i == 0 { assert(entry.kind == archive::Kind.Directory && entry.path == "src"); }
        else { assert(entry.size == 11 && entry.path == "src/main.goose"); }
    }
    // Inspection paths own storage independent of the compressed source.
    for i in bytes.len { bytes[i] = 0xaa; }
    for entry, i in entries {
        if i == 0 { assert(entry.path == "src"); }
        else { assert(entry.path == "src/main.goose"); }
    }
    bytes.clear();
    result = archive::inspect(bytes, limits, entries);
    assert(result.error == archive::Error.InvalidInput && entries.len == 0);
    assert(read_file(".build/fixtures/small.tar.gz", bytes));
    let tight = archive::Limits { entries: 1 };
    result = archive::inspect(bytes, tight, entries);
    assert(result.error == archive::Error.LimitExceeded && entries.len == 0);
    result = archive::extract(bytes, argv[1], limits);
    assert(result.error == archive::Error.None);
    result = archive::extract(bytes, argv[1], limits);
    assert(result.error == archive::Error.SystemError && result.os_error != 0);
    var output: u8[>..] = [];
    assert(read_file(str(argv[1], "/src/main.goose"), output));
    assert(output == "hello\0world");
    bytes.clear();
    assert(read_file(".build/fixtures/unsafe.tar.gz", bytes));
    result = archive::inspect(bytes, limits, entries);
    assert(result.error == archive::Error.UnsafeEntry && entries.len == 0);
    rejected("conflict", str(argv[1], "-conflict"), archive::Error.UnsafeEntry);
    rejected("no-eof", str(argv[1], "-no-eof"), archive::Error.Malformed);
    rejected("bad-isize", str(argv[1], "-bad-isize"), archive::Error.Malformed);
    let invalid = archive::Limits { depth: -1 };
    result = archive::inspect(bytes, invalid, entries);
    assert(result.error == archive::Error.InvalidInput);
}
