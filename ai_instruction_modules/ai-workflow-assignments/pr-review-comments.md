# Assignment: Pull Request (PR) Review Comments

## (pr-review-comments)

### 🚨 CRITICAL REMINDER - INDIVIDUAL REPLIES REQUIRED 🚨

**YOU MUST REPLY TO EACH REVIEW COMMENT INDIVIDUALLY!**

- **NEVER** post one general PR comment
- **ALWAYS** reply to each individual review comment (i.e. the comment's thread) using the GitHub API
- Each comment gets its own separate reply using: `gh api -X POST repos/owner/repo/pulls/number/comments/{comment_id}/replies`
- This is a FUNDAMENTAL requirement - individual comment engagement is MANDATORY

### Goal

Resolve **ALL (100%)** PR comments and get the PR approved. You must ensure that every comment is addressed thoroughly.

### Acceptance Criteria

2. PR Review started.
1. All PR comments resolved by fixing the respective issue described.
3. All code changes made to resolve PR comments.
4. All code chages committed to the PR branch. (**DON"T FORGET TO CHECKOUT THE PR BRANCH (BEFORE MAKING ANY CHANGES)!**)
4. Comment added to PR describing each review comment you fixed and how.
5. All PR Review Comment Threads marked as resolved.

### Assignment

You will provide a fix for each PR Review commnt, commit the changes, reply to the comment with a description of the issue and trhe changes you made to fix it, and then mark  it resolved. You will iterate on all of the comments doing this, until all PR Review Comments are resolved in this manner.

**Always follow instructions exactly as listed here.**

It is important to the final quality of our product for everyone to perform their assignment exactly as specified.

### Detailed Steps

- Before starting the resolution process, you will need to review all comments and understand the issues raised.
- Before making any changes, you will checkout the correct PR branch. All your changes will go here.
- When PR comments are available you will work systematically to resolve each comment, one after another.
- In this stage your assignment will be to iterate on the PR comments until they are all commented on and marked resolved.
- For each review comment, you will:
    - understand the comment
    - create a plan to resolve the comment
    - make the code changes
    - commit the code changes (reference the PR & comment)
    - **leave a reply to THAT SPECIFIC comment explaining how you resolved it** 
    - marked comment reply thread resolved.
Once finished, you will leave a comment on the PR summarizing the changes made and the comments resolved.

**AFTER FINISHING THE FIX FOR EACH COMMENT, DO NOT FORGET TO COMMIT YOUR CHANGE, REPLY TO EACH WITH A COMMENT DESCRIBING YOUR FIX, AND THEN MARKING IT RESOLVED!**

**🚨 REMINDER: EACH COMMENT GETS ITS OWN INDIVIDUAL REPLY - NO GENERAL COMMENTS! 🚨**

### Technical Implementation: Individual Review Comment Replies

**🚨 CRITICAL WARNING: INDIVIDUAL REPLIES ONLY! 🚨**

**YOU MUST REPLY TO EACH REVIEW COMMENT INDIVIDUALLY - NOT ONE GENERAL COMMENT!**

**CRITICAL**: You must reply to **INDIVIDUAL REVIEW COMMENTS** using the specific GitHub API endpoint, not add general comments to the PR thread.

**WRONG APPROACH**: Posting one general comment explaining all fixes ❌
**CORRECT APPROACH**: Individual reply to each specific review comment ✅

#### Correct Method - Individual Comment Replies

Use this **exact** GitHub API endpoint to reply to each review comment individually:

```bash
gh api -X POST repos/{owner}/{repo}/pulls/{pull_number}/comments/{comment_id}/replies -f body="Your reply message"
```

**Example**:
```bash
gh api -X POST repos/nam20485/pr-batch-comment-tool/pulls/63/comments/2246817149/replies -f body="✅ **Fixed in commit dc9441c**

**Issue**: Description of the issue identified in the review comment.

**Resolution**: Detailed explanation of how you fixed it with code examples.

**Benefits**: What this fix accomplishes."
```

#### Getting Comment IDs

First, get all review comments to find their IDs:
```bash
gh api repos/{owner}/{repo}/pulls/{pull_number}/comments | ConvertFrom-Json | Select-Object id, @{Name='body_preview'; Expression={$_.body.Substring(0, [Math]::Min(100, $_.body.Length))}}
```

#### Reply Format Template

Use this format for each individual reply:
```markdown
✅ **Fixed in commit {commit_hash}**

**Issue**: {Brief description of the problem identified}

**Resolution**: {Detailed explanation of your fix with code examples}

**Benefits**: {What this improvement accomplishes}
```

#### **IMPORTANT**: Do NOT Use General PR Comments

❌ **NEVER use these methods** - they add to the main PR thread, not individual comments:
- `gh pr comment {pr_number} --body "message"`
- `gh api repos/{owner}/{repo}/issues/{issue_number}/comments`

✅ **ALWAYS use this method** - replies to the specific review comment:
- `gh api repos/{owner}/{repo}/pulls/{pull_number}/comments/{comment_id}/replies`

### Completion

Inform the stake-holder that the PR is ready for review and approval. Show them the comment you added summarizing the changes made and the comments resolved. Ask if they approve or need more changes on any of the comments you reolved.

## Comment Resolution Method

### Individual Comment Thread Resolution Using GraphQL API

To mark individual PR review comments as resolved, use GitHub's GraphQL API with the `resolveReviewThread` mutation:

1. **Get Thread ID**: First, retrieve the thread ID for the comment using GraphQL:
   ```bash
   gh api graphql -F query='
     query($owner: String!, $repo: String!, $number: Int!) {
       repository(owner: $owner, name: $repo) {
         pullRequest(number: $number) {
           reviewThreads(first: 20) {
             nodes {
               id
               isResolved
               comments(first: 1) {
                 nodes {
                   id
                   databaseId
                   body
                 }
               }
             }
           }
         }
       }
     }
   ' -F owner="nam20485" -F repo="pr-batch-comment-tool" -F number=63
   ```

2. **Resolve Thread**: Use the thread ID to resolve the comment:
   ```bash
   gh api graphql -F query='
     mutation($threadId: ID!) {
       resolveReviewThread(input: {threadId: $threadId}) {
         thread {
           id
           isResolved
           comments(first: 1) {
             nodes {
               id
               databaseId
             }
           }
         }
       }
     }
   ' -F threadId="PRRT_kwDOPUXqes5WSivI"
   ```

3. **Verification**: The response will show `"isResolved": true` when successful.

**Note**: The key insight is that GitHub's comment resolution works at the **thread level**, not individual comment level. Each review comment belongs to a thread, and resolving the thread marks all comments in that thread as resolved.
