# Assignment: Pull Request (PR) Approval and Merge

## (pr-approval-and-merge)

### Goal

Resolve all PR comments and get the PR approved

### Acceptance Criteria

1. All PR comments resolved.
2. PR approved.
3. Branch merged upstream.
4. Issue closed.

### Assignment

In this stage your assignment will be to iterate on the PR comments until they
are all resolved and the PR is approved. Once PR comments are available
you will work systematically to resolve each comment, one after another.

**Always follow instructions exactly as listed here.**

It is important to the final quality of our product for everyone to perform their assignment exactly as specified.

### Detailed Steps

1. Fetch and analyze all of the review comments for the assigned PR.
2. Create an issue that lists each issue with a link, a description of the issue and its cause, and then your plan to resolve.
3. After gaining an approval, create a branch off of the PR's base branch, link it to your issue, start a review, and then being resolving each issue systematically in order. After finishing resolution of each issue, commit your changes, update the issue, and reply with a comment explaining the outcome on the original review comment. Then mark the comment resolved.

1. For each unresolved, non-outdated review comment:

    1. If you have already submitted a reply with a plan, then you must wait for the stake holder to review your plan and appro it before you can move any further to implement the changes.
        1. Move on to the next comment.
    1. If you have not yet submitted a reply, then you will need to review and address the comment.
        1. This involves reading the comment and the entire context of this comment to gain a full understanding.
        1. Including:
            1. All the replies in this comment's thread
            1. The PR's original code changes
            1. Any code changes or plans resulting from previous iterations on this comment.
            1. Have you already submitted any plans that were rejected?
            1. At this point, go read the issue and PR again
            1. Given the context determine the options to resolve the comment.

            You must then leave a reply explaining the plan, and/or different options for them to choose from.
            1. Using the original comment from the reviewer, reply to that comment so that the replies are all in the original thread.
            2. If you have a recommendation, then state so and why.
            3. Ask for approval, direction, or other required input/feedback to proceed.
            4. Move on the next comment.
            5. If you are unsure how to proceed, then ask for help in the chat or reply.
            6. If the stake-holder reply contains approval for a previously submitted plan, then you will be able to implement the changes now.
            7. If the stake-holder reply contains a request for changes, then you will need to review the comment and ensure that you understand the feedback provided.
            8. Address the feedback and update your implementation accordingly.
            9. Communicate your changes and seek further clarification if needed.

Once you have addressed all of the comments to the degree with which you can for this round, then notify the orchestrator and reviewer that you are ready for them to look at your replies and then wait for them to noify you again.

**After implementing each approved plan, leave a comment reply to the preceding approval comment informing stakeholders that you have completed implementing the indicated plan.** Include details of what was changed and confirm the implementation status.

* Iterate in this manner until all comments and replies have been addressed.
* Once all PR comments have been resolved, the stake-holder will approve the PR.

### ⚠️ CRITICAL: Commit Changes Before Merge

**BEFORE the PR is merged upstream, you MUST:**

1. **Commit all local changes** to the PR branch using `git add .` and `git commit`
2. **Push changes** to the remote branch using `git push origin <branch-name>`
3. **Verify changes are in the remote branch** by checking the PR diff or using `git show origin/<branch-name>:<file-path>`

**Failure to commit and push changes will result in permanent loss of all implementation work when the branch is merged and deleted.**

### Completion

Ask the stake-holder if they want you to merge the PR or if they would like to do so themselves.
If so, then merge the PR, and close the issue and branch.
Eat a cookie and have an espresso or a milkshake. You can have both if you want.
Ask the stake-holder for your next assignment.
