# Assignment: Continue Existing Tasks

## (continue-task-work)

### Goal

Complete previously-existing tasks that have already been started but not yet finished. This includes guidance on how to determine which task to start, how to perform tasks within the task-based workflow, including detailed steps and considerations.

### Input

1. A git repository
2. list of issues

### Acceptance Criteria

1. All existing incomplete tasks have been identified and prioritized
2. Current progress and status of each task has been assessed
3. Most appropriate task has been selected for continuation
4. Task has been resumed from the correct point in the workflow
5. All previous work has been preserved and built upon
6. Task has been completed successfully following established workflows
7. All related issues, branches, and documentation have been updated appropriately

### Assignment

Your assignment is to identify and complete tasks that have been previously started but remain incomplete. This involves systematic assessment of existing work, proper continuation of established workflows, and successful completion of outstanding tasks.

**Always follow instructions exactly as listed here.**

It is important to the final quality of our product for everyone to perform their assignment exactly as specified.

### Detailed Steps

1. **Task Discovery and Assessment**
   1. Check GitHub Issues for items assigned to you using: `gh issue list --label assigned:copilot`
   2. Look for issues with `state:in-progress` label to identify active tasks
   3. Review any open pull requests that may be related to your work
   4. Check for any local branches that contain uncommitted or unpushed work
   5. Identify any tasks mentioned in previous conversation history or documentation

2. **Current State Analysis**
   1. For each identified task, thoroughly review the GitHub issue:
      - Read the original description and acceptance criteria
      - Check which checkboxes have been completed
      - Review all comments and updates made
      - Understand the current progress and status
   2. Examine any related branches:
      - Check what changes have been made
      - Verify if changes are committed and pushed
      - Assess the quality and completeness of existing work
   3. Identify any merge conflicts or integration issues that need resolution

3. **Task Prioritization and Selection**
   1. Assess the priority and urgency of each incomplete task
   2. Consider dependencies between tasks and their impact on other work
   3. Evaluate your current capacity and the effort required for each task
   4. Select the most appropriate task to continue based on:
      - Priority and business value
      - Proximity to completion
      - Availability of required resources
      - Stakeholder expectations and deadlines

4. **Workflow Continuation Setup**
   1. Ensure you have the correct branch checked out for the selected task
   2. Pull any remote changes: `git pull origin <branch-name>`
   3. Check for and resolve any merge conflicts with upstream changes
   4. Verify that your local environment is properly configured
   5. Review the task's issue to understand exactly where to resume

5. **Progress Assessment and Planning**
   1. Determine exactly what work remains to be completed
   2. Identify any changes in requirements or scope since the task was started
   3. Assess if the original plan is still valid or needs modification
   4. Update the implementation plan if necessary and seek approval for changes
   5. Communicate with stakeholders about resuming work on the task

6. **Task Execution and Completion**
   1. Continue working through the remaining checkboxes in the issue
   2. Update progress by checking off completed items as you finish them
   3. Follow all established coding and documentation standards
   4. Test changes thoroughly, especially integration with existing work
   5. Maintain clear commit messages and regular progress updates

7. **Integration and Quality Assurance**
   1. Ensure all changes integrate properly with existing codebase
   2. Run comprehensive tests (unit, integration, manual) on the complete solution
   3. Verify that all acceptance criteria are met
   4. Address any issues or conflicts that arise during integration
   5. Update documentation to reflect any changes or additions made

8. **Stakeholder Communication and Review**
   1. Notify stakeholders that the task has been resumed and provide status update
   2. Share any findings or issues discovered during the continuation process
   3. Request review of completed work when appropriate
   4. Address any feedback or change requests promptly
   5. Confirm that the final deliverables meet expectations

### Completion

1. **Final Validation and Testing**
   1. Perform a comprehensive final review of all completed work
   2. Ensure all changes are properly committed and pushed
   3. Verify that all acceptance criteria from the original issue are satisfied
   4. Run final tests to confirm everything works as expected
   5. Update any related documentation or configuration files

2. **Issue and Branch Management**
   1. Update the GitHub issue with a final summary of completed work
   2. Remove the `state:in-progress` label from the completed issue
   3. Create pull request if not already created, following PR requirements
   4. Assign appropriate reviewers (nam20485 and /gemini)
   5. Link the PR to the issue for automatic closure

3. **Handoff and Next Steps**
   1. Notify stakeholders that the task has been completed
   2. Provide any necessary documentation or instructions for using the deliverables
   3. Clean up any temporary resources or branches as appropriate
   4. Update any project tracking or status systems
   5. Gather feedback and lessons learned for future task continuation

Ask the orchestrator for your next assignment once the existing task has been successfully completed and all related issues have been resolved.
