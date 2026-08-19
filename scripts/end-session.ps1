[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SessionSummary,

    [string]$CurrentStatus = "",

    [string[]]$NextActions = @(),

    [string[]]$Warnings = @(),

    [string]$BuildStatus = "Not run",

    [string]$UnitTestStatus = "Not run",

    [string]$ManualTestStatus = "Not run",

    [string]$RepoContextNote = "",

    [string]$ProjectContextNote = "",

    [string]$DecisionTitle = "",

    [string]$DecisionStatus = "Proposed",

    [string]$DecisionId = "",

    [string]$ProjectName = "",

    [string]$ToolName = "Codex",

    [string]$SessionMode = "Coding",

    [string]$ThreadReference = "",

    [switch]$SyncMirror,

    [switch]$GitCommitCloseout
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return [System.IO.Path]::GetFullPath($PathValue)
    }

    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $PathValue))
}

function Ensure-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if (-not (Test-Path -LiteralPath $PathValue)) {
        New-Item -ItemType Directory -Path $PathValue | Out-Null
    }
}

function Read-FileUtf8 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    Get-Content -LiteralPath $PathValue -Raw
}

function Read-JsonFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if (-not (Test-Path -LiteralPath $PathValue)) {
        return $null
    }

    Get-Content -LiteralPath $PathValue -Raw | ConvertFrom-Json
}

function Get-ConfigValue {
    param(
        [Parameter(Mandatory = $true)]
        $PrimaryConfig,

        [Parameter(Mandatory = $true)]
        $FallbackConfig,

        [Parameter(Mandatory = $true)]
        [string]$PropertyName,

        $DefaultValue = $null
    )

    if ($PrimaryConfig -and ($PrimaryConfig.PSObject.Properties.Name -contains $PropertyName)) {
        $primaryValue = $PrimaryConfig.$PropertyName
        if ($null -ne $primaryValue -and "$primaryValue" -ne "") {
            return $primaryValue
        }
    }

    if ($FallbackConfig -and ($FallbackConfig.PSObject.Properties.Name -contains $PropertyName)) {
        $fallbackValue = $FallbackConfig.$PropertyName
        if ($null -ne $fallbackValue -and "$fallbackValue" -ne "") {
            return $fallbackValue
        }
    }

    return $DefaultValue
}

function Write-FileUtf8 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,

        [Parameter(Mandatory = $true)]
        [string]$Content
    )

    Set-Content -LiteralPath $PathValue -Value $Content -Encoding utf8
}

function Invoke-GitSafe {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $hasNativePreference = $null -ne (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue)
    if ($hasNativePreference) {
        $previousPreference = $PSNativeCommandUseErrorActionPreference
        $script:PSNativeCommandUseErrorActionPreference = $false
    }

    try {
        try {
            @(& git @Arguments 2>$null)
        }
        catch {
            @()
        }
    }
    finally {
        if ($hasNativePreference) {
            $script:PSNativeCommandUseErrorActionPreference = $previousPreference
        }
    }
}

function Invoke-GitStrict {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & git @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Git command failed: git $($Arguments -join ' ')"
    }
}

function Convert-ToMarkdownBullets {
    param(
        [AllowEmptyCollection()]
        [string[]]$Items,

        [string]$DefaultItem
    )

    if ($Items.Count -eq 0) {
        return "- $DefaultItem"
    }

    ($Items | ForEach-Object { "- $_" }) -join "`r`n"
}

