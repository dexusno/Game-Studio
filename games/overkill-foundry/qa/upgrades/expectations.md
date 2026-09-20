# P08 independent expectations

These cases were selected from the source contract before the first stable P08 build. The implementation author does not own this suite. Numeric inputs deliberately distinguish plausible errors; registration of all 152 IDs is structural evidence only. The final review will list the cases actually executed and their exact build identity.

Sources: [upgrade rules](../../design/UPGRADE-SYSTEM.md), [timing](../../design/TIMING-AND-PERSISTENCE.md), [recipe rows](../../design/RECIPE-CATALOGUE.md), and the exact upgrade rows recorded by `source_oracle.py` from both source JSON catalogues.

| Boundary | Selected discriminating expectations |
| --- | --- |
| Acquisition and persistence | UGS-039 rejects 8 HP atomically; at 9 pays 8 once and persists the unfinished Ammo/Shield choice. Loading or resolving that choice does not repay. UGS-071 increases max HP once and does not reacquire on reload. Duplicate owned IDs reject. |
| Physical recipe copies | UGS-039 buffs the first of MA038's two outputs, only from the selected copy. UGS-144/145 lower the first numeric cooldown assignment independently for two copies; None recipes cannot be chosen. UGS-102 uses producing-copy identity, once per turn, resets across a missed turn. |
| Free output versus paid Use | UGS-009 counts three qualifying productions, not passive grants or copies; UGS-015 copies committed first-output properties without replaying Use or delivery. MY1-09 discounts only after three earlier material-paying Uses and cannot waive Heat/HP costs. UGS-136 counts paid Utilities across rounds. |
| First-turn use exceptions | MY1-21 permits exactly two Uses of the selected None copy on turn 1, both at full cost; other copies and later turns remain one Use. MY1-10 clears numeric cooldown without resetting a None-use flag. |
| Generated value and saving | UGS-140's first bonus survives saving the output; its original sale reference does not change. Generic grants use original N for immutable resale. Copies follow their own printed-effects versus committed-properties contract. |
| Shield phase order | Real UGS-123 plus MY3-03 retain min(remaining,9), not 9 unconditionally. SH066's full-group payments precede retention; first-bound MA055 readers see the value available at their own place in the ordered sequence. T05/T06's exact Noor/later-city effects remain outside this pool. |
| Immediate reaction ordering | UGS-019 reacts to individual enemy attack hits that remove protection/HP; UGS-076 and UGS-066 can provoke recoil. Final lethal recoil interrupts later ordinary work. The actual UGS-067 rescue is excluded by city bounds; the legacy rescue fixture is not P08 evidence. |
| HP, Heat and combined affordability | MAU-03 triggers once for a single actual payment of at least 2 Heat, not decay or two 1-Heat events. MAU-05 heals only after an actual HP-cost recipe and its reactions finish. UGS-106 adds healing to an actual recipe heal, not max-HP acquisition or another upgrade heal. Later refunds cannot fund an upfront cost. |
| Timing and deterministic UI state | UGS-001 survives zero-shot turns and affects one main shot. UGS-139's +1 precedes percentages. UGS-094 cooldown ties use memory slot order rather than numeric copy ID. Pending nested choices preserve candidate lists, cursor and completed payments across preview and serialization. |
| Campaign integration | Real UGS-089 nested offers, UGS-032 full-memory copy/exchange, UGS-100 Utility-only slots and UGS-133 Borrowed lifetime; UGS-113 decline/exhaustion and UGS-150 exhaustion; ever-acquired IDs remain ineligible after spent effects. Save/reload, duplicate requests and Continue must preserve exactly-once boundaries using real P08 effects, not earlier test callbacks. |

Not acceptance evidence: controlled arenas do not test the graphical host, input focus, audio, full-city balance, human comprehension, fun, performance or the F.I.S.T. visual bar. Generated grants used as fixture setup are not a substitute for executing the named upgrade being tested.

Preparation correction: the source oracle rejected initially proposed UGS-027, UGS-043 and UGS-059 cases because their minimum city is 2. Those IDs were removed before compiling or executing this suite; this is a QA scope correction, not an implementation defect.
