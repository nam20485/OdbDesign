Key finding: the degeneracy is server-wide, not sample_design-specific
Every loadable design on the server emits ComponentRecord.id == 0. The deployed server never populates the proto id field (9); it populates index (10), but only per-side (top and bottom each restart at 0). So "switch to a proper design" can't escape it — there is no design with real unique component Ids. The subagent correctly refused to force a bad choice or silently add a hack.

What this means concretely (surveyed across 7 designs / 9 steps):

| Data | Status on every design |
| --- | --- |
| Components list (RefDes, part, package, X/Y, rotation, pins) | Correct — enumerated by unique RefDes |
| Net → component connections | Correct — 0 unresolved; resolves via the netlist's ordinal reference |
| Packages / Parts / Drill tools | Correct |
| Per-component net summaries (ComponentDetail.Nets) | Merged — keyed on (side, id=0), so all components on a side share one bogus net list (a 1-pin mounting hole reports 610 nets) |

The merged per-component-net list is the only casualty, and no current tab displays it — it only matters when Task 2 renders the Component→Nets hierarchy. Smallest real designs: Panel - vplanproject_1230/vplanproject (257 comps, but panelized), ap50132476/pcb (643, normal board), sample_design/step (813, canonical). Turbot is ~175 MB (avoid).

The important nuance on "no workarounds"
The netlist already references components by their per-side ordinal (index), and net→component resolution succeeds via that ordinal (0 unresolved everywhere). So (side, index) is arguably the real component identity in this data model — the builder keying on the unpopulated id is the actual mismatch. Re-keying on index would be aligning to the server's real foreign key, not contorting around bad data.