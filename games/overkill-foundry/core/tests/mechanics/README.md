# Combined mechanics candidate

The 20 September copied candidate passes **16/16 native suites** against one
authoritative `overkill_core` library. It combines the original eight suites
with physical-part lifecycles, upgrade clauses, Shared and Mara recipes, Jig
copy attribution, encounter/status contracts, campaign lifecycles and terminal
notification regressions. No gameplay values or policy scores are changed by
this integration target.

The candidate includes the pending terminal-event, Modifier category, shop
price predicate, SH092/SH113 schedule, UGS-121 copy, MA082 Burn-cap and MA098
unused-reserve corrections. It is still **version-held**: this execution does
not approve older saves, activate support bindings, update content eligibility
or establish release readiness. Frozen individual author and QA reports keep
their original source identities and finite semantic limitations.

[results.json](results.json) records all 16 actual suite outputs, source and
binary identities. The graph is
`8ce7ff421aa9bc2a6ed4a1be4d77ad5c4a882f08cc6b48b92a49ca4fa3798637`
over 78 copied compilation inputs. The shared library SHA-256 is
`afe752450de55d4837b2b5ebe3e1517eb5a21fb77912fdfd0bb03a0649a05228`.
The immutable source, configure/build logs, CTest XML, complete `LastTest.log`,
executables and runner trace remain under ignored
`core/build/mechanics-integration-20260920-221228`.

| Added suite | Groups | Assertions |
|---|---:|---:|
| Physical output lifecycle | 207 | 59,169 |
| Upgrade clauses | 106 | 6,139 |
| Shared recipe clauses | 113 | 27,881 |
| Mara recipe clauses | 107 | 20,526 |
| Jig copy attribution | 7 | 671 |
| Encounter/status contracts | 9 | 3,946 |
| Campaign lifecycle | 6 | 652 |

The terminal suite separately checks unique terminal notifications and exactly
once campaign completion. Original core, route, robot, recipe, upgrade,
campaign, campaign-upgrade and runner suites also pass. The runner regression
completes and replays its seed-1 city in 440 actions; this is not a replacement
for the historical 400-run balance experiment. Prepared contract fixtures do
not establish naturally earned combinations, human difficulty or game feel.

The first manual capture, `mechanics-integration-20260920-220954`, omitted the
upgrade tests' `presentation/precision.hpp` dependency and failed compilation.
It is preserved as an author capture error; no game source was changed to fix
it. The completed execution's JUnit stdout used CTest's default 1,024-byte
limit. The full output was then extracted from that same run's `LastTest.log`,
without rerunning tests. The original capture tool and later extractor are
separately retained and hashed. The current tool performs this extraction
directly.

Run from the repository root on the configured Windows/MSVC workstation:

```powershell
python -X utf8 games/overkill-foundry/core/tests/mechanics/capture.py --cmake "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
```

Each run creates a new archive and verifies copied and live input hashes after
execution. It never edits active support, rules versions or historical reports.