function Normalize-StringList {
    param(
        [AllowEmptyCollection()]
        [string[]]$Items
    )

    if ($Items.Count -eq 1 -and $Items[0] -match ",") {
        return @(
            $Items[0].Split(",") |
            ForEach-Object { $_.Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
    }

    return @(
        $Items |
        ForEach-Object { $_.Trim() } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
}

function Convert-ToMarkdownNumbers {
    param(
        [AllowEmptyCollection()]
        [string[]]$Items,

        [string]$DefaultItem
    )

    if ($Items.Count -eq 0) {
        return "1. $DefaultItem"
    }

    $index = 1
    $lines = foreach ($item in $Items) {
        "$index. $item"
        $index++
    }
    $lines -join "`r`n"
}

function Set-MarkdownSection {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Content,

        [Parameter(Mandatory = $true)]
        [string]$Heading,

        [Parameter(Mandatory = $true)]
        [string]$Body
    )

    $normalizedContent = $Content.TrimEnd()
    $sectionPattern = "(?ms)^## " + [regex]::Escape($Heading) + "\s*\r?\n.*?(?=^## |\z)"
    $replacement = "## $Heading`r`n`r`n$Body"

    if ([regex]::IsMatch($normalizedContent, $sectionPattern)) {
        return [regex]::Replace($normalizedContent, $sectionPattern, $replacement)
    }

    return $normalizedContent + "`r`n`r`n" + $replacement + "`r`n"
}

function Update-MirrorMap {
    param(
        [Parameter(Mandatory = $true)]
        [string]$MirrorMapPath,

        [Parameter(Mandatory = $true)]
        [string]$ProjectNameValue,

        [Parameter(Mandatory = $true)]
        [string]$RepoNameValue,

        [Parameter(Mandatory = $true)]
        [string]$WorkRemoteValue,

        [Parameter(Mandatory = $true)]
        [string]$BranchNameValue,

        [Parameter(Mandatory = $true)]
        [string]$BackupRepoPathValue,

        [Parameter(Mandatory = $true)]
        [string]$BackupRemoteValue,

        [Parameter(Mandatory = $true)]
        [bool]$IsCompanyRepoValue
    )

    if (-not $BackupRepoPathValue) {
        return $false
    }

    $content = Read-FileUtf8 -PathValue $MirrorMapPath
    $lines = [System.Collections.Generic.List[string]]::new()
    foreach ($line in ($content -split "`r?`n")) {
        $lines.Add($line)
    }

    $statusValue = if ($IsCompanyRepoValue) { "Configured" } else { "Optional" }
    $row = "| $ProjectNameValue | AI-Workspace/06_Repositories/$RepoNameValue | $WorkRemoteValue | $BranchNameValue | $BackupRepoPathValue | $BackupRemoteValue | $BranchNameValue | $statusValue |"
    $projectPattern = '^\|\s*' + [regex]::Escape($ProjectNameValue) + '\s*\|'

    $updated = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match $projectPattern) {
            if ($lines[$i] -ne $row) {
                $lines[$i] = $row
                $updated = $true
            }
            else {
                $updated = $false
            }
            Write-FileUtf8 -PathValue $MirrorMapPath -Content (($lines -join "`r`n").TrimEnd() + "`r`n")
            return $updated
        }
    }

    $separatorIndex = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\|\-\-\-') {
            $separatorIndex = $i
            break
        }
    }

    if ($separatorIndex -ge 0) {
        $lines.Insert($separatorIndex + 1, $row)
    }
    else {
        if ($lines.Count -gt 0 -and $lines[$lines.Count - 1] -ne "") {
            $lines.Add("")
        }
        $lines.Add("| Project | Work Repo | Work Remote | Work Branch | Backup Repo | Backup Remote | Backup Branch | Status |")
        $lines.Add("|---|---|---|---|---|---|---|---|")
        $lines.Add($row)
    }

    Write-FileUtf8 -PathValue $MirrorMapPath -Content (($lines -join "`r`n").TrimEnd() + "`r`n")
    return $true
}

