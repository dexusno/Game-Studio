# Precision persistence evidence review — 20 September 2026

The bounded read-only review found no defect in the new probe or unsupported
claim in its stated persistence scope. The archived run records **81 passing
checks**, including **three actual Windows sharing conflicts**, against the
production campaign model, session and SaveStore. No new build, render run,
input test or gameplay change was made for this review.

The 95-source graph reconstructs as
`7c37c50009576ea164a5fafa80a04d0f9c5944b421f39920544a0c4642ed9714`.
All captured source sizes/raw/LF hashes, 25 artifacts, the archived DLL and the
portable/archive identity records match. DLL SHA-256 is
`e78ee7dfbe896cedf6e663667967cfc3649906559f4faf82f913e51107441a80`;
the launcher records the same binary before and after, with exit code 0.
Build `220141` succeeds; CampaignProbe `220217` completes normally.

The 20 captured core files independently reconstruct the logged digest
`b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf`,
with `review_snapshot=1`. This is the explicit **1843 historical core**. It
excludes the newer provisional corrections and cannot serve as their engine
verification. The live model/storage files reviewed still match this capture;
the diff adds the probe and one opt-in CampaignProbe call, without a production
Precision-model change.

## What the probe establishes

- Each ordinary, saved Mayor retry and Heavy Magnet failure holds a real read
  handle on that save's `.lock` file. SaveStore requests a zero-sharing write
  lock and fails with Windows error 32 before writing/replacing the data file.
  No injected success/failure stub stands in for the OS conflict.
- The session rereads the still-accessible save envelope. All three failures
  log `reconciled=1`, revision 1, and an unconfirmed transaction. Actual runtime
  assertions compare complete serialized campaign bytes, exact save-file bytes,
  revision and an empty committed-event queue. Separate production reloads see
  the unchanged committed state.
- Stale/invalid callbacks cannot set the result. Once measured, the result and
  original steering remain fixed; repeated/reopened attempts, cancel/back,
  underlying collection/turn commands, Continue, replacement and direct typed
  campaign actions are blocked. Heavy Magnet fixes the category before a
  required discard choice and writes nothing until that choice is submitted.
- The explicit failure exit grants nothing and disables in-memory resume.
  Ordinary Continue restores the original encounter entry. A new ordinary
  Perfect attempt then matches the authoritative state preview and increments
  the save revision once; stale/duplicate callbacks neither rewrite the bytes
  nor republish events. Fresh production reload confirms the saved campaign.

There are 33 distinct assertion labels: six shared setup checks, 24 checks in
each of three modes, and three Heavy-only checks. Their execution accounts for
all 81 log lines. Independent parsing of all six archived save envelopes checks
signature, payload length, commit-token format and the actual FNV integrity
calculation. Every current envelope is revision 3, with revision 2 in its
`.previous` file, consistent with Continue followed by one successful collection.

## Limits retained

These are synthetic callbacks and deliberately prepared encounters/resources.
All post-failure successes are **fresh ordinary attempts after Continue**; this
probe does not commit the retained retry or Heavy discard result in place after
releasing its lock. Its lock conflict precedes replacement, so it does not add
coverage of post-replacement ambiguity, power loss, process exits or disk-full
recovery. Those remain separate storage evidence.

The success assertions compare full state and event count/no-repeat behavior;
they do not independently compare every successful event payload to the preview.
The failed-write assertions do establish that no speculative event is published.
The in-flight before/failed-write save bytes are compared during the run; the six
retained envelopes are the later successful revisions, not copies of revision 1.

The ordinary accompanying campaign probe reaches position 2, 68 HP, 40 receipts,
revision 42 and hash `dd3d15f93fa3b0a4`, with 16 captures. These captures were
identity-checked, not visually judged in this review. No physical timing, focus,
input, sound, visual quality, human feel, full-city or packaged-build claim follows.

[Portable evidence](precision-persistence-evidence.json) records the review's
identities and checks. The unchanged source/run artifact remains at
`unreal/Saved/Validation/precision-persistence-20260920-220140`; its
[checkpoint](../unreal/Tools/precision-tests/persistence-checkpoint.json) and
[run instructions](../unreal/Tools/precision-tests/README.md) remain unchanged.
