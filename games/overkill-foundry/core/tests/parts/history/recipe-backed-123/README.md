# Executed lifecycle candidate

The strict MSVC native build passed **123 physical-type groups, 38,293
assertions, zero failures**. The manifest has 207 output obligations; **84 remain
unproved by this slice**. This is author-operated evidence awaiting independent
linkage review. The active runtime-support ledger was not edited.

The exact proposed type-to-fixture pairs are in
[`proposed-bindings.json`](proposed-bindings.json). The 84 residual IDs and reasons
are there and in [`../contracts.json`](../contracts.json): 29 generated plain
value types, 34 Modifier types (including six Spread implementations), and 21
Magnet types. The matrix does not prove every conditional recipe effect, the
remaining upgrade clauses, held collection composition, balance or usability.

| Identity | SHA256 / value |
| --- | --- |
| Build | `native-part-lifecycle-105b12edcbf90bb1` |
| Test executable | `105b12edcbf90bb1020d068c3ca13a2b2e541212f34c07e98a2617ee1e9918e0` |
| Shared library | `2660aa5fc62316143b7aff16f31785b760f8923b54b149adf04e174c7ffdd0b0` |
| Complete 31-input graph | `18465c3d6e65b36705f24aeae89992c677108c8a44816c03ce506dfe09ca7d86` |
| Manifest | `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9` |
| Results JSON | `1ad73f4c8c2c9d4461eb3e16124880a95d23ee201e212fda3d65f25b18609446` |

[`results.json`](results.json) records all input hashes, actual configure/build
commands, successful process exits, library/executable identities, and the exact
123 named results read from [`parts.log`](parts.log). The capture compared the
input graph before and after compilation/execution. `capture.py --check` passes
against this graph and the retained native artifacts.

This candidate includes the terminal-notification correction at `core.cpp`
SHA256 `86198fda6e4f4c8d5dfd5a32a203242ba789ace9a91ecfba77ce702dc5e7895d`.
It still uses `of-core-0.4` while the integration owner awaits the save/version
decision. Any version-header or production change requires another capture;
these results must not be relabelled as that later build. The earlier 133-binding
runtime-linkage evidence remains unchanged and describes its historical build.

Independent QA can rebuild from `core/tests/parts/CMakeLists.txt`, inspect the
literal reference table against the cited catalogue, run the 123 groups, then
sample type-to-fixture sufficiency in the proposal. See the parent README for
the controlled fixtures, assertions, commands and exclusions. No public release
eligibility follows from the proposed bindings.