function Invoke-GitCommitForPaths {
    param(
        [Parameter(Mandatory = $true)]
        [string]$GitRoot,

        [Parameter(Mandatory = $true)]
        [string[]]$RelativePaths,

        [Parameter(Mandatory = $true)]
        [string]$CommitMessage,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $stagedBefore = @(Invoke-GitSafe -Arguments (@("-C", $GitRoot, "diff", "--cached", "--name-only") + @("--")))
    if ($stagedBefore.Count -gt 0) {
        Write-Warning "$Label already has staged changes. Skipping auto closeout commit to avoid mixing unrelated files."
        return $false
    }

    $statusForPaths = @(Invoke-GitSafe -Arguments (@("-C", $GitRoot, "status", "--porcelain", "--") + $RelativePaths))
    if ($statusForPaths.Count -eq 0) {
        Write-Host "$Label closeout commit: no file changes detected."
        return $false
    }

    Invoke-GitStrict -Arguments (@("-C", $GitRoot, "add", "--") + $RelativePaths)
    $stagedAfter = @(Invoke-GitSafe -Arguments (@("-C", $GitRoot, "diff", "--cached", "--name-only", "--") + $RelativePaths))
    if ($stagedAfter.Count -eq 0) {
        Write-Host "$Label closeout commit: nothing staged after add."
        return $false
    }

    Invoke-GitStrict -Arguments @("-C", $GitRoot, "commit", "-m", $CommitMessage)
    return $true
}

function Initialize-ProjectFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot,

        [Parameter(Mandatory = $true)]
        [string]$ProjectNameValue,

        [Parameter(Mandatory = $true)]
        [string]$RepoNameValue
    )

    $templateRoot = Join-Path $WorkspaceRoot "05_Projects\_PROJECT_TEMPLATE"
    $projectRoot = Join-Path $WorkspaceRoot ("05_Projects\" + $ProjectNameValue)

    Ensure-Directory -PathValue $projectRoot

    if (Test-Path -LiteralPath $templateRoot) {
        $templateFiles = Get-ChildItem -LiteralPath $templateRoot -File
        foreach ($file in $templateFiles) {
            $destinationPath = Join-Path $projectRoot $file.Name
            if (-not (Test-Path -LiteralPath $destinationPath)) {
                Copy-Item -LiteralPath $file.FullName -Destination $destinationPath
                $content = Read-FileUtf8 -PathValue $destinationPath
                $content = $content.Replace("<repo-name>", $RepoNameValue)
                Write-FileUtf8 -PathValue $destinationPath -Content $content
            }
        }
    }

    return $projectRoot
}

function Get-NextDecisionId {
    param(
        [Parameter(Mandatory = $true)]
        [string]$DecisionIndexContent
    )

    $matches = [regex]::Matches($DecisionIndexContent, "(?m)^\|\s*(\d{4})\s*\|")
    if ($matches.Count -eq 0) {
        return "0001"
    }

    $maxValue = 0
    foreach ($match in $matches) {
        $value = [int]$match.Groups[1].Value
        if ($value -gt $maxValue) {
            $maxValue = $value
        }
    }

    ($maxValue + 1).ToString("0000")
}

$scriptRoot = Resolve-FullPath -PathValue $PSScriptRoot
$repoRoot = Split-Path -Parent $scriptRoot
$repositoriesRoot = Split-Path -Parent $repoRoot
$workspaceRoot = Split-Path -Parent $repositoriesRoot
$repoName = Split-Path -Leaf $repoRoot
$now = Get-Date
$today = $now.ToString("yyyy-MM-dd")
$dateTimeLabel = $now.ToString("yyyy-MM-dd HH:mm")
$machineName = if ($env:COMPUTERNAME) { $env:COMPUTERNAME } else { "Unknown machine" }

$configTemplatePath = Join-Path $repoRoot "tools\ai-workspace.repo-config.template.json"
$configLocalPath = Join-Path $repoRoot "tools\ai-workspace.repo-config.local.json"
$configTemplate = Read-JsonFile -PathValue $configTemplatePath
$configLocal = Read-JsonFile -PathValue $configLocalPath

if (-not $ProjectName) {
    $configuredProjectName = Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "projectName" -DefaultValue ""
    if ($configuredProjectName) {
        $ProjectName = [string]$configuredProjectName
    }
    else {
        $ProjectName = $repoName
    }
}

$projectRoot = Initialize-ProjectFiles -WorkspaceRoot $workspaceRoot -ProjectNameValue $ProjectName -RepoNameValue $repoName

$branchOutput = @(Invoke-GitSafe -Arguments @("-C", $repoRoot, "branch", "--show-current"))
$branchName = if ($branchOutput.Count -gt 0) { ($branchOutput[0]).Trim() } else { "" }
if (-not $branchName) {
    $branchName = "unknown-branch"
}

$NextActions = Normalize-StringList -Items $NextActions
$Warnings = Normalize-StringList -Items $Warnings

$statusLines = @(Invoke-GitSafe -Arguments @("-C", $repoRoot, "status", "--short", "--untracked-files=all"))
$changedFiles = @()
if ($statusLines.Count -gt 0) {
    foreach ($line in $statusLines) {
        if (-not [string]::IsNullOrWhiteSpace($line)) {
            $trimmed = $line.Trim()
            if ($trimmed.Length -gt 3) {
                $changedFiles += $trimmed.Substring(3)
            }
        }
    }
}
else {
    $lastCommitFiles = @(Invoke-GitSafe -Arguments @("-C", $repoRoot, "show", "--pretty=", "--name-only", "HEAD"))
    $changedFiles = $lastCommitFiles | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}

