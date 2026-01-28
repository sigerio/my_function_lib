---
name: OpenSpec: Apply
description: Implement an approved OpenSpec change and keep tasks in sync.
category: OpenSpec
tags: [openspec, apply]
---
<!-- OPENSPEC:START -->
**防护措施**
- 优先采用简单、最小化的实现，只有在明确要求或必要时才增加复杂性。
- 将变更范围严格限制在请求的结果内。
- 如需额外的 OpenSpec 约定或说明，请参阅 `openspec/AGENTS.md`（位于 `openspec/` 目录内——如果找不到，请运行 `ls openspec` 或 `openspec update`）。

**步骤**
将这些步骤作为待办事项跟踪，逐一完成。
1. 阅读宪章，所有设计均依照宪章要求。
2. 阅读 `changes/<id>/proposal.md`、`design.md`（如存在）和 `tasks.md` 以确认范围和验收标准。
3. 分类任务，创建里程碑节点，每个里程碑节点完成后向我描述当前完成的内容，并询问是否需要功能验证、代码审核等。
4. 按顺序完成任务，保持编辑最小化并专注于请求的变更。
5. 在更新状态之前确认完成——确保 `tasks.md` 中的每个项目都已完成。
6. 所有工作完成后更新检查清单，将每个任务标记为 `- [x]` 以反映实际情况。
7. 需要额外上下文时参考 `openspec list` 或 `openspec show <item>`。

**参考**
- 如果在实施过程中需要提案的额外上下文，使用 `openspec show <id> --json --deltas-only`。
- 完成后审核是否符合宪章规范，是否符合AGENTS.md规范，是否正确按照AGENTS.md进行代码审核
<!-- OPENSPEC:END -->
