# Independent Mara boundary probes

Use the copied `core/build/mara-semantic-05/source` graph and identities in `qa/mara-recipe-evidence.json`. Do not substitute mutable production files.

Rebuild captured `source/core/tests/mara-recipes` in a separate directory with `MARA_CORE_ROOT` explicitly pointing to captured `source/core`, then run CTest. Configure this QA folder using `FROZEN_GAME` for that captured `source` root and `FROZEN_LIBRARY` for the rebuilt `mara_contract_core.lib`. Build Release and run `independent_mara_recipes`. Windows CMake paths use forward slashes.

The optional argument `M01` through `M07` runs one complete independent group. To reproduce old negative controls, build the same source against `mara-semantic-03/source` and its preserved library; M01 and M02 must fail. The current author suite's `--case` interface did not exist in historical03.

`audit.py` validates copied sources, retained metadata/artifacts, exact source rows, bounded proposal links and unchanged failed history without writing production or rerunning author capture. `clause-review.json` retains a disposition and residuals for each of107 IDs. Assertions and group labels do not count as exhaustive source/interactions approval.
