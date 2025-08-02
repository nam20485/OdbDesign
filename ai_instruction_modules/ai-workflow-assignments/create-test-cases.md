# Assignment: Create Test Cases

## (create-test-cases)

### Goal

Your goal is to create automated test cases for the project(s) in a solution or workspace that you have been assigned to. You will iterate, creating new test cases, until you are told to stop. 

### Acceptance Criteria

1. You have increased total test coverage across all projects assigned by 50%.
2. Test Projects for each code project have been created.
6. All projects build and runs successfully.
8. All test cases pass 100%.
7. All acceptance criteria from the template have been satisfied
4. Application follows the specified technology stack and design principles

### Assignment

Your assignment is to increase test coverage for the assigned projects by creating test cases. You will continue to create more and more test cases until told to stop. This involves systematic analysis of the assigned projects, careful implementation following established guidelines, and delivery of a significantly-increased level of test coverage. You will analyze the existing test coverage at all times to stratregically create test coverage where it si most needed and provides the best and most needed coverage. Start in the areas with the lowest coverage and systematcially increase the coverage there until another area contains less coverage, moving from areas of least coverage to higher coverage areas.

You will create both unit tests and integration tests. Ensure that both types of suites exist.

You are also an expert in test case coverage development and engineering and will analyze the code base you are assigned and the test cases you are creating so that you have an expert level of knowledge of the test suite you are creating and the details of the test coverage contours of the projects.

Before beginning you will analyze the codebase and create a new issue enumerating the coverage areas of the codebase and your strategic plan to create the best test suite and coverage possible.

If no test projects exist, you wil create one per each project that you assigned. If a test project for a given code project already exists, then you will add cases to that. 

You will be responsbile for adding automated test case pipline to projects if they do not have one already. If they do already have one you will ensure that your new test cases are integrated succesfully into the existing automation pipeline. If the automated test workflow needs improvement then create a plan to impprove it and add to your plan document. 

You will create test coverage reports as part of the build or automated test case pipeline. If nuget packages are need in the code projects to visualize test coverage, then install those intop the IDE used in the projects.

## Detailed Steps

1. Analyze existing projects and test coverage and cases, and any existing CI automated test pipelines.
2. Create an issue documenting your plan.
3. Ask for approval for your plan. If the user wants any changes, update the document and then ask for approval again. You will iterate making changes until you receive approval.

**You will not make any changes until approval is obtained.**

4. Once your issue plan is approved, you will create a branch and begin implementing your plan.

**Always follow instructions given here (generally) and specifically as described in the task-based workflow process.**

It is important to the final quality of our product for everyone to perform their assignment exactly as specified.

## Stack 

Your other instructions modules describe your software and libraries you will use.


### For C#
* xUnit
* Moq
* Bunit
* Fluent Assertions
* Test Coverage Visualization NuGet packages like Coverlet or integrated Visual Studio functionality.

### Powershell
* Pester

### Docker
* The best Docker testing frameworks you can find.

### C++
* Catch2

### Completion

Ask the stake-holder if they want you to merge the PR or if they would like to do so themselves.
If so, then merge the PR, and close the issue and branch.
Eat a cookie and have an espresso or a milkshake. You can have both if you want.
Ask the stake-holder for your next assignment.