$lastCommitOutput = @(Invoke-GitSafe -Arguments @("-C", $repoRoot, "log", "--oneline", "-1"))
$lastCommit = if ($lastCommitOutput.Count -gt 0) { ($lastCommitOutput[0]).Trim() } else { "" }
if (-not $lastCommit) {
    $lastCommit = "No commit found"
}

if ($SyncMirror) {
    if ((-not $configTemplate) -and (-not $configLocal)) {
        throw "Cannot sync mirror because both repo config files are missing."
    }

    $configuredIsCompanyRepo = [bool](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "isCompanyRepo" -DefaultValue $false)
    $configuredBackupRepoPath = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "backupRepoPath" -DefaultValue "")
    $configuredMirrorMode = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "mirrorMode" -DefaultValue "fetch-merge")
    $configuredBackupRemote = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "backupRemote" -DefaultValue "github")

    if (-not $configuredIsCompanyRepo) {
        Write-Warning "Mirror sync was requested, but isCompanyRepo is false. Skipping mirror sync."
    }
    elseif (-not $configuredBackupRepoPath) {
        Write-Warning "Mirror sync was requested, but backupRepoPath is empty. Skipping mirror sync."
    }
    else {
        $syncMode = $configuredMirrorMode
        if (-not $syncMode) {
            $syncMode = "fetch-merge"
        }

        if ($syncMode -eq "fetch-merge") {
            $syncScriptPath = Join-Path $workspaceRoot "08_Sync_And_Handoff\scripts\sync_company_repo_to_backup.ps1"
            & powershell -ExecutionPolicy Bypass -File $syncScriptPath `
                -WorkRepo $repoRoot `
                -BackupRepo $configuredBackupRepoPath `
                -Branch $branchName `
                -BackupRemote $configuredBackupRemote
        }
        else {
            Write-Warning "Mirror mode '$syncMode' is not auto-supported by end-session.ps1 yet. Skipping mirror sync."
        }
    }
}

$repoHandoffPath = Join-Path $repoRoot "handoff\latest.md"
$repoActiveTaskPath = Join-Path $repoRoot "tasks\active.md"
$repoContextPath = Join-Path $repoRoot "docs\00_CONTEXT.md"
$repoDecisionIndexPath = Join-Path $repoRoot "decisions\0000_INDEX.md"

$projectHandoffPath = Join-Path $projectRoot "handoff.md"
$projectActiveTaskPath = Join-Path $projectRoot "active_task.md"
$projectContextPath = Join-Path $projectRoot "context.md"
$projectExternalBackupPath = Join-Path $projectRoot "external_backup.md"
$projectSessionHistoryPath = Join-Path $projectRoot "session_history.md"
$mirrorMapPath = Join-Path $workspaceRoot "08_Sync_And_Handoff\repository_mirror_map.md"

$filesChangedSection = Convert-ToMarkdownBullets -Items $changedFiles -DefaultItem "No file delta detected"
$warningsSection = Convert-ToMarkdownBullets -Items $Warnings -DefaultItem "No warning"
$nextActionsSection = Convert-ToMarkdownNumbers -Items $NextActions -DefaultItem "Confirm the next task before the next session"

if (-not $CurrentStatus) {
    $CurrentStatus = "Session closed with updated handoff."
}

$repoHandoffContent = @"
# Latest Handoff

## Date

$dateTimeLabel

## Machine

$machineName

## Branch

$branchName

## Last Commit

$lastCommit

## What was done

$SessionSummary

## Files changed

$filesChangedSection

## Current status

$CurrentStatus

## Next actions

$nextActionsSection

## Warnings

$warningsSection

## Tests/checks

- Build: $BuildStatus
- Unit tests: $UnitTestStatus
- Manual tests: $ManualTestStatus
"@
Write-FileUtf8 -PathValue $repoHandoffPath -Content $repoHandoffContent

$repoActiveTaskContent = Read-FileUtf8 -PathValue $repoActiveTaskPath
$repoCloseoutBody = @"
- Date: $dateTimeLabel
- Branch: $branchName
- Last commit: $lastCommit
- Summary: $SessionSummary

