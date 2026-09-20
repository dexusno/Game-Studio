# Windows save envelope

This IO layer is outside the deterministic rules library. It stores one opaque payload; the future campaign caller must serialize profile facts, current run, fixed offers and exactly-once operation receipts together. It is not a campaign/Continue implementation.

`SaveStore::read()` returns a checked payload and commit token. `commit(payload, expectedToken)` compares that token while holding an exclusive OS file handle. Use an empty token only to create a new save. Revisions aid diagnostics; they are not identities, since recovery can reuse a number. `OFSAVE02` envelopes carry a random 128-bit identity generated with Windows BCrypt, independently of all gameplay RNG. The integrity hash detects damage; it is not an authentication mechanism.

A commit writes and flushes a same-directory temporary file, replaces the primary while preserving the previous valid envelope, flushes the committed file, then verifies it. Recovery never promotes a damaged primary into the backup. Unknown envelope versions are rejected rather than rolled back. Old development `OFSAVE01` data is incompatible; no delivered player saves exist yet.

Only `ok=true` permits a success acknowledgement. A failed call can occur after replacement, so `reconcileRequired=true` means the caller must reload and inspect its operation receipt before retrying. A failed commit must not be interpreted as proof that nothing happened. The payload must carry stable operation IDs for purchases/rewards; the random storage token does not provide exactly-once game semantics by itself.

Run `tools/test_storage.ps1` and `qa/storage/run_probes.ps1` with paths from [BUILD.md](../BUILD.md). Tests use real process exits at selected boundaries on local NTFS. They do not emulate sudden hardware power loss, every disk/controller, cloud sync or a campaign integration.

API contracts checked against Microsoft's documentation: [ReplaceFileW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew), [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers), [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw) and [BCryptGenRandom](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom). The unsupported `REPLACEFILE_WRITE_THROUGH` flag is not used; flushing is explicit.
