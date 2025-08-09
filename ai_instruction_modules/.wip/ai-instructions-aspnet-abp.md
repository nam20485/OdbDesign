# ABP .NET Copilot Instructions

## Code Style and Structure
- Write concise, idiomatic C# code.
- Follow ABP Framework’s folder and module structure (e.g., *.Application, *.Domain, *.EntityFrameworkCore, *.HttpApi).
- Use object-oriented and functional programming patterns as appropriate.
- Prefer LINQ and lambda expressions for collections.
- Use descriptive variable and method names.
- Adhere to ABP’s modular development approach to separate concerns between layers.

## Naming Conventions
- PascalCase for class names, method names, and public members.
- camelCase for local variables and private fields.
- UPPERCASE for constants.
- Prefix interface names with "I" (e.g., IUserService).

## C# and .NET Usage
- Use C# 10+ features (record types, pattern matching, null-coalescing assignment).
- Leverage ASP.NET Core and ABP modules (e.g., Permission Management).
- Use Entity Framework Core with ABP’s DbContext and repository abstractions.

## Syntax and Formatting
- Follow C# Coding Conventions.
- Use expressive syntax (null-conditional, string interpolation).
- Use `var` for implicit typing when the type is obvious.
- Keep code clean and consistent.

## Error Handling and Validation
- Use exceptions for exceptional cases only.
- Implement error logging with ABP’s logging system.
- Use Data Annotations or Fluent Validation for model validation.
- Leverage ABP’s global exception handling middleware.
- Return appropriate HTTP status codes and consistent error responses in HttpApi controllers.

## API Design
- Follow RESTful API design principles.
- Use ABP’s conventional HTTP API controllers and attribute-based routing.
- Integrate API versioning if needed.
- Use ABP’s action filters or middleware for cross-cutting concerns.

## Performance Optimization
- Use async/await for I/O-bound operations.
- Use IDistributedCache for caching.
- Write efficient LINQ queries and avoid N+1 problems.
- Implement pagination or PagedResultDto for large data sets.

## Key Conventions
- Use ABP’s Dependency Injection system.
- Use repository pattern or EF Core directly as appropriate.
- Use AutoMapper or ABP’s object mapping for DTOs.
- Implement background tasks with ABP’s background job system or IHostedService.
- Use domain events and entities as recommended by ABP.
- Keep business rules in the Domain layer.
- Avoid unnecessary dependencies.
- Do not alter dependencies between application layers.

## Testing
- Use ABP startup templates with Shouldly, NSubstitute, and xUnit.
- Write unit tests with xUnit and ABP’s test module.
- Use NSubstitute for mocking.
- Implement integration tests for modules.

## Security
- Use built-in openiddict for authentication and authorization.
- Implement permission checks with ABP’s permission management.
- Use HTTPS and enforce SSL.
- Configure CORS policies as needed.

## API Documentation
- Use Swagger/OpenAPI for API documentation with ABP’s built-in support.
- Provide XML comments for controllers and DTOs.
- Follow ABP’s guidelines for documenting modules and services.

Refer to official Microsoft, ASP.NET Core, and ABP documentation for best practices in routing, domain-driven design, controllers, modules, and other ABP components.