### Next actions

$nextActionsSection

### Warnings

$warningsSection
"@
$repoActiveTaskContent = Set-MarkdownSection -Content $repoActiveTaskContent -Heading "Session Closeout" -Body $repoCloseoutBody
Write-FileUtf8 -PathValue $repoActiveTaskPath -Content $repoActiveTaskContent

$projectHandoffContent = @"
# Project Handoff

## Last Updated

$dateTimeLabel

## Last Session Summary

$SessionSummary

## Current State

$CurrentStatus

## Next Actions

$nextActionsSection

## Warnings

$warningsSection

## Links

- Repository: ../../06_Repositories/$repoName/
- Repo handoff: ../../06_Repositories/$repoName/handoff/latest.md
"@
Write-FileUtf8 -PathValue $projectHandoffPath -Content $projectHandoffContent

if (-not (Test-Path -LiteralPath $projectSessionHistoryPath)) {
    $sessionHistoryTemplate = @"
# Session History

## Mục Tiêu

Lưu lịch sử phiên làm việc ở mức tóm tắt để có thể tra lại tiến độ mà không phụ thuộc hoàn toàn vào chat app.

## Lưu Ý

- đây không phải full transcript của mọi tin nhắn
- đây là session journal ngắn gọn
- nếu cần, có thể thêm thread reference hoặc link tới chat

## Entries
"@
    Write-FileUtf8 -PathValue $projectSessionHistoryPath -Content $sessionHistoryTemplate
}

$resolvedThreadReference = if ($ThreadReference) { $ThreadReference } else { "Not recorded" }
$sessionHistoryContent = Read-FileUtf8 -PathValue $projectSessionHistoryPath
$sessionHistoryEntry = @"
### $dateTimeLabel - $ToolName ($SessionMode)

- Thread reference: $resolvedThreadReference
- Machine: $machineName
- Repository: `06_Repositories/$repoName/`
- Branch: `$branchName`
- Last commit: $lastCommit
- Summary: $SessionSummary
- Current status: $CurrentStatus

#### Next actions

$nextActionsSection

#### Warnings

$warningsSection
"@
$sessionHistoryContent = $sessionHistoryContent.TrimEnd() + "`r`n`r`n" + $sessionHistoryEntry + "`r`n"
Write-FileUtf8 -PathValue $projectSessionHistoryPath -Content $sessionHistoryContent

$projectActiveTaskContent = Read-FileUtf8 -PathValue $projectActiveTaskPath
$projectCloseoutBody = @"
- Date: $dateTimeLabel
- Repository: 06_Repositories/$repoName/
- Summary: $SessionSummary

### Next actions

$nextActionsSection

### Warnings

$warningsSection
"@
$projectActiveTaskContent = Set-MarkdownSection -Content $projectActiveTaskContent -Heading "Session Closeout" -Body $projectCloseoutBody
Write-FileUtf8 -PathValue $projectActiveTaskPath -Content $projectActiveTaskContent

if ($RepoContextNote) {
    $repoContextContent = Read-FileUtf8 -PathValue $repoContextPath
    $repoContextBody = @"
- Date: $dateTimeLabel
- Note: $RepoContextNote
"@
    $repoContextContent = Set-MarkdownSection -Content $repoContextContent -Heading "Last Session Update" -Body $repoContextBody
    Write-FileUtf8 -PathValue $repoContextPath -Content $repoContextContent
}

if ($ProjectContextNote) {
    $projectContextContent = Read-FileUtf8 -PathValue $projectContextPath
    $projectContextBody = @"
- Date: $dateTimeLabel
- Note: $ProjectContextNote
"@
    $projectContextContent = Set-MarkdownSection -Content $projectContextContent -Heading "Last Session Update" -Body $projectContextBody
    Write-FileUtf8 -PathValue $projectContextPath -Content $projectContextContent
}

