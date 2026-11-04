# Security

This experimental library has no supported stable releases. Use maintained
system zlib/libarchive builds and review the API restrictions before use.
Report suspected vulnerabilities privately through GitHub's private security
advisory reporting when a public repository is available and the feature is
enabled. Until then contact the repository owner privately through your existing
channel; do not open public issues containing exploit details. No public security
inbox or disclosure service is claimed by this unpublished checkout.

Archives are untrusted bytes, not trusted software. CRC checks detect corruption;
they do not prove authenticity. The caller owns artifact authorization, bounded
I/O, trusted metadata/signature verification, parent directory control, cleanup,
and publication. See docs/api.md for concurrency, resource and partial-output
limits. No TLS, cryptography or executable launch is implemented here.
