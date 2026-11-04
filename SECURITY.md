# Security

This experimental library has no supported stable releases. Use maintained
system zlib/libarchive builds and review the API restrictions before use.
Report suspected vulnerabilities privately through
[GitHub private vulnerability reporting](https://github.com/carabistouflette/goose-archive/security/advisories/new).
Private vulnerability reporting is enabled on this repository. Do not open public
issues containing exploit details before coordinated disclosure.

Archives are untrusted bytes, not trusted software. CRC checks detect corruption;
they do not prove authenticity. The caller owns artifact authorization, bounded
I/O, trusted metadata/signature verification, parent directory control, cleanup,
and publication. See docs/api.md for concurrency, resource and partial-output
limits. No TLS, cryptography or executable launch is implemented here.
