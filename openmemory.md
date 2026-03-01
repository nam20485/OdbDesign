## Overview
- Repo: OdbDesign (C++17). Core library parses ODB++ archives into domain models/Protobuf, server exposes REST (Crow) and new gRPC endpoints. DesignCache provides cached FileArchive/Design access backed by test data under `test_data/FILES`.

## Architecture
- Library: `OdbDesignLib` contains FileModel (parsing, FeatureRecord, SymbolName, FeaturesFile), ProductModel, protobuf bindings under `protoc/`.
- Server: `OdbDesignServer` (Crow REST + gRPC). gRPC service implemented in `OdbDesignServer/Services/OdbDesignServiceImpl.{h,cpp}` with endpoints `GetDesign`, `GetLayerFeaturesStream`, `GetLayerSymbols`, `HealthCheck`.
- Tests: `OdbDesignTests` built with gtest; fixtures in `OdbDesignTests/Fixtures` provide DesignCache backed by sample design archives. CMake target `OdbDesignTests`.

## User Defined Namespaces
- (none yet)

## Components
- `OdbDesignServer/Services/OdbDesignServiceImpl`: gRPC service using DesignCache to serve designs, stream layer features, and assemble ordered symbol lists with unit normalization and strict contract checks.
- `OdbDesignLib/FileModel/Design/SymbolName`: Parses symbol records (`$index name unit`), exposes name/index/unit and protobuf conversions.

## Patterns / Notes
- Symbol ordering in `GetLayerSymbols`: normalize units → validate symbol unit compatibility → compute max referenced sym nums (sym_num/apt_def_symbol_num) with contract check → order symbols (explicit indices first, keep first on duplicates; fill gaps with index-less symbols) → pad with placeholders to cover referenced IDs.
- Symbol parsing errors: malformed index tokens now raise `parse_error` instead of silently defaulting.

