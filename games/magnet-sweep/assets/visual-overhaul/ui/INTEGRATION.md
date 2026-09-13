# Expedition recovery console

Original UI implementation in `ExpeditionUIVisuals.h/.cpp` and the UI functions in `ExpeditionPresentation.cpp`. No bitmap panel backgrounds or baked essential text. The console draws at the existing 1600 x 900 reference scale with measured runtime text wrapping.

- Two fonts: unmodified Barlow Condensed SemiBold for display headings, buttons and instrument numerals; Barlow Medium for explanations and consequences. Root supplies `FWorkbenchImpl::DisplayFont` and the existing `RuntimeFont` through composite fonts with a `Regular` typeface.
- Starter selection: six large equipment cards with original tool-family engineering glyphs, unlock states, clear equip actions and preserved descriptions.
- Outfitter: four equipment cards, equipment inventory, dedicated inspector, actual price/socket information and every original purchase/refit consequence before commitment. All button IDs and click behavior are preserved.
- Field view: warm ivory recovery-order hierarchy, explicit objective states, segmented battery and cargo instruments, separate Q/F equipment panels, contextual anchored-machine labels, and restrained brass/teal feedback. Hovering still exposes the actual body tooltip. Goal, hot and installed-frame labels remain visible.
- Notices occupy the upper console strip instead of covering workshop controls. Pause darkens the world beneath its physical console panel. Every underlying gameplay and input action remains unchanged.

The common panel treatment uses recesses, fine machined edge highlights, a clipped corner accent and limited fasteners on raised modal panels. Family accents align with the hero magnet's deep teal enamel, copper, steel, graphite and ivory direction.

Validation by this agent: scoped whitespace check; Click, BuildMechanismVisuals and UpdateVisuals remain identical to HEAD when this handoff is written. Fonts downloaded from the official Google Fonts repository with their original OFL 1.1 notices and SHA-256 records. No engine build or native launch was performed by this UI worker. Root owns integration, rendering inspection and packaging; inspect long pair consequences, selection states, late-site labels, 1280 x 720 scaling, and both unlocked/locked starter cards in the actual engine before acceptance.

Font notices must accompany the packaged font files. Manifest merge rows are supplied in `../fonts/manifest-rows.csv` and `manifest-rows.csv`; root owns the shared manifest.
