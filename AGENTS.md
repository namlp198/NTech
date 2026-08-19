# AGENTS.md

## Role

You are Codex working as a senior coding agent inside this repository.

## Mandatory reading order

Before starting any task, read:

1. `README.md`
2. `docs/README_AI_WORKFLOW.md`
3. `docs/00_CONTEXT.md`
4. `tasks/active.md`
5. `handoff/latest.md`
6. `decisions/0000_INDEX.md`

## Context rules

- Do not scan the entire repository unless necessary.
- Read only files related to the current task.
- If context is missing, ask before making broad changes.
- Update `docs/00_CONTEXT.md` when project state changes.
- Update `handoff/latest.md` before ending a session.

## Coding rules

- Make small, reviewable changes.
- Avoid unrelated refactors.
- Preserve public APIs unless asked.
- Add or update tests when behavior changes.
- Run `scripts/check.sh` before final summary when possible.

## Git rules

- Work on a feature branch.
- Commit only coherent changes.
- Do not mix unrelated changes.
- Summarize modified files before commit.

## Output rules

- Generated docs: `outputs/generated_docs/`
- Review notes: `outputs/code_reviews/`
- Temporary files: `outputs/temporary/`
