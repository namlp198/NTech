# README_AI_WORKFLOW

## Purpose

This file captures the repo-local AI workflow context without modifying the repository's main `README.md`.

## Repository Identity

- Repository: `NTech`
- Project notes: `05_Projects/NTech/nvision/`
- Bootstrapped on: `2026-08-19`
- Branch at bootstrap: `codex/nvision-foundation`

## Read Before Changing Code

1. `README.md`
2. `docs/00_CONTEXT.md`
3. `tasks/active.md`
4. `handoff/latest.md`
5. `decisions/0000_INDEX.md`
6. `05_Projects/NTech/nvision/session_history.md` if you need more than the latest handoff

## Expected AI Workflow

- Read only the files needed for the current task.
- Keep changes small and reviewable.
- Update `tasks/active.md` when the current task changes.
- Update `handoff/latest.md` before ending the session.
- Update `docs/00_CONTEXT.md` when project state materially changes.

## Session Closeout

Preferred command after finishing a work session:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\end-session.ps1 `
  -SessionSummary "Short summary of the completed work" `
  -CurrentStatus "Current repository/project state" `
  -NextActions "Next step 1","Next step 2" `
  -ToolName "Codex" `
  -SessionMode "Coding"
```

If the repo is a company repo and mirror sync was configured in:

```text
tools/ai-workspace.repo-config.template.json
tools/ai-workspace.repo-config.local.json
```

then add:

```powershell
-SyncMirror
```

Mirror sync hiá»‡n Ä‘Æ°á»£c thiáº¿t káº¿ cho tráº¡ng thÃ¡i repo Ä‘Ã£ commit xong pháº§n code chÃ­nh.

If you also want the closeout metadata to be committed automatically, add:

```powershell
-GitCommitCloseout
```

The end-session flow also refreshes:

```text
08_Sync_And_Handoff/repository_mirror_map.md
```

when `backupRepoPath` is configured.

The same closeout flow also appends a summarized entry to:

```text
05_Projects/NTech/nvision/session_history.md
```

This is a session journal, not a full raw chat transcript.

## Output Locations

- Codex and Claude review notes: `outputs/code_reviews/`
- Generated documents: `outputs/generated_docs/`
- Temporary files: `outputs/temporary/`

## Notes

- Keep the repository's main `README.md` focused on the project itself.
- Keep this file focused on AI-assisted workflow inside the repository.
