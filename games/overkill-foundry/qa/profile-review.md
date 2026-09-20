# Independent profile and Collection mechanics review

2026-09-20 — **finite pass after the narrow PF-01 repair**. No remaining confirmed
isolation, save-corruption or retained-discovery defect in this reviewed scope.
The repair is verified in a CPU helper; the earlier Unreal binaries remain
historical evidence and do not contain it. Production files were not edited by QA.

Acceptance was the profile increment's README contract and the profile/defeat/
replacement provisions of `TIMING-AND-PERSISTENCE.md` (especially lines 82–86,
116 and 122–128), within P11. This is not complete P11, shipping or visual approval.

| Reviewed artifact | Exact identity and scope |
| --- | --- |
| Original 2016 compilation/native archive | 84-source LF graph `5cac35f953ba7ce556720cfc67924398f3b48f23884b38bae3199430dd0c86b6`; DLL `828d39bf1020400106ae2ffcd4b6c3ab2ad03b27e752216cc35d926115e12d4d`; Game EXE `86a1c861ec1f1d7f46635f80e1af0f9fca64dd7756dd9bbef9c945772a4c15b9`. Uses the reviewed 1843 **pre-terminal-fix** core, not later live core. |
| 2032 Collection reset recheck | 89-source LF graph `1282abcaabbce655d2e10518c8f4d1b3e072dab8ccf4e67b91bc35580965d166`; DLL `9d2d671e961a206f0c4ca2144e500d220fd026178b9881d8de9453ee217d5a9d`; Game EXE `9fb422f304443ac17bdebebc73d0fe1f0e5740a6759cd6cb79805993efe9b61f`. Only the declared reset/wording recheck is assessed here. |
| Independent repaired-source execution | Archived 2016 core/header plus `FoundryProfiles.cpp` SHA-256 `871b3c6016c5cc7f7d0e9fa8be45475c3249e72d10e7f053f80106e02d9aaf58`; QA EXE `7ab142fb1ce42921e17b9dc6c9d62a08031bea587f4e6e3c0099f5d673c43815`. Windows x64, MSVC 19.44.35228, SDK 10.0.26100, Release C++20, `/W4 /WX /permissive-`. |

**PF-01 — low severity, fixed in the independently tested source.** Original
`FoundryProfiles.cpp:16–33` rejects ASCII controls but accepts decoded C1 controls.
With a new 32-character lowercase-hex ID, call `Store::create` using the UTF-8
name `Alpha \xC2\x80 Beta`. Expected: rejection without allocating its directory,
as required by `unreal/Tools/profile-tests/README.md:18`. Observed: success and
persisted display metadata. Names are not paths; no campaign corruption was
observed. The original QA binary/source/result remain preserved (four passing
groups, one failed group, 61 attempted assertions). The author's one-condition
repair now rejects U+0080, U+0085 and U+009F before allocation; adjacent U+00A0 is
accepted and restored exactly without starting a campaign. Recheck: **five
groups, 69 assertions pass**. No new Unreal execution is attributed to that fix.

The independent [probe](profiles/probes.cpp) also passed real NTFS junction
rejection without changing either campaign; successful profile selection under
exclusive name/preference file locks with a fallback label and explicit notice;
unsupported name-metadata version and previous-only recovery without rewriting
campaign bytes; Alpha/Beta run and discovery isolation; and exact reproduction
of the archived defeat by applying **one production End Turn** to the original
prepared 1-HP input. The replacement run has 80 HP/100 Credits, a new run ID and
the same 16 discoveries. The prepared 246 discoveries coexist with only the
ordinary 12-copy memory. These are controlled CPU checks, not earned progression.

Source review of the captured `FoundryProfiles.h/.cpp`, `FoundryCampaign.cpp`
(15–66, 111–138, 175–195), `FoundryHost.cpp` (221–226) and Collection UI confirmed:
Default retains `Saved/Campaign/profile.ofsave`; generated IDs, not display names,
choose profile directories; creation/selection do not call New Game; a bad
candidate loads before replacing the current selection; explicit save/probe/
inspection paths bypass normal selection; replacement cancellation changes only
the view; Collection iterates the active profile's seen IDs and submits no action.
Switching profiles clears the previous combat and Collection view state.

The read-only [artifact audit](profiles/audit_evidence.py) passes 1,026 identity
checks: all 84 original sources in both copies, 73 original artifacts, the 89-source
recheck parent and 34 parent artifacts, and all 10 recheck artifacts. Exact save
and source hashes, 20 inspected frame names, helper identities and results are in
[profile-evidence.json](profile-evidence.json). The original pre-EndTurn input was
not retained under that name in the final native archive; QA found the original
fixture with the already-recorded hash and preserved a supplemental copy without
changing the archive.

Independent frame inspection covered 15 of the 24 native JPGs and all five final
recheck JPGs: empty Alpha/Beta, replacement confirmation, distinct Mayor state,
restored Alpha and seen-but-unpurchased facts, fixed-path title, 1-HP fight, enemy
presentation, Game Over, retained Collection and fresh run. The original list
offset defect remains visible in frame 17. Recheck frames 02/03 show Ash Crown
and Blood Fuel at the top after filter/search; archived source applies reset
flags after preserving old offsets. The browsing envelope is byte-identical.
Frames 04/05 show the actual selected/loaded singular wording and six projected
HP damage. This closes that specific recorded reset defect, not general UI design.

The author-operated native mouse/keyboard session was at 1280×720. Its clicks,
relaunch and exit are attributed to its trace/logs, not independently performed
here. The verified CampaignProbe log reports 17 engine profile checks, 30 recipe
controls and position 2 / 68 HP / 40 receipts / revision 42 / hash
`c6eca939d9360880`, with `physical_input=0`. The two archived author helper logs
report 86 assertions each; these were inspected, not rerun. The early failed GUID
creation images belong to an older module and are not final-build success evidence.

Not run: Unreal/desktop input, controller, audio, fresh process-crash/power-loss
injection, full-city profile progression, shipping package, migration acceptance,
human usability or visual/feel approval. The earlier map asset was not separately
archived. Later operator art and live core changes are outside this review.
