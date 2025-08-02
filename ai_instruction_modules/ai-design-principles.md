# Software Design Principles

These design principles, patterns, and rules should always be used when designing and writing code. Always strive to design and write your code according to these dictates. Using these principles will ensure that the code you produce will always be of high quality and easy to read and maintain.

## Principles

## 12 Factor App

[12 Factor App](https://12factor.net/)

### SOLID Design

[SOLID](https:/
/www.digitalocean.com/community/conceptual-articles/s-o-l-i-d-the-first-five-principles-of-object-oriented-design#introduction)

### Gang of Four (GoF) Deign Patterns

These are the classic patterns of design that ensure high quality. Be familiar with them and in what situation that each should be used. Only use a particualr pattern when it is appropriate to the situation. They are basic blueprints so you will probably need to modify them slightly to fit each situation.

[GoF Design Patterns](https://www.digitalocean.com/community/tutorials/gangs-of-four-gof-design-patterns#gof-design-pattern-types)

## Semantic Versioning (SemVer)

This pattern will be used whenever you need to use a version for something. We use v2.0.0 of the Semantic Versioning specification.

[SemVer v2.0.0](https://semver.org/)

## Domain Driven Design (DDD)

[Domain Driven Design](https://www.digitalocean.com/community/tutorials/an-introduction-to-domain-driven-design)

* Fluent API
* Code First

## Mandatory Applicaiton Requirements

Every application should always include these items.

* Automated Testing: Always include automated test cases with a substantial level of coverage.
  * Unit Testing
  * Integration Testing
* Documentation:
  * Always include a README.md or other documentation describing the application in detail, its configuration, and use.
  * Always include OpenAPI + Swagger + ReDoc documentation for all APIs.
* API Documentation: Always include OpenAPI documentation for all APIs.
* Initialization Scripts: Always include initialization and startup scripts to allow for the easiest and most efficient path for users to use the app.
* Containerization:
  * Include Docker image definitions wherever possible.
  * Include Docker Compose definitions when you have two or more Docker images in your project, or when your project depends on another respource, e.g. a DB.
* Logging: Always include detailed logging output to both stdout/stderr and log files.
* Infrastructure as Code: For deployable apps, provide Terraform defintiions to simplify infrastructure deployment and provisioning.

## General Rules

Always observe these rules in all applications you create and code you write.

* Increase warning levels to higher than defualts.
* Always turn on "Warnings as Errors" for all projects.
* If a specific warning is unavoidable, or if the warning is not relevant in this context, then use a pragma or project settings to disable that warning only.
* Always write descriptive XML documentation comments for all methods, classes, fields, properties. Use as much detail as possible, including at minimum:
  * Description
  * Each input parameter
  * Return value
  * Exception
  * Link any references to other well-known types.
* When possible ephemeral application configuration should be specified using envionment variables.
* Always pin all version numbers to the highest precision possible for all dependencies and libraries used. Ideally the pinning detail should be sufficient enough to specify one exact version.
