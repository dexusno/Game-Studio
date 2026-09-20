# Deferred contact effects

Klaus stopped visual refinement on 20 September 2026 to prioritize mechanics.
The current demo does not meet the concept16C/F.I.S.T. quality or perspective.

`deferred-runtime.patch` preserves the unfinished robot impact/death effect work.
It is not compiled, integrated, visually verified or approved. Both live Robot
source files were restored byte-for-byte from the verified2032 source archive
before further mechanics builds. Do not apply the patch as an accepted change.

`contact_probe.py` and `contact-report.json` are read-only CPU measurements of
existing original robot surfaces for possible future cosmetic placement. They
do not create gameplay hitboxes or decide hits, damage or targets. The rules
engine remains authoritative; graphics only display its committed results.

An ignored copy of both unfinished C++ files and their identities is retained in
`unreal/Saved/Validation/deferred-impact-20260920-2100`.
