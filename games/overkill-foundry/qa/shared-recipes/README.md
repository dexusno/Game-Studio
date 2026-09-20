# Independent Shared probes

These probes target the archived `9b4b7a2b909c` graph, never current production. The report and JSON list exact paths and hashes. Large executable/source copies remain ignored under `core/build/qa-shared-recipes/frozen-9b4b7a2b`.

Rebuild the captured `inputs/core/tests/shared-recipes` project into a separate directory using MSVC Release. Configure this folder with `FROZEN_GAME` pointing to those captured `inputs` and `FROZEN_LIBRARY` pointing to the newly built `overkill_core.lib`; then build and execute `independent_shared_recipes`.

`python qa/shared-recipes/audit.py` from the game root checks retained original review records and frozen inputs/artifacts without writing or checking mutable production. `clause-review.json` records the finite manual per-ID dispositions. `probe-results.txt` records seven passed groups and a separately unaccepted SH084 observation.
