# Recipe action controls

`FoundryRecipeControls` stores reversible UI selections and builds the shared
core `Action`. The core remains responsible for eligibility, costs, random
collection, damage, copying and sacrifices. A displayed choice is not a promise
that it is currently affordable: the complete request is previewed before Use,
Install, Lock and load, Fire or End Turn becomes available.

The campaign's keyboard and button paths use the same `FoundrySession` builders.
They still pass through `FoundryStage::Control` for the reviewed camera flow.
Reordering the selected bullet preserves the association between each physical
Full-Spread Head and its reserved Shield payment.

| Request field | Exposed recipes / context |
| --- | --- |
| Craft target | SH015, SH049, MA026, MA029, MA049, MA050, MA074, MA087, MA095, MA100, MA105, MA117 |
| Craft second target | MA105: Burn destination |
| Craft reserve parts | SH032, SH065, SH071, SH076, SH101, SH102, SH118, MA030, MA066, MA089 |
| Craft material choices | SH069, SH080, SH101, SH106, SH108, SH119, SH121, SH122; SH101 choices follow the selected sacrifice order |
| Craft output index | Active SH103 Fine Mould, when producing multiple Ammo outputs |
| Craft discount allocation | MY1-09 Rotating Toolhead: five material fields; qualification and payable totals come from the core |
| Install sacrifice | MA022: one unused Ammo |
| Install target | MA086: attacking enemy |
| Install amount | MA083: no Heat or all current Heat; MA096: 3–8 Heat |
| Modifier amount | SH085: 0–15 installed Shield; MA045: 2–5 Heat; MA061: 4–12 installed Shield |
| Modifier parts / target | SH036: attached Ammo; MA052: 1–3 consumed Ammo; MA053: enemy when its conditional effect requires one |
| Load sacrifices | SH110: one unused Shield per physical Full-Spread Head |
| Fire support targets | SH017, SH042, SH087, MA012, MA077, MA091: per-physical-part additional enemy |
| Fire Spread targets | SH005, SH051, MA041: per-physical-part targets; SH051 allows up to two in chosen order. SH082, SH110 and MA111 are automatic all-target effects. |
| Fire copy choice | Active SH072 Return Delivery: one fired Base/Common Ammo |
| End Turn choice | Each newly installed MA048 Holdfast: decline or pay 3 Heat |
| Collect discarded materials | Active due SH105 Heavy Magnet Lift: only complete legal two-unit leave-behind requests returned by core preview |

For Heavy Magnet, ordinary Collect can be cancelled before committing. Precision
first records the measured result, then holds that result and the original
steering while the player chooses the discarded units. That modal cannot reopen
the timing test, award the haul early, or accept underlying combat actions.
Only the final selection submits one campaign transaction. Closing the app and
using Continue retains the established policy: restart the original fight entry,
not a newly invented mid-fight resume policy.

## Focused validation

From the repository root, use the installed CMake executable:

```powershell
cmake -S games/overkill-foundry/unreal/Tools/recipe-control-tests -B .local/overkill-foundry/recipe-control-tests -A x64
cmake --build .local/overkill-foundry/recipe-control-tests --config Release --target foundry_recipe_controls --parallel 4
ctest --test-dir .local/overkill-foundry/recipe-control-tests -C Release --output-on-failure
```

The executable compiles the same production helper and shared core, exercises a
configured action path for each of the 246 recipes, and compares previews with
commits without mutating the preview's input. Focused checks cover explicit zero
versus maximum payment, exact victim identity, reordered reserved payments,
Unload, delayed Holdfast output, qualified discounts and rejected allocations,
Fine Mould's selected output and Heavy Magnet's fixed Precision categories.
These controlled cases do not establish natural progression, balance, native
input handling, rendered layout or every cross-recipe combination.

The separate `Tools/ui-fixtures` generator adds four deliberately prepared saves:

- `recipe-options`: twenty recipes, abundant supplies and selectable physical parts.
- `recipe-heavy`: next-round Heavy Magnet collection with unused Precision.
- `recipe-shot-synthetic-three`: one Mite and two Rams for synthetic multi-target
  layout coverage, reserved payment, support/Spread targeting and Return Delivery.
- `recipe-discount`: a legally acquired MY1-09 with three paid uses completed.

Generate into a **new** directory. The generator refuses to overwrite saves.
Open a copied prepared save with `unreal.ps1 Run -SavePath <path> -InspectSave`;
ordinary Continue intentionally restarts a fight. Prepared resources, copies,
parts and synthetic formations must never be described as earned gameplay.

## 2026-09-20 checkpoint

The first native pass used module `4905bff67301b67cd0537c67aca8a5a104b8b0dfb871b256e079995049a984c6`
and game executable `a9cf9ecce61ec331ba4297f7aa34ce4dafcaf1e0811a29029cbce67ccd583b51`.
Its exact supporting sources and binaries are in the ignored
`Saved/Validation/roster-v001-runtime-20260920-1843` archive. The copied save,
capture and event evidence is in
`Saved/Validation/recipe-controls-native-20260920-1845/native-pass-evidence.json`.
Input was native mouse/key injection with screenshot inspection by the engineer;
it was not an owner playtest.