if ($DecisionTitle) {
    $decisionIndexContent = Read-FileUtf8 -PathValue $repoDecisionIndexPath
    if (-not $DecisionId) {
        $DecisionId = Get-NextDecisionId -DecisionIndexContent $decisionIndexContent
    }

    $decisionRow = "| $DecisionId | $DecisionTitle | $DecisionStatus | $today |"
    if ($decisionIndexContent -notmatch [regex]::Escape($decisionRow)) {
        $decisionIndexContent = $decisionIndexContent.TrimEnd() + "`r`n" + $decisionRow + "`r`n"
        Write-FileUtf8 -PathValue $repoDecisionIndexPath -Content $decisionIndexContent
    }
}

$workRemote = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "workRemote" -DefaultValue "company")
$backupRepoPath = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "backupRepoPath" -DefaultValue "")
$backupRemote = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "backupRemote" -DefaultValue "github")
$mirrorMode = [string](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "mirrorMode" -DefaultValue "fetch-merge")
$isCompanyRepo = [bool](Get-ConfigValue -PrimaryConfig $configLocal -FallbackConfig $configTemplate -PropertyName "isCompanyRepo" -DefaultValue $false)

$externalBackupContent = @"
# External Backup / Mirror

File này dùng khi project có repo backup/mirror bên ngoài AI-Workspace.

## Work Repository

AI-Workspace/06_Repositories/$repoName

## Backup Repository

$backupRepoPath

## Work Remote

$workRemote

## Backup Remote

$backupRemote

## Sync Mode

- Company repo: $isCompanyRepo
- Mirror mode: $mirrorMode

## Daily Workflow

1. Work in 06_Repositories/$repoName.
2. Commit and push to official remote.
3. Run scripts/end-session.ps1.
4. If enabled, sync to external backup repo.
5. Review 05_Projects/$ProjectName/handoff.md.

## Security Notes

- Confirm permission before mirroring company code.
- Do not sync secrets.
- Do not sync private AI notes into company repo unless approved.
"@
Write-FileUtf8 -PathValue $projectExternalBackupPath -Content $externalBackupContent

$mirrorMapUpdated = $false
if ((Test-Path -LiteralPath $mirrorMapPath) -and $backupRepoPath) {
    $mirrorMapUpdated = Update-MirrorMap `
        -MirrorMapPath $mirrorMapPath `
        -ProjectNameValue $ProjectName `
        -RepoNameValue $repoName `
        -WorkRemoteValue $workRemote `
        -BranchNameValue $branchName `
        -BackupRepoPathValue $backupRepoPath `
        -BackupRemoteValue $backupRemote `
        -IsCompanyRepoValue $isCompanyRepo
}

if ($GitCommitCloseout) {
    $repoCommitMessage = "chore: update session closeout for $repoName"
    $workspaceCommitMessage = "chore: update workspace handoff for $ProjectName"

    $repoPathsToCommit = @(
        "handoff/latest.md",
        "tasks/active.md",
        "docs/00_CONTEXT.md",
        "decisions/0000_INDEX.md"
    )

    $workspacePathsToCommit = @(
        "05_Projects/$ProjectName/handoff.md",
        "05_Projects/$ProjectName/active_task.md",
        "05_Projects/$ProjectName/context.md",
        "05_Projects/$ProjectName/external_backup.md",
        "05_Projects/$ProjectName/session_history.md"
    )

    if ($mirrorMapUpdated) {
        $workspacePathsToCommit += "08_Sync_And_Handoff/repository_mirror_map.md"
    }

    $null = Invoke-GitCommitForPaths -GitRoot $repoRoot -RelativePaths $repoPathsToCommit -CommitMessage $repoCommitMessage -Label "Repository"
    $null = Invoke-GitCommitForPaths -GitRoot $workspaceRoot -RelativePaths $workspacePathsToCommit -CommitMessage $workspaceCommitMessage -Label "Workspace"
}

Write-Host ""
Write-Host "End-session update completed."
Write-Host "Repository: $repoRoot"
Write-Host "Project notes: $projectRoot"
Write-Host "Branch: $branchName"
Write-Host "Last commit: $lastCommit"
if ($SyncMirror) {
    Write-Host "Mirror sync: requested"
}
else {
    Write-Host "Mirror sync: not requested"
}
if ($mirrorMapUpdated) {
    Write-Host "Mirror map: updated"
}
else {
    Write-Host "Mirror map: unchanged"
}
if ($GitCommitCloseout) {
    Write-Host "Git closeout commit: requested"
}
else {
    Write-Host "Git closeout commit: not requested"
}
