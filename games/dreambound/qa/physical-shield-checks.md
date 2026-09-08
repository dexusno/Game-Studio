# Physical shield courtyard QA

2026-09-08. **Windows Shipping staged run: 14 scenarios passed, 0 failed.** Integration executed the package; independent QA inspected its machine-readable result. This covers the replacement physical-shield courtyard. The rejected beta2 pulse weapon and its 31-check report do not certify this slice. No ordinary play or enjoyment claim follows from these passes.

Final integration confirmation: `Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe` in `BuildOutput/ShieldStudy/Windows`, 166,155,776 bytes, SHA-256 `3D6B826D204A05BC3FA55FC3E945C8165817BE8A8EADE31D8996C0775CC516A0`. The final copied package ran from the saved project with Unreal5.8.2, NullRHI, seed833283 and isolated slotDreamboundQA_PhysicalShield. Result: complete/non-aborted,14/0. See [exact final identity](../evidence/shield-study-build.json) and [retained result](../evidence/shield-study-checks.json). Four executions of the14 scenarios occurred as integration/capture/control-label corrections landed; no scenario execution failed. Staged runtime duration is not gameplay length.

The new runner is an independent staged check in the actual Unreal world. It uses ordinary actor ticks, public combat/input methods, actual shield and projectile sweeps, and the real ward/reward/save paths. It never recursively ticks the world. Fourteen scenario results group the relevant observations:

| Scenario coverage | Current result |
| --- | --- |
| Isolated slot setup and exact prior QA-journal restoration | Passed |
| Held guard, rim contact, committed attack/charge exposure and recovery | Passed; rim contact reduced real target health from 2,000 to 1,952 |
| Launch, unavailable guard, pause preserving deployment, outward/return contacts and physical catch | Passed; target took both travel passes; return caught in 0.570 simulation seconds |
| Recorded swept movement behind cover and recall around the obstruction | Passed; direct route was blocked; physical catch in 0.701 simulation seconds, without timeout recovery |
| Real Anchor projectile crossing versus flank and reverse crossing | Passed; frontal crossing spent integrity/banked force; flank and back shots damaged the player |
| Two earned attachments through ward interaction, actual spawned-enemy defeat and reward choices | Passed; Anchor then Frost installed; repeated interaction/choice did not duplicate grants |
| Fresh checkpoint reconstruction with Anchor/Frost, deployed-shield cleanup and CRC fallback | Passed; checksum mutation selected previous revision 12 from a 2,590-byte journal |
| Final courtyard result, same-seed retry, death and retained patterns | Passed |

Reproduce through the integration build with `-DBVerify -DBSaveSlot=DreamboundQA_PhysicalShield -DBSeed=833283 -NullRHI -unattended -nosound`. Editor execution also requires `-game`. Read `Saved/QA/physical-shield-checks.json` from the running game's Saved directory; require `complete: true`, `aborted: false`, and `failed: 0`. Process exit alone is insufficient. The runner stops within 55 seconds after entry on a responsive ticking engine and restores only its explicitly named QA journals. No owner save slot is accepted.

Independent QA inspected the first packaged result; integration ran and retained the final artifact result. Neither represents normal player input.

Fixtures are artificial: a separate floor/wall and stationary high-health target sit 100 metres above the courtyard; player positions and the cover route are staged; Anchor rank two is granted for its interception test. Actual courtyard phases are started through `Interact`, but lethal damage is staged to test progression without claiming combat success. Journal corruption flips a checksum byte while preserving the serialized payload. These checks use game input APIs, not Windows keyboard/mouse input.

Source review found a pending victory delay could survive an immediate new run. Integration reset it in both `StartNewRun` and `ResumeRun`. Staged ordinary victory/retry passed; interruption during the short delay was not directly triggered. No unresolved executed failure was found within the tested scope. QA owns this report and `DBShieldChecks.h/.cpp`; implementation fixes remain with their authors.

Ordinary launch/held controls, focus capture, moving rendered shield/scene readability, listening, performance, a whole unstaged encounter journey and enjoyment remain untested by this runner. Owner feedback is required to establish whether the replacement interaction is enjoyable or meets the intended presentation.