That pass exercised real Precision timeout followed by Heavy Magnet discards,
Escape and focus changes, then one saved collection and one result cue after the
save. It also exercised MA096's six-Heat installation, MA022's exact Ammo
sacrifice, SH085's chosen five-Shield payment, SH103's second output, SH080's two
materials, MA105's Burn transfer, SH101's paired sacrifices/materials, MY1-09's
zero-payable fourth use, and MA048's three-Heat payment and next-round delivery.
The synthetic one-Mite/two-Ram case selected and reordered a physical SH110
reservation, selected support/Spread targets, loaded into settled 16C, fired by
keyboard and returned to preparation in the same round. SH072 had expired at
End Turn in that native case; its selection is covered by the helper checks,
not by a native Return Delivery claim. Native logs are `Run-185422`, `185738`,
`190937` and `191054`, all with the full `20260920-` date prefix.

Three observed presentation defects were repaired after that pass:

- Selecting another recipe resets its detail scroll to show its name/effect.
  Changes to the current recipe's options preserve its current scroll.
- A spent Fine Mould no longer offers an enhanced-output selector.
- Loaded rows show the actual firing order, with each reserved Shield directly
  below its physical source and without a firing-order number.

The focused strict CTest then passed **3,090 assertions / all 246 recipe paths**,
including reversed loaded order and absence of a second Fine Mould enhancement.
This final build deliberately uses the same frozen pre-terminal-notification
core because a separate core repair was pending save compatibility decisions.
All twenty core source/header files match Git revision
`1a2027d33e2af07344745c0d529c8afd1d84ede5` after LF normalization. The exact archived
bytes have engine build digest
`b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf`.
Normal builds still default to current shared core; this override is explicit:

```powershell
$reviewCore = 'games/overkill-foundry/unreal/Saved/Validation/roster-v001-runtime-20260920-1843/source/core'
& games/overkill-foundry/tools/unreal.ps1 Build -CoreRoot $reviewCore
& games/overkill-foundry/tools/unreal.ps1 BuildGame -CoreRoot $reviewCore
```

Both builds succeeded: `Build-20260920-192030.log` and
`BuildGame-20260920-192257.log`. Tools were Unreal **5.8.2 / CL56702186**, MSVC
**14.44.35228**, Windows SDK **10.0.22621.0**. The new module is
`d8cdfb1fd618a67ad542de10d2bd4b953093d9b447bea4eaf373795a2be3027a`;
the executable is `26b2674b60df1ca325e22045b79dcac0de9bdfa35e8660771f6918417aebeec7`.
`FoundryHost`, `FoundryRobot` and the roster mapping are unchanged from the
separately reviewed 80-case roster build. The runtime delta is confined to
`FoundryCampaignUI`, `FoundryRecipeControls.cpp/.h` and one extra preflight check.
Two helper descriptions also replace implementation wording with player text.

Final `CampaignProbe-20260920-192454.log` passed **30** focused saved-transaction
checks and the actual entry/shop/fight/reward/next-route sequence: position 2,
68 HP, 40 receipts, save revision 42, final campaign hash `7f25f5574f362f27`,
16 settled rendered captures. It uses automated commands, not physical input.
`Fixture-20260920-192703.log` reproduced all **85 ordered lines** from the native
teaching runner, ending at `965951ac6f2b9563` (80 HP, round 3, four shots).
The LF-normalized transcript SHA256 is
`dce5d0611e298195c84d4bde9b20504d2bb0cc897edcd17260542f245b181fc1`.
Both engine runs recorded unchanged module hashes before/after launch.
An extra shell-level `$LASTEXITCODE` check incorrectly stopped a chained command
after the successful CampaignProbe; its own recorded process exit was zero.
Fixture then ran separately and passed. No game failure was concealed by this
harness correction.

Native repair rechecks used `Run-20260920-192749.log`, `192903.log` and
`192949.log` (same full prefix). `final-native-evidence.json` and
`final-fixture-origins.json` record their original save hashes, unchanged module
identity and exact transaction events.
The repaired recipe scroll, absent spent-Mould selector and loaded order were
visibly checked; those first two inspection runs made no transactions. The
third run used the actual Open Exhaust (MA058) button, committed one save with
two deaths and no Fire event, showed death playback before the reward panel,
then showed Salvage secured. That deliberately prepared case used two enemies
at 1/1000 HP; it is transition coverage, not a natural fight.

Key native frames in the ignored evidence directory:

- `final-native-scroll-before-selection.jpg` and
  `final-native-new-recipe-scroll-top.jpg`;
- `final-native-spent-mould-no-selector.jpg`;
- `final-native-loaded-firing-order.jpg`;
- `final-native-utility-kill-ready.jpg`,
  `final-native-utility-death-hold.jpg`, and
  `final-native-utility-rewards-after-deaths.jpg`.

The final module/executable hashes above were unchanged after all native runs.
The exact source graph, baseline delta and artifact hashes are in
[`checkpoint.json`](checkpoint.json), produced by `capture_checkpoint.py` with
the explicit frozen core root and 1843 source archive as its comparison base.

The focused CMake check supports the same explicit source snapshot with
`-DFOUNDRY_TEST_CORE_ROOT=<absolute snapshot directory>`; omitting that option
uses current core. Its actual test log is archived with the checkpoint.

Remaining acceptance limits: these prepared cases are not natural acquisition
of all 246 recipes, all cross-recipe combinations, balance, owner play or final
visual approval. Shot/city scenery and overall polish remain below the chosen
benchmark. The next separate UI acceptance task includes a full saved Cinderwall
replay, an independent save-profile chooser, and a Game Over presentation with
the required gloating hostile AI. The current title has no profile chooser and
the defeat text alone does not meet the latter requirement. A minor existing
Host caption says "Returning to preparation..." during the Utility victory
hold; the next screen is rewards. This checkpoint did not edit that Host path.
