# Profiles, Collection and Game Over

Normal launches retain the original `Saved/Campaign/profile.ofsave` as **Default**.
Additional profiles use generated identities beneath `Saved/Campaign/Profiles`.
The selected profile owns one campaign and its discovery/unlock facts. Selecting
or creating a profile does not create a run. New Game retains the existing
campaign replacement confirmation and the core's persistent discovery facts.

`FoundryProfiles` routes independent paths to the existing `CampaignSession` and
`SaveStore`. Campaign envelopes, checksums, CAS tokens, flushing, previous-save
recovery, content guards and transaction receipts remain unchanged. Display
names and the last selected identity use separate SaveStore metadata envelopes.
An unreadable name does not hide its campaign. A corrupt campaign cannot replace
the current valid selection. An unreadable selection preference leaves the
profiles browsable and reports that it could not be remembered.

There is no fixed profile-count cap. Profile names are display text, never path
components. New names must be short, valid UTF-8 without control characters;
existing profile directories cannot be overwritten by creation. Directory
aliases are rejected. Renaming and deleting profiles are outside this increment.

Independent QA subsequently found that encoded C1 controls U+0080–009F passed
the original validator. The narrow decoded-codepoint repair is separate from
the frozen native captures: `FoundryProfiles.cpp` SHA-256
`871b3c6016c5cc7f7d0e9fa8be45475c3249e72d10e7f053f80106e02d9aaf58`.
The strict C++17 and C++20 helper builds each pass 92 assertions, including both
C1 endpoints rejected before directory creation and adjacent U+00A0 accepted.
Independent QA passed 5 groups / 69 assertions against the isolated repaired
source, including U+0085 and exact U+00A0 recovery. Root's subsequent Editor
`212056` / Game `212212` builds include the repair; old artifacts are unchanged.

Explicit `-FoundrySave`, `-FoundryCampaignProbe` and `-FoundryInspectSave` launches
retain their exact fixed path and do not read or modify the normal profile
selection preference. The compatible Campaign constructor defaults to fixed
path behavior for existing probes. Only the normal Host call enables profiles.

Collection lists only the active saved profile's discovered recipe IDs. Search,
kind filters and printed source facts are inspection only: they do not grant
recipes, alter active memory, reveal the unseen pool or write a campaign action.
The separate Game Over uses an original static Slate terminal portrait and short
hostile gloat. It names no new antagonist or lore. Mara remains the only playable
mercenary; later mercenaries and Lockdown remain unavailable.

## Reproduce focused storage checks

