# linux-dynamic-release-plan

## Objective

The goal is to replace the current `linux-release` preset and binaries with the new `linux-dynamic-release` preset and binaries.

## Background

We discovered that the current derfault/main Cmake preset on linux, `linux-release` as fatal flaw bc it links protobuf lib statically which cause two copied to be loaded at runtime which leads to derenferencing uninitialized pointers and crashed. The new preset `linux-dynamic-release` links the library as a shared dynamic lib, which removes and resolves the issue.

Since the `linux-release` preset and binaries are the main build used in everything from all of the Docker imaages, k8s deployment, the build workflows (build, test, publish docker, upload artifacts, the release, the docs, README, the build envirnoment setupo scripts, etc. etc.) everywhere its used needs to now be replaced wuthg the new dynamic preset build, `linux-dynamic-release`.

## Tasks

Analyze the project and updaate the references, then validate everything.

Success looks like:

- the cmake preset can configure and build successfully
- all test pass
- all the updtaed build and test etc. workflows succeed
- anything else you find that needs to be updated

## Acceptance Criteria

- All the above tasks are completed and validated.

## Plan

Create a plan in a new markdown file to accomplish the above tasks.

- Present options with pros and cons, and recommend a course of action with why.
- Present to me for approval.

## Branch

Create n new branch and PR for this work. Branch/PR's base should be the `nam20485` branch.
