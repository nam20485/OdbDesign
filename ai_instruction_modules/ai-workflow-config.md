# AI Workflow Configuration for AI Tools

This configuration defines the preferred AI interaction patterns and tool usage for the AI Tools workspace.

## Core AI Tools - Default Usage

### Sequential Thinking Tool
**Use by default for:**
- Complex problem analysis and breakdown
- Multi-step development planning
- Debugging and troubleshooting workflows
- Architecture and design decisions
- Code refactoring strategies
- Performance optimization planning
- Security analysis and implementation planning

**Usage Pattern:**
```
When encountering tasks that involve:
1. Multiple steps or phases
2. Decision trees or branching logic
3. Analysis requiring deep reasoning
4. Planning with potential revisions
5. Complex problem-solving scenarios

→ ALWAYS use sequential thinking to break down the approach
```

### Memory Tools
**Use by default for:**
- Tracking project progress and context
- Maintaining component relationships and dependencies
- Recording user preferences and coding standards
- Documenting decisions and their rationales
- Tracking recurring issues and solutions
- Managing workspace knowledge and history

**Usage Pattern:**
```
For ANY significant interaction:
1. Check existing memory for relevant context
2. Update memory with new insights or progress
3. Create entities for new components/concepts
4. Link related information through relations
```

## Technology-Specific Guidelines

### ASP.NET Core Development
- Use sequential thinking for API design and endpoint planning
- Track component relationships in memory (controllers, services, models)
- Document dependency injection patterns and configurations
- Maintain memory of middleware pipeline decisions

### Docker & Containerization
- Use sequential thinking for multi-stage build optimization
- Track container dependencies and configurations in memory
- Document deployment patterns and environment-specific settings

### Google Cloud Platform
- Use sequential thinking for service architecture planning
- Maintain memory of resource configurations and relationships
- Track deployment patterns and infrastructure decisions

## Workflow Templates

### New Feature Development
1. **Sequential Thinking**: Break down feature requirements and implementation steps
2. **Memory Check**: Review existing related components and patterns
3. **Implementation**: Apply established patterns with incremental validation
4. **Memory Update**: Record new patterns, decisions, and lessons learned

### Debugging & Troubleshooting
1. **Sequential Thinking**: Analyze symptoms, hypothesize causes, plan investigation
2. **Memory Search**: Look for similar issues and solutions from project history
3. **Investigation**: Follow systematic debugging approach
4. **Memory Update**: Record root cause, solution, and prevention strategies

### Code Review & Refactoring
1. **Sequential Thinking**: Analyze current code, identify improvement opportunities
2. **Memory Review**: Check established patterns and architectural decisions
3. **Planning**: Create step-by-step refactoring approach
4. **Memory Update**: Record refactoring patterns and their outcomes

## Automation Triggers

### Always Use Sequential Thinking When:
- User asks "how to" or "what's the best way"
- Multiple approaches are possible
- Task involves 3+ logical steps
- Planning is required before implementation
- Analysis of pros/cons is needed
- Troubleshooting complex issues

### Always Use Memory When:
- Starting any development session
- User mentions project components or history
- Making architectural decisions
- Implementing patterns used elsewhere in project
- User provides preferences or feedback
- Completing significant milestones

### Use Both Tools When:
- Planning major features or refactoring
- Analyzing complex system interactions
- Designing new components or services
- Investigating cross-cutting concerns
- Making technology stack decisions

## Quality Assurance

### Before Providing Solutions:
1. ✅ Have I used sequential thinking to break down the problem?
2. ✅ Have I checked memory for relevant context and patterns?
3. ✅ Have I considered the ASP.NET Core + Google Cloud context?
4. ✅ Will I update memory with new insights or decisions?

### After Completing Tasks:
1. ✅ Update memory with outcomes and lessons learned
2. ✅ Record any new patterns or best practices discovered
3. ✅ Document any user preferences or feedback provided
4. ✅ Link new knowledge to existing project context

## Tool Synergy Patterns

### Effective Combinations:
- **Sequential Thinking → Memory Update**: Plan approach, then record decisions
- **Memory Search → Sequential Thinking**: Gather context, then plan detailed approach
- **Sequential Thinking ↔ Memory**: Iterative refinement of understanding and planning
- **Memory Entities + Relations**: Build comprehensive project knowledge graph

### Integration Guidelines:
- Start complex tasks with memory search for context
- Use sequential thinking to plan approaches systematically
- Update memory throughout the process, not just at the end
- Create entities for all significant project components
- Link related concepts through meaningful relations

## Success Metrics

The AI interaction should demonstrate:
- **Consistency**: Similar problems approached with similar systematic methods
- **Context Awareness**: Decisions that consider project history and established patterns
- **Learning**: Improved responses based on accumulated project knowledge
- **Systematic Thinking**: Complex problems broken down methodically
- **Knowledge Retention**: Project insights preserved and leveraged over time

---

*This configuration ensures that AI interactions with the AgentAsAService project are systematic, context-aware, and build upon accumulated knowledge for optimal development outcomes.*
