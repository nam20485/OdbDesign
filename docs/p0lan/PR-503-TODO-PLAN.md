## Complete PR Comment Status List
- [x] 2616115986 — Typo “FIleModel” → “FileModel” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616115987 — Typo “steam” → “stream” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616115989 — Typo “exsiting” → “existing” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616115991 — Typo “wil” → “will” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616115994 — SymbolName index parsing swallows exceptions; add explicit handling/logging.
- [x] 2616115995 — Document rationale for `kMaxReasonableIndex` magic number in GetLayerSymbols.
- [x] 2616115997 — Add tests for SymbolName index parsing (valid/invalid/missing).
- [x] 2616115998 — Document `collect_symbols` behavior.
- [x] 2616116000 — Clarify bounds check / non-negative idx handling before resize.
- [x] 2616116004 — Typo “pre-l;oad” → “pre-load” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616116005 — Typo “som” → “some” in `docs/plan/ipc2581/spec-issues.md`.
- [x] 2616116006 — Avoid redundant passes over feature records in GetLayerSymbols.
- [x] 2616116007 — Consider removing local alias or justify; prefer explicit type for ordered symbols.
- [x] 2616116008 — Add tests for GetLayerSymbols endpoint (success, errors, units, ordering).
- [x] 2616116009 — Add inline documentation for symbol placement logic (explicit indices first, etc.).

## Unresolved Comments List
- [x] 2616115986 — Fix typo “FIleModel” → “FileModel” in IPC-2581 spec issues doc. *Plan:* apply suggested text replacement.
- [x] 2616115987 — Fix typo “steam” → “stream” in IPC-2581 spec issues doc. *Plan:* apply suggested text replacement.
- [x] 2616115989 — Fix typo “exsiting” → “existing” in IPC-2581 spec issues doc. *Plan:* apply suggested text replacement.
- [x] 2616115991 — Fix typo “wil” → “will” in IPC-2581 spec issues doc. *Plan:* apply suggested text replacement.
- [x] 2616115994 — SymbolName index parsing should not swallow parse errors. *Plan:* surface parse_error when stoi fails; add coverage in tests.
- [x] 2616115995 — Explain `kMaxReasonableIndex` threshold. *Plan:* add inline comment with reasoning.
- [x] 2616115997 — Add SymbolName index parsing tests. *Plan:* extend `SymbolContractTests.cpp` with valid/invalid/missing index cases.
- [x] 2616115998 — Document `collect_symbols`. *Plan:* add comment describing vector/map extraction behavior.
- [x] 2616116000 — Clarify bounds check on index vs vector size. *Plan:* add explicit non-negative guard/assert before resize.
- [x] 2616116004 — Fix typo “pre-l;oad” → “pre-load” in IPC-2581 doc. *Plan:* apply suggested text replacement.
- [x] 2616116005 — Fix typo “som” → “some” in IPC-2581 doc. *Plan:* apply suggested text replacement.
- [x] 2616116006 — Reduce redundant feature record iterations. *Plan:* avoid extra passes by combining max computations and document reasoning.
- [x] 2616116007 — Replace local alias or justify. *Plan:* use explicit vector type for readability.
- [x] 2616116008 — Add GetLayerSymbols tests. *Plan:* add service tests using existing design cache fixture to cover success and error cases plus units normalization.
- [x] 2616116009 — Document symbol placement ordering rules. *Plan:* add inline comments before ordering loops.

