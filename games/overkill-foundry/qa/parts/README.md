# Independent physical-part probes

These are a bounded QA sample written independently of the lifecycle matrix's
author. They include no author test/oracle files. Expectations come from the
selected catalogue rows, timing rules and resale policy. The CMake target
compiles the preserved candidate core, not changing live production.

From the repository root:

```powershell
python -X utf8 games/overkill-foundry/qa/parts/audit.py
python -X utf8 games/overkill-foundry/qa/parts/run.py --run
python -X utf8 games/overkill-foundry/qa/parts/run.py --check
```

`--run` requires the ignored author archive identified by
`core/tests/parts/evidence/results.json`. It builds under `core/build/qa-parts`,
runs these probes and the unchanged captured author suite, then records CTest.
It replaces only this review's evidence; it never calls the author's capture
mode or changes bindings. `--check` verifies recorded sources and artifacts
without executing tests. An optional `--cmake` argument selects another local
CMake executable.

The evidence JSON records all source and binary identities. Raw build logs,
source copies and binaries remain in the ignored build directory. Portable
probe and CTest logs are in `evidence/`.

The fixtures provide abundant resources, synthetic recovering targets and
controlled upgrade acquisition. Payments, copying, part use, target protection,
collection, retention, sale receipts and cleanup execute production APIs. This
is neither natural acquisition nor a gameplay/balance run. See the
[review](../part-lifecycle-review.md) for exact scope and version limitations.
