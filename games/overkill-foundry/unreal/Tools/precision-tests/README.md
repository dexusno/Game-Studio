# Precision controls and persistence checks

The portable `tests.cpp` checks timer boundaries, focus pausing and inclusive
Good/Perfect windows. It does not exercise Slate or disk persistence.

`FoundryPrecisionProbe.cpp` adds a separate opt-in preflight to CampaignProbe.
It runs the production `FFoundryCampaign` model, campaign/session rules and
Windows SaveStore. The 20 September run passes **81 checks**, including three
real Windows sharing conflicts that block the writer's exclusive save lock:

- Ordinary Precision after a synthetic Perfect callback.
- A retry produced by an actual Miss with Mayor MY1-17.
- Heavy Magnet collection after activation and End Turn, with a measured result
  held until its discard choice is submitted.

For each failed write, the exact save bytes, serialized campaign, revision and
committed event queue stay unchanged. Stale/invalid/repeated callbacks cannot
grant or reroll a result, and the modal blocks underlying collection, turns,
navigation, Continue and New Game. Returning to title grants nothing. Ordinary
Continue restores the original encounter entry; after that restart and release
of the file lock, a new attempt saves exactly once, matches the shared-core
preview, and survives a fresh production disk reload.

This is model-level integration using synthetic callbacks. It adds no claim
about physical timing, input focus, audio, visual layout or human enjoyment.
The prepared Heavy Magnet/protection grant is not earned progression. The
lock conflict happens before replacement; ambiguous post-replacement failures
and process exits retain their separate storage/session QA evidence.

The actual Editor build `220141` and CampaignProbe `220217` use the explicit
reviewed1843 core, digest `b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf`.
The live version-held rules corrections are excluded. The same run passes the
ordinary campaign control path at position 2, 68 HP, 40 receipts, revision 42,
hash `dd3d15f93fa3b0a4`, with 16 captures and a normal process exit.

[persistence-checkpoint.json](persistence-checkpoint.json) identifies the 95
unchanged source inputs, Editor DLL, logs, six save envelopes and 16 captures.
The ignored immutable archive is
`unreal/Saved/Validation/precision-persistence-20260920-220140`.
The test adds no gameplay, UI layout or save-version change. Independent review
of this new test and its evidence is pending.

Run from the repository root, using a fresh isolated probe save:

```powershell
& games/overkill-foundry/tools/unreal.ps1 -Action Build -CoreRoot games/overkill-foundry/unreal/Saved/Validation/roster-v001-runtime-20260920-1843/source/core
& games/overkill-foundry/tools/unreal.ps1 -Action CampaignProbe -Width 1280 -Height 720
```
