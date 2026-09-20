# Independent UC01 probes

The immutable candidate and original overlay paths/hashes are in `qa/upgrade-copy-evidence.json`. Build only captured inputs. The current live engine may contain later changes.

1. Rebuild captured `inputs/core/tests/upgrade-copy-contracts` into a separate directory and run its CTest targets.
2. Copy the original `49b5f504…/inputs`; apply the captured `uc01.patch` to that copy only; rebuild its `core/tests/upgrade-contracts` project and run CTest.
3. Configure this folder with `FROZEN_GAME`, `FROZEN_LIBRARY` and `ORIGINAL_QA_SOURCE` pointing to captured headers, the corresponding rebuilt library and retained original failed `probes.cpp`. Use forward slashes in CMake paths on Windows.
4. To create historical controls, also set `LEGACY_GAME` to the original captured inputs and `LEGACY_LIBRARY` to the preserved original failed review's library. Run `legacy_state_emitter` with an existing ignored output directory.
5. Run `original_upgrade_review`, then `independent_upgrade_copies` with that historical-save directory. Rebuild/run against the isolated overlay as well. The source and original failure artifacts stay unchanged.

`audit.py` checks retained input, artifact, scope and result identities read-only. It never checks mutable live inputs or overwrites an author capture. The legacy observation is a compatibility limitation, not a migrated-save pass. C04/C05 are explicitly controlled pending-delivery representations; the actual real upgrade acquisition/copy tests are separate.
