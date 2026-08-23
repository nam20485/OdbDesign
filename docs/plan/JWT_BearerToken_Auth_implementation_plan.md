# Implementation Plan: JWT Authentication with Thalhammer/jwt-cpp

This plan covers implementing JWT bearer token authorization, token creation (`/login`), and a refresh token mechanism (`/refresh`) for `OdbDesignServer`.

## Open Questions
- **Token Secret Storage:** Where should the server's secret keys (for signing the JWT and Refresh tokens) be stored? Should they be loaded from `config.json` or environment variables?
- **User Authentication:** Since there's no database currently, how will `/login` authenticate users? Should we implement a hardcoded mock user for now, or read from a configuration file, until a real user DB is integrated?

## Database for Token-Related State

Since `OdbDesign` currently does not use a database, we have a few options for storing refresh tokens (which require state tracking to support revocation and rotation).

I propose **SQLite (via `sqlite3` or an ORM like `sqlite_orm`)** as it is lightweight, requires no separate database server, and easily integrates into a C++ CMake project. 
Alternatively, for a simpler immediate solution that requires no new dependencies, we can use an **In-Memory Store (`std::unordered_map`)**. However, this means all users will be logged out whenever the server restarts because the refresh tokens will be lost.

> [!WARNING] 
> Please let me know whether you prefer to introduce SQLite for persistent refresh token storage, or if an in-memory store is sufficient for this initial implementation.

---

## Proposed Changes

### Configuration and Dependencies

#### [MODIFY] `vcpkg.json`
- Add `"jwt-cpp"` as a dependency.
- Add `"sqlite3"` (if you approve the SQLite recommendation for token state).

### Authentication Core

#### [NEW] `OdbDesignServer/App/JwtAuthMiddleware.h`
- Create a Crow middleware `JwtAuthMiddleware` to handle `Authorization: Bearer <token>` extraction and validation.
- Integrate `jwt::decode` and `jwt::verify` using the server's secret key via `jwt-cpp`.
- Inject user claims (e.g., `user_id`) into the request context for downstream routes.

#### [MODIFY] `Utils/crow_win.h`
- Include `JwtAuthMiddleware.h`.
- Update the application type definition to include the new middleware:
  ```cpp
  using CrowApp = crow::Crow<crow::CORSHandler, JwtAuthMiddleware>;
  ```

#### [NEW] `OdbDesignServer/Services/TokenService.h` & `.cpp`
- Create a dedicated class to encapsulate `jwt-cpp` usage and state storage.
- `GenerateAccessToken(userId)`: Returns a signed JWT (short-lived, e.g., 15 minutes).
- `GenerateRefreshToken(userId)`: Returns a signed Refresh Token (long-lived, e.g., 7 days) and stores its hash/ID in the chosen database/state store.
- `ValidateToken(token)`: Validates the token's signature and expiration.
- `RevokeRefreshToken(token)`: Removes the token from the database.

### API Endpoints

#### [NEW] `OdbDesignServer/Controllers/AuthController.h` & `.cpp`
- **`POST /login`:**
  - Accepts a JSON payload with `username` and `password`.
  - Validates credentials (mock implementation pending database decision).
  - Calls `TokenService` to generate an Access Token and a Refresh Token.
  - Returns `{"access_token": "...", "refresh_token": "..."}`.
- **`POST /refresh`:**
  - Accepts a JSON payload with `{"refresh_token": "..."}`.
  - Validates the refresh token against the state store.
  - Generates a new Access Token (and optionally rotates the Refresh Token).
  - Returns the new tokens.

#### [MODIFY] `OdbDesignServer/OdbDesignServerApp.cpp`
- Register `AuthController` in `add_controllers()`.

### Protected Routes

#### [MODIFY] `OdbDesignServer/Controllers/...` (Existing Controllers)
- Update existing controllers (e.g., `DesignsController`, `FileModelController`) to read from the `JwtAuthMiddleware` context and require authentication where appropriate.

## Verification Plan

### Automated Tests
- Create unit tests for `TokenService` validating generation and verification.
- Use `ctest --preset linux-debug` to run tests and ensure the new library doesn't break compilation.

### Manual Verification
- Use `curl` to:
  1. `POST /login` to obtain an access and refresh token.
  2. Access a protected endpoint using the `Authorization: Bearer <token>` header.
  3. Verify a `401 Unauthorized` response when using an expired or invalid token.
  4. `POST /refresh` to obtain a new access token and verify it works.
