# AI Workflow Roles

This module defines the ai instruction module workflow roles concept. When working in the repository and using the rest of the instructions modules, the actual and specific workflow you will follow will be defined by the role that is assigned to you.

## Goal
The new role-based workflow introduces a whole new ai instructions module files-based workflow. The new workflow will allow multiple agents to make changes to implement an entire project, or make changes to an exsiting project. Agents will asume one of two or more roles: orchestrators or collaborator team members. There's one orchestrator per project who will orchestrate, direct, and organize e a team of one or more collaborator agents. The goal is that these agents can run autonomously effectively in order to efficiently achieve their objectives correctly.

When operating with little to no supervision, LLM-based agents function most effectively when the instructions they are given are straightforward, specific, and unambiguous. Care must be taken to break up their given tasks and deconstruct them into smaller modular chunks.

## AI Instruction Modules
The instruction modules files define how everyone, in all roles, behave and perform their work in general. Instructions found in your role definition explain what you are expected to do specifically in your type of role.

## Role Assignment
You will be assigned your role at the beginning of your session. If you make it this far and do not know your role yet, you need to ask for your role assignment at this point before going any further.

## Role Definition and Guidelines
Once assigned a role you need to go find that role definition role file and read it. Role definition files can be found
in the ai-workflow-roles directory. Inside the directory you will find one sub-directory for each role, named by the name of the role. Inside each named role definition sub-directory you will find a file named: ai-workflow-role.md. Read this file to understand your role-specific definition and instructions. At the end of this you may 0 or more other files linked to support that roles instructions. If given you will need to read all these files as well.

```plaintext
<repo root directory>/
    ai_instructions_modules/
        ai-workflow-roles/
            *<role_name_subdir>*/  // e.g. collaborator" or "orchestrator"
                ai-workflow-role.md
                *<any more linked files>.md*
```                

## Expectations in your Role
Once you have finished reading your role definition instructions you will be expected to start performing your role's workflow.

* If you don't understand, need clarification, have questions, or see any problems make sure to ask until you achieve sufficient clarity.
* Before starting, analyze your current repository and present a short outline of what your instructions specify when applied in this context.
* Ask for any specific instructions user would like to add.
* Ask for approval of your plan.
* Upon approval, begin executing your workflow.
* When finished, notify user and present a summary to the user.
