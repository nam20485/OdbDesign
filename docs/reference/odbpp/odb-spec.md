# ODB++ Specification reference

This repository includes a copy of the ODB++ specification PDF for convenient, offline reference.

## In-repo copy (preferred)

- `docs/odb_spec_user.pdf`

## Public copy (Siemens / ODB++ site)

- https://odbplusplus.com/wp-content/uploads/sites/2/2024/08/odb_spec_user.pdf

## Version note

- As of **2026-01-11**, the latest version referenced by this project is **8.1 update 4**.

## Why this is tracked here

When debugging parsing/serialization issues, it’s easy to infer behavior from code and sample designs but still miss **format guarantees** (optional vs required fields, grammar, units, and edge cases). Use the spec as the source of truth for those assumptions.
