---
name: game-research
description: Research and compare small commercial game opportunities using dated market evidence, explicit uncertainty, scope limits, and a testable hook. Use for trend scouting, competitor analysis, or an opportunity shortlist, not implementation.
---

# Game research

Read [CHARTER](../../../studio/CHARTER.md), [WORKFLOW](../../../studio/WORKFLOW.md), [TEAM](../../../studio/TEAM.md), and [research method](../../../research/README.md). Use `research/sources.json` as the maintained source catalog. Check existing opportunities before adding duplicates.

Frame the question around the intended player, one appealing interaction, target platform, achievable production scope, and discovery channel. A popular genre alone is not a useful game concept. Look for a distinctive hook that can be demonstrated quickly and tested within the owner's available time.

Use current accessible sources and preserve direct URLs and access dates. Record each claim as observed, reported, estimated, or hypothesis. Distinguish a source's publication date from the date of the behavior it describes. Prefer primary evidence for platform terms and direct store or developer evidence for competitors. Missing data remains missing; failed requests are not zero demand.

The `trend-scout` can own discovery and source notes; the `market-analyst` can independently assess competitors, pricing, audience, and assumptions. A `game-designer` may turn a supported opportunity into a compact loop and test. Give each separate files and a bounded research question. Use only roles needed for the current comparison.

For opportunity discovery, create a dated run under `research/runs/` and persistent candidate JSON under `research/opportunities/`, following [the record contract](../../../studio/IMPLEMENTATION-CONTRACT.md). For a focused comparison or question about an existing idea, answer the question or update its relevant evidence without inventing candidates or running the full funnel. When scoring helps the requested decision, use `python scripts/studio.py score PATH`; use `python scripts/studio.py validate` after record changes. Demand, distinctiveness, fun potential, scope fit, and distribution scores are decision aids; trend stage and confidence remain separate.

If Steam comparator data would help, use `python scripts/studio.py steam-snapshot --apps APPID [APPID ...] --out PATH` with real app IDs. This captures timestamped prices and defined review counts; it does not estimate sales. Do not convert reviews, concurrent players, or wishlists into revenue without an explicit sourced model and uncertainty. Never describe forecast revenue as observed income.

For opportunity discovery, end with a small ranked shortlist, the strongest counterevidence, the cheapest next test, and a recommendation that fits the budget. For narrower research, return the focused conclusion, sources, uncertainty, and next test only if useful. Mark candidates draft or shortlisted until there is an actual selection decision. Record reasons for holding or rejecting candidates rather than deleting them. Fun potential is a hypothesis until people have played the relevant interaction; agent opinion is not a playtest.
