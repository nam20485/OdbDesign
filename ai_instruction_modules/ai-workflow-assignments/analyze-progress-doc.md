# Assignment: Analyze Progress Document

## (analyze-progress-doc)

### Goal

Rigorously interrogate the provided repository to analyze the progress made and identify any gaps or areas that need further work. Determine what exactly has been completed and what is still needed. If an issue is provided, then relate the findings using the issue as the basis. Document your findings in a new issue.

### Acceptance Criteria

1. New issue created documenting findings with clear analysis and recommendations.
2. Entire codebase analyzed using systematic approach and appropriate tools.
3. All branches and recent commits reviewed for progress assessment.
4. Gap analysis completed with prioritized recommendations.
5. Stakeholder approval obtained for the analysis and recommendations.

### Assignment

You will analyze the provided repository (current workspace or specified repository) to determine the progress made and identify any gaps or areas that need further work. You will then document your findings in a new issue. If you are provided an issue, then you will use that as the basis for your analysis.

**Always follow instructions exactly as listed here.**

It is important to the final quality of our product for everyone to perform their assignment exactly as specified.

### Detailed Steps

1. Determine what application is supposed to be being built
   1. **Start by reading the issue if provided.**
   2. Also read the [docs/ai-new-app-template.md](../../docs/ai-new-app-template.md) and included files.

   **Once you know what the specification of the application is supposed to be, you can analyze how much of that is complete, and what is still missing.**
   
   **Conduct systematic repository analysis using all available tools and information:**
   - Use `semantic_search` and `grep_search` for comprehensive code analysis
   - Use `mcp_github_list_issues` to review all existing issues
   - Use `mcp_github_search_pull_requests` to analyze pull request history
   - Use `git log --oneline --graph --all` to examine commit history and branches
   - Use `read_file` to examine key documentation and configuration files
   - Review project structure, dependencies, and build configurations
   - Examine any workflow runs that exist

   **⚠️ CRITICAL: VERIFICATION REQUIREMENTS**
   - **DO NOT accept statements in issues, PRs, or documentation as factual without verification**
   - Base your analysis ONLY on what you can verify in the actual codebase, commits, and branches
   - Your job is to perform an independent investigation and establish the definitive factual state of the repository
   - You must evaluate all claims yourself and provide verifiable evidence for every assertion you make
   - Include a dedicated "Evidence Sources" section documenting exactly where each finding was discovered

2. **Analyze branch status and recent activity:**
   - Find the branch with the latest commits using `git branch -v --sort=-committerdate`
   - Identify the branch with the most work completed
   - Compare branches to understand development flow and current state

3. **If an issue was provided, perform targeted analysis:**
   - Detail the progress made against each item in the original issue
   - Identify any gaps or areas that need further work based on what was outlined
   - Cross-reference issue requirements with actual implementation

4. **Create comprehensive findings documentation:**
   - Document all completed work with evidence (commits, files, features)
   - Identify specific gaps and missing components
   - Prioritize recommendations by importance and effort required
   - Include risk assessment for incomplete items

5. **Create a new issue documenting your findings:**
   - Use clear, structured format with sections for completed work, gaps, and recommendations
   - If original issue provided, follow its structure and format
   - Include checkboxes: checked for completed work, unchecked for remaining tasks
   - Include original parts of the old issue, then add new sections for your findings
   - **Must include "Evidence Sources" section** documenting where each finding was discovered

6. **Link and reference appropriately:**
   - If original issue provided, link the new issue using GitHub references (e.g., "Related to #123")
   - Reference specific commits, files, and branches in your analysis
   - Include links to relevant documentation or external resources

7.  **Provide evidence and methodology:**
   - List all tools and sources used to make your determination
   - Include specific commands run and files analyzed
   - Document the branch with latest commits and the branch with most work completed
   - Include any assumptions or limitations in your analysis

8.  **Quality validation:**
   - Review your analysis for completeness and accuracy
   - Ensure all major project components have been examined
   - Verify that recommendations are actionable and specific
   - Confirm that the new issue follows proper formatting and standards

### Completion

1. **Present findings to stakeholder:**
   - Inform the stakeholder that the analysis is complete
   - Present the new issue with issue number and summary
   - Provide direct link to the issue for review

2. **Stakeholder review process:**
   - Ask if they approve the analysis and recommendations
   - Address any questions or requests for clarification
   - Make revisions if needed based on feedback

3. **Finalization:**
   - Mark the analysis as complete once approved
   - Update any related project tracking or documentation
   - Prepare for next steps based on the identified gaps and recommendations
