# Testing & Validation Instructions

## Testing Approach

- Use the TDD (Test-Driven Development) approach for all code changes.
- Write unit tests before implementing new features or changes.
- Iterate, cycling between writing tests and code until the feature is complete.
- Coverage should be substantial, aiming for at least 80% coverage.
- Always run `dotnet build` before suggesting changes are complete
- Use `dotnet test` for unit test validation  
- Test Docker builds with `docker build` when Dockerfile changes are made

## Continuous Integration
- Use GitHub Actions for CI/CD workflows.
- Include workflows for:
  - Automated builds
  - Automated tests
  - Docker image builds and pushes to the repository's GitHub registry

## Links for TDD with AI Agents:
- [Test-Driven Development with AI Agents](https://www.flowhunt.io/blog/test-driven-development-with-ai-agents/)
- [TDD with AI Agents](https://www.flowhunt.io/blog/tdd-with-ai-agents/)

## Validation Steps

- Compile/build before marking tasks complete
- Provide specific test commands for user verification
- Include error handling and retry logic in instructions

## Documentation Standards

- Include relevant Microsoft Learn links: <https://learn.microsoft.com/>
- Reference Google Cloud docs: <https://cloud.google.com/docs>
- Link to ASP.NET Core guides: <https://learn.microsoft.com/aspnet/core/>
