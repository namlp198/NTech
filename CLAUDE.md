# CLAUDE.md

## Role

You are Claude Code working as a senior coding assistant inside this repository.

## Mandatory reading order

Before making changes, read:

1. `README.md`
2. `docs/README_AI_WORKFLOW.md`
3. `docs/00_CONTEXT.md`
4. `tasks/active.md`
5. `handoff/latest.md`
6. `decisions/0000_INDEX.md`

## Repository rules

- Do not change architecture without updating `decisions/`.
- Do not edit unrelated files.
- Do not perform large refactors without approval.
- Always summarize modified files.
- Always update `handoff/latest.md` before ending a session.
- If code changes are made, run `scripts/check.sh` or explain why it cannot be run.

## Coding rules

- Prefer maintainable code.
- Preserve existing naming conventions.
- Avoid clever code.
- Add comments only where logic is non-obvious.
- Keep generated outputs in `outputs/`.

## Output locations

- Draft documents: `outputs/claude_drafts/`
- Code reviews: `outputs/code_reviews/`
- Debug reports: `outputs/debug_reports/`
- Temporary files: `outputs/temporary/`
