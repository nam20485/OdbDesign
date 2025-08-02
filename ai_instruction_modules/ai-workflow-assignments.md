# Assignment-Based Workflow

The assignment-based workflow builds on the task-based workflow process by including the  new concept of assignments. Workflow assignments are specifically-defined sets of goals,  acceptance criteria, and steps.

## Workflow Assignments

* Each workflow assignment is unique and describes how to accomplish a specific task, start a new project/application, or stage of the project.
* Workflow assignments are assigned to you by the orchestrator
* When assigned, you are to perform the assignment until finished and/or assigned something new.
* Each type of assignment is described in a workflow assignment definiton file.
* Each definition file contains everything you need to know to be able to perform and complete an assignment successfully.

## Workflow Assignment Definition File Locations

* Definition files are found under the `ai_instruction_modules/ai-workflow-assignments/` directory in a subdirectory named exactly as the workflow assignment definition's "short ID" (See sections below for definition of an assignment's "short ID").

*WORKSPACE-ROOT*/
    ai_instructions_modules/
        ai-workflow-assignments/
            *<assignment_short_id>*        // e.g. "pr-approval-and-merge"

## Workflow Assignment Definition Format

The format is made up of the following sections:

* Assignment Title: A descriptive title for the assignment.
* Assignment Short ID: A unique identifier for the assignment, typically in parentheses.
* Goal: A clear statement of what the assignment aims to achieve.
* Acceptance Criteria: A list of conditions that must be met for the assignment to be considered complete.
* Assignment: A detailed description of the assignment, including any specific tasks or actions required.
* Detailed Steps: A step-by-step guide on how to complete the assignment, including any specific instructions or considerations.
* Completion: Instructions on how to finalize the assignment, including any follow-up actions or confirmations needed.

Currently, the following workflow assignment definition files are available:

* [pr-approval-and-merge.md](ai-workflow-assignments/pr-approval-and-merge.md): This assignment involves resolving all pull request (PR) comments, getting the PR approved, and merging it upstream.
* [perform-task.md](ai-workflow-assignments/perform-task.md): This assignment involves executing a specific task given to you by an orchestrator or stakeholder, using the task-based workflow process.  This includes understanding the task requirements, planning/gaining approval and gathering necessary resources, and delivering the expected outcome.
* [continue-task-work.md](ai-workflow-assignments/continue-task-work.md): This assignment provides a comprehensive guide on how to complete previously-existing tasks that have already been started but not yet finished. It includes guidance on how to determine which task to start, how to perform tasks within the task-based workflow, including detailed steps and considerations.
* [create-application.md](ai-workflow-assignments/create-application.md): This assignment involves creating a new application given the description from a new app template that has been filled out, including setting up the project structure, configuring necessary components, and ensuring the application meets specified requirements.
* [create-test-cases.md](ai-workflow-assignments/create-test-cases.md): This assignment involves creating test cases for the application, ensuring that all critical functionality is covered and that the tests are properly integrated into the build process. You will create a plan, document it in an issue, and once gaining approval, you will implement the plan. You will also create (or upgrade, if necessary) an auomated test pipeline.
* [pr-review-comments.md](ai-workflow-assignments/pr-review-comments.md): This assignment involves reviewing and addressing comments on a pull request (PR). You will systematically resolve each comment, commit changes, reply with a description of changes, and mark the comment reolved. You need to ensure the PR is ready for approval and merge.
* [create-test-cases.md](create-test-cases.md): This assignment involves creating test cases for the application, ensuring that all critical functionality is covered and that the tests are properly integrated into the build process. You will create a plan, document it in an issue, and once gaining approval, you will implement the plan. You will also create (or upgrade, if necessary) an auomated test pipeline.
