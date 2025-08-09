# ASP.NET Core Community Toolkit Copilot Instructions

## Code Style and Structure
- Write concise, idiomatic C# code.
- Follow ASP.NET Core's standard folder and project structure (e.g., Controllers, Services, Models, Data).
- Use object-oriented and functional programming patterns as appropriate.
- Prefer LINQ and lambda expressions for collections.
- Use descriptive variable and method names.
- Adhere to separation of concerns between layers (Presentation, Business, Data Access).

## Naming Conventions
- PascalCase for class names, method names, and public members.
- camelCase for local variables and private fields.
- UPPERCASE for constants.
- Prefix interface names with "I" (e.g., IUserService).

## C# and .NET Usage
- Use C# 10+ features (record types, pattern matching, null-coalescing assignment).
- Leverage ASP.NET Core and Community Toolkit libraries for common functionality.
- Use Entity Framework Core with standard DbContext and repository patterns.

## Syntax and Formatting
- Follow C# Coding Conventions.
- Use expressive syntax (null-conditional, string interpolation).
- Use `var` for implicit typing when the type is obvious.
- Keep code clean and consistent.

## Error Handling and Validation
- Use exceptions for exceptional cases only.
- Implement error logging with ASP.NET Core's built-in logging system.
- Use Data Annotations or Fluent Validation for model validation.
- Use ASP.NET Core's global exception handling middleware.
- Return appropriate HTTP status codes and consistent error responses in API controllers.

## API Design
- Follow RESTful API design principles.
- Use ASP.NET Core's standard API controllers and attribute-based routing.
- Integrate API versioning if needed.
- Use action filters or middleware for cross-cutting concerns.

## Performance Optimization
- Use async/await for I/O-bound operations.
- Use IMemoryCache or IDistributedCache for caching.
- Write efficient LINQ queries and avoid N+1 problems.
- Implement pagination for large data sets.

## Key Conventions
- Use ASP.NET Core's built-in Dependency Injection system.
- Use repository pattern or EF Core directly as appropriate.
- Use AutoMapper for object mapping between DTOs and entities.
- Implement background tasks with IHostedService or BackgroundService.
- Use domain events and entities following DDD principles.
- Keep business rules in the Business/Domain layer.
- Avoid unnecessary dependencies.
- Follow clean architecture principles.

## Testing
- Use standard .NET testing tools with xUnit, NUnit, or MSTest.
- Write unit tests with chosen testing framework.
- Use Moq, NSubstitute, or similar for mocking.
- Implement integration tests for API endpoints.
- Use TestServer for integration testing ASP.NET Core applications.

## Security
- Use ASP.NET Core Identity for authentication and authorization.
- Implement JWT tokens or cookie-based authentication as appropriate.
- Use HTTPS and enforce SSL.
- Configure CORS policies as needed.
- Implement proper authorization policies and claims-based security.

## API Documentation
- Use Swagger/OpenAPI for API documentation with ASP.NET Core's built-in support.
- Provide XML comments for controllers and DTOs.
- Follow standard API documentation practices.

Refer to official Microsoft, ASP.NET Core, and Community Toolkit documentation for best practices in routing, domain-driven design, controllers, services, and other ASP.NET Core components.
