# Optimize Workflow Runs

Create a plan to overhaul and optimize the entire set of workflow runs, then execute it.

## Dependencies

- Gemini MCP server tool accessible and working.
- Ability to update code.
- Ability to create branches, issues, PRs, and reply to/resolve PR review comments.
- Create and check out a new branch for this work.

## Current Problems

- Organically grown workflows without an explicit strategy.
- Many steps take > 20–30 minutes.
- GitHub caching strategy is unimplemented or weak.

## Acceptance Criteria

- Reduce median CI runtime for typical PRs from ~30 minutes to < 12 minutes.
- Implement caching for vcpkg, CMake build artifacts, and test dependencies to persist across jobs.
- Parallelize build/test matrix (OS/toolchain) where feasible with constrained concurrency.
- Add actionable failure logs and artifacts for triage (e.g., ctest output, code coverage reports when enabled).
- Document local reproduction steps and CI caching keys in docs.

## Measurement Plan

- Establish baseline timings from the last 20 workflow runs and record median/p95 per job.
- After changes, measure the same metrics and include deltas in PR description.
- Add a lightweight dashboard in README or a badge set to expose runtimes and success rate.

## Plan

- Collaborate with Gemini (via gemini-mcp brainstorming or appropriate tool) to identify problems and get feedback.
- Based on feedback, create a detailed step-by-step plan with a to-do list.
- Add explanations and rationale for each change.

## Review Plan

- Review the plan for feedback with Gemini-MCP.
- Incorporate review feedback.

## Apply Plan Review Feedback

- Optimize and update the plan based on the review.
- Create a GitHub issue documenting the plan.

## Execute Plan

- Once the plan is created, reviewed, and optimized, apply the plan.
- Create a new branch and PR based on the issue created in the last step.
- Commit changes to the PR.

## Review Changes

- Once changes have been applied, review the changes in the PR.
- Have Gemini and yourself conduct separate PR reviews.

## Apply Review Changes

- Once PR reviews are finished, collect the list of PR review comments.
- Create a detailed plan to resolve the comments.
- Execute the plan, update each PR review comment with a summary of the fix, and then resolve that PR review thread.

## References

- Workflows to audit: .github/workflows/* (cache keys, matrix, upload/download artifacts, concurrency)
- vcpkg caching examples: GitHub Actions docs
- ccache/sccache integration for C/C++ builds