Use the installed CMake executable (VS 2022's bundled CMake is verified here):

```powershell
cmake -S games/overkill-foundry/unreal/Tools/profile-tests -B games/overkill-foundry/unreal/Saved/ProfileTests -A x64
cmake --build games/overkill-foundry/unreal/Saved/ProfileTests --config Release --parallel 4
ctest --test-dir games/overkill-foundry/unreal/Saved/ProfileTests -C Release --output-on-failure
```

Set `-DFOUNDRY_TEST_CORE_ROOT=<exact reviewed core directory>` for an explicitly
identified snapshot, and `-DFOUNDRY_TEST_CXX_STANDARD=20` to cover Unreal's char8_t
filesystem strings as well as the C++17 standalone target. Artifacts stay under
the ignored build directory. The test never reads a player's profile.

Current helper evidence: both C++17 and C++20 strict MSVC `/W4 /WX /permissive-`
builds pass 86 assertions using the reviewed pre-terminal-fix core from the
1843 roster archive. This covers 34 visible profiles, path/name rejection,
actual shop discoveries, isolated progress, retained facts on replacement,
fixed-path bypass, stale-writer reconciliation, campaign/name/selection damage,
and injected failures at all four metadata save boundaries. It does not claim
new process-crash or power-loss tests; the established storage backend is used
without modification.

The `foundry_profile_fixtures NEW_OUTPUT_DIRECTORY` executable creates separate,
explicitly prepared inspection saves: actual shop discoveries, all 246 discovered
recipes with ordinary active memory, a 1-HP fight before an enemy turn, and the
Game Over reached by applying that actual End Turn. The abundant Collection and
low HP are QA preparation, not earned play or balance evidence.

The first ordinary native creation attempt found an adapter defect: Unreal's
`FGuid::ToString(Digits)` returns uppercase letters, while directory identities
are canonical lowercase. The request failed safely before writing a profile.
The Campaign adapter now normalizes the generated ID. `FoundryProfileProbe`,
invoked by CampaignProbe, exercises that actual Unreal creation call as well as
selection, empty-state clearing, exact cancellation and fixed-path bypass. This
distinguishes engine adapter coverage from the standalone storage tests.

## Frozen native checkpoint, 20 September 2026

The `Build-20260920-201349.log` Editor and `BuildGame-20260920-201359.log` Game
targets both pass with Unreal 5.8.2 CL 56702186, MSVC 14.44.35228 and Windows SDK
10.0.22621.0. They deliberately compile the reviewed core at
`Saved/Validation/roster-v001-runtime-20260920-1843/source/core`, whose logged
digest is `b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf`.
This is the pre-terminal-notification-fix `of-core-0.4` source, not later live
core work. Reproduction commands from the repository root:

```powershell
$reviewCore = 'games/overkill-foundry/unreal/Saved/Validation/roster-v001-runtime-20260920-1843/source/core'
& games/overkill-foundry/tools/unreal.ps1 -Action Build -CoreRoot $reviewCore
& games/overkill-foundry/tools/unreal.ps1 -Action BuildGame -CoreRoot $reviewCore
& games/overkill-foundry/tools/unreal.ps1 -Action CampaignProbe
& games/overkill-foundry/tools/unreal.ps1 -Action Run -Width 1280 -Height 720
```

`CampaignProbe-20260920-201424.log` passes 17 engine profile checks, the existing
30 recipe-control checks, and the saved encounter/reward loop: position 2,
68 HP, 40 receipts, revision 42, hash `c6eca939d9360880`. Its 16 rendered captures
use automated commands, not native or human input. The actual profile API probe
uses separate generated SaveStore paths and does not touch ordinary profiles.

The native check used agent-driven Windows mouse/keyboard input at 1280×720.
The primary module stayed
`828d39bf1020400106ae2ffcd4b6c3ab2ad03b27e752216cc35d926115e12d4d`;
the Game executable stayed
`86a1c861ec1f1d7f46635f80e1af0f9fca64dd7756dd9bbef9c945772a4c15b9`.
The immutable 84-input source graph is
`5cac35f953ba7ce556720cfc67924398f3b48f23884b38bae3199430dd0c86b6`.

- Created two normal-launch QA profiles. Creation left them empty; Alpha's real
  shop inspection produced 16 discoveries, while Beta remained independent and
  later retained its own pending Mayor choice. A long Beta name wrapped in the
  chooser and stayed contained in the ordinary header.
- Browsed an unpurchased shop recipe, opened replacement confirmation and chose
  **Keep current campaign**, switched profiles, quit, relaunched and continued
  Alpha's same route. Its save stayed byte-identical throughout, SHA-256
  `2ebced59a9217595685657a32c74fe93aef59a8809b8873c21147a1a20a810d7`.
- Explicit fixed-path mode hid normal profile switching. Its prepared 246-entry
  Collection scrolled to the last entry; search returned Fuel Brick with printed
  source facts. Browsing left the save byte-identical, SHA-256
  `4df3f873db767675ccb6c703feec7d8a7bb20333d39db27af563b41b2283a208`.
- From the prepared 1-HP Mite/Ram fight, one native End Turn saved the actual
  defeat. Enemy presentation preceded the original AI Game Over. Collection
  retained 16 discoveries; New Game then opened a fresh Mayor choice at 80 HP
  and 100 credits while the title still showed those 16 discoveries.

The terminal portrait is original code-native Slate geometry in
`FoundryCampaignUI.cpp`, authored for this game without external images, fonts
or sampled character artwork. Its short gloat assigns no new antagonist lore.

Portable exact identities are in [native-checkpoint.json](native-checkpoint.json).
The ignored archive `Saved/Validation/profiles-native-archive-20260920-2034`
contains copied sources/binaries, logs, the 24 native frames, a detailed native
trace, before/after save copies, tests and campaign captures. The compile inputs
come from the verified `profiles-frozen-20260920-2016` snapshot, not the later
operator edits. The five early creation/framing images are explicitly attributed
to the older `4545e434…` module. The earlier map asset was not separately copied
before the next asset import; later maps are intentionally excluded from this
native identity rather than presented as its original scene.

Native filtering exposed one presentation defect: after scrolling to the bottom,
switching kind kept the old list offset while selecting the first match. Frame
17 preserves it. The follow-up source queues list/detail resets until after
Rebuild preserves the old widgets' offsets. The frozen earlier binary is not
claimed to contain that repair.

The later combined Editor `203232` / Game `203314` build contains the reset.
Its finite native recheck passed: All kinds at the bottom → Utility selected Ash
Crown at the top; Utility at the bottom → Heat search selected Blood Fuel, first
of 27 results. The browsing envelope stayed byte-identical. In the same process,
actual route entry, ordinary Collect, Solid Casting, part selection and Lock and
load displayed **1 bullet part selected**, **1 part**, and **6 HP damage, 0
absorbed by Shield** in the settled shooting view. No extra HP or resources were
granted. This inspection uses the prepared discovered-reference list only.

The user initially stopped desktop control with Escape, then explicitly resumed
it. The existing process was rediscovered; no duplicate was launched. It later
closed normally. [recheck-checkpoint.json](recheck-checkpoint.json) identifies all
five frames, the log, before/after save evidence, and unchanged `9d2d671e…` DLL /
`9fb422f3…` EXE. It references the separate parent operator source/archive graph
`1282abcaabbce655d2e10518c8f4d1b3e072dab8ccf4e67b91bc35580965d166`.

No controller, shipping package, new process-crash/power-loss, full twelve-position
traversal or independent human-usability claim is made. The complete Collection
and low-HP fight are explicit prepared fixtures, not earned progression or
balance evidence. These checks do not establish final visual acceptance. The
normal-launch QA Alpha/Beta profiles remain in ignored Saved data; no existing
player campaign was replaced.
