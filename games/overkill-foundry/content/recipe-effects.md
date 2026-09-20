# Shared/Mara executable recipe bindings

The C++ core executes the 246 source recipes in `catalogue.snapshot.json`. `compile_recipe_effects.py` emits **metadata only** (ID, name, costs, rarity, count and output kind). Runtime behavior is explicit C++ in `core/src/recipe_effects.inl`, called by the single authoritative Engine in `core.cpp`. The 19 original primitive definitions remain in `starterContent()` and are reused by `completeContent()` with their complete source metadata. No description is evaluated at runtime.

Reproduce metadata with `python -X utf8 games/overkill-foundry/content/compile_recipe_effects.py --check`. Run `tools/core.ps1 -Action test` from the repository root. The current dedicated recipe suite exercises every recipe's action path, tabulates each Ammo's controlled main damage and each Shield's first calculated value, and checks selected interactions. Runtime registration is distinct from independent semantic evidence. `Rules::implementedRecipeIds()` describes registered recipe dispatch, not certification of all combinations, the upgrade pool, balance or the final game. `runtime-support.json` remains integration-owned.

## Integration contract

The existing `Rules::apply`, `preview`, `serialize`, `stateHash`, action constructors, fixture and custom primitive Effects remain available. All actions are candidate-state transactions: rejected validation leaves the live state, resources, instance identities and RNG unchanged. The snapshot contains bindings, pending deliveries, immutable physical-part metadata, attachments and robot clocks. Schema 3 / rules `of-core-0.3` / content `cinderwall-recipes-0.2` reject earlier snapshots rather than silently dropping effects.

`Rules::grantPart(state, recipeId, installed=false)` gives one canonical physical output without a Recipe Use, cost or cooldown. It does not activate a Utility or execute an automatic multi-output generator. `grantPlainPart(state, Kind::Ammo/Shield, value, source, installed=false)` gives an explicitly generated fixed-value part. Both return real immutable events. They support purchased/granted parts without disguising them as crafting; callers still own their enclosing campaign transaction.

| Action field | Meaning |
| --- | --- |
| `subject` | Owned recipe-copy ID for Craft; physical-part ID for Install/Remove/Activate. |
| `target` | Main Fire target or the enemy chosen by a Utility, Modifier or Officer Brace. |
| `parts` | Bullet in placement order for Load; selected unused reserve part(s) for copy, sacrifice or attachment recipes. |
| `amount` | Chosen variable/optional Heat or installed-Shield payment. For Fine Mould affecting a multi-output Craft, zero-based output index (default first). |
| `choices` | Material indices Iron=0, Copper=1, Carbon=2, Glass=3, Circuit=4, in the printed choice order. Gathering choices are made at Craft, preserved on the part, then bound at Activate. |
| `targets` | Additional ordered enemy choices for Utility operations; Fire Exchange uses one second target. |
| `sacrifices` | Reserve payment IDs. Welding Clamp consumes one Ammo at first Install. Load pairs one unused Shield per Full-Spread Outlet in bullet order, reserves it until Fire, and releases it on Unload/End Turn. |
| `partTargets` | Physical bullet-part ID to chosen additional enemy. Chipping Tip, Breakaway Core, Follow-Through Ember, Forked Tip, Trailing Wick and Coal Scatter use one each while another enemy exists. It also accepts spreading-part targets. |
| `spreadTargets` | Existing spread-target interface, retained. Three-Way Nozzle accepts zero to two distinct other enemies. Automatic all-enemy spreads need no chosen target. |
| `partChoices` | Return Delivery uses one `{part: 0, enemy: selectedAmmoId}` pair at Fire. Holdfast uses `{part: installedHoldfastId, enemy: 0 or 3}` at End Turn for its explicitly later optional Heat payment. The second member is a selection value in these two legacy-compatible cases; it is not an enemy target. |
| `discarded` | Material counts actually gathered and left in the pile by Heavy Magnet Lift. Must total its required discard and cannot spend previously held supplies. |

`legalActions` remains a convenience list, not an exhaustive action search or balancing policy. It cannot enumerate every combination of targets, payments, reserved parts and material choices. The renderer and runner must construct typed selections and use `preview`/`apply` for authoritative legality.

## Physical identity and clocks

- Recipe copies have independent cooldown and once-per-round state. Each physical output has its own identity, creator/source, rarity, immutable resale reference and standard material basis. Discounts do not reprice the output. Warm Rivets use their canonical one-part basis. Generated fixed-value grants use the locked pure Ammo/Shield family basis.
- Fresh copies reproduce the source's printed output (including fixed generated subparts), keep its rarity/resale basis, start their age anew and drop attached bonuses. Their delivery is an effect, not another Recipe Use.
- First Shield installation pays once, snapshots values once and binds once. Removing/reinstalling changes only currently applied protection/order. Detached reactive parts cannot trigger. A pure ordinary Shield grant removed at full untouched strength remains unused for eligible copying or sacrifice; first-install bookkeeping is still preserved.
- Next-shot effects refresh by source name; different names combine. Physical Ammo/Shield copies retain their separate printed effects unless the source specifies a shared cap. Recorded deliveries survive ordinary source removal, while source-presence hooks still check their installed source.
- Part-only Heat/HP payment counters are distinct from direct Utility Use payments and general cost telemetry. This matters to Furnace Ledger, Furnace Rest, Deep Freeze Cast and related source-specific hooks.
- Fire snapshots ordinary conditional inputs before its costs/pre-hit changes. Explicit pre-hit operations then run in bullet order. Support and spread hits use live defenses and cannot retarget; final player death interrupts their remaining ordinary payloads.

## Gathering stock and future work

`State::finitePile` is false unless the encounter supplies an actual finite pile; there is no invented stock cap. With finite stock, `pile` is the remaining available material stock. Normal haul preferences allocate normal positions in fit order; the ordinary Precision bundle is recorded separately; named extras then request only remaining stock. Missing bonus stock does not reduce baseline supplies or restore a spent attempt. A finite encounter must supply enough total stock for its guaranteed baseline. The caller may supply a newly authored pile for a later collection; this is not a carried-inventory limit.

The recipe tests are graphics-free. They do not prove the UI exposes every required choice, that all upgrade listeners are implemented, that a full-city policy balances well, or that the game feels good. Independent QA owns `qa/expanded/` and its review. The integration owner must reconcile the final evidence, render the same core through Unreal, implement P08 upgrade hooks and connect the full-city runner before the release gate can be cleared.
