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

## Git workflow (bắt buộc — chuẩn tại 06_Repositories/_standards/GIT-WORKFLOW.md)

- Nhánh: `main` (trunk ổn định), `develop`, `feature/*`, `fix/*`, `release/x.y`, `hotfix/x.y-*`.
- **Chỉ làm việc trên `develop`, `feature/*`, `fix/*`, `hotfix/*`.** Tuyệt đối không commit, push, merge cục bộ hay tag trên `main`/`release/*`. Nếu đang đứng ở nhánh đó, chuyển sang `develop` trước khi sửa file.
- Tag SemVer `vX.Y.Z` (firmware: `att-vX.Y.Z`, `sff-vX.Y.Z`, `mff-vX.Y.Z`), chỉ trên `release/*`.
- Nhánh AI tạo phải đặt tên `feature/*`, không dùng `claude/*`, `codex/*`.
- Đầu phiên: chạy `pwsh/powershell ..\_standards\scripts\Get-RepoSyncStatus.ps1 -Repo <tên repo>`; nếu `develop` có commit từ ngày trước chưa vào `main`, nhắc người dùng.
- Cuối phiên (hoặc khi task xong): nếu `develop` đi trước `main` và đủ điều kiện (tree sạch, đã push, CI xanh, không xung đột) thì **hỏi** người dùng có merge `develop → main` (qua PR) không. Không tự merge khi chưa được đồng ý trong phiên đó.
