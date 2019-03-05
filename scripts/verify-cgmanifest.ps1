# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
param(
    [Parameter(Mandatory=$true)]
    [String] $Path
)

$ErrorActionPreference = "Stop"

function Find-Submodules([String]$Path) {
    # Validate passed in path exists
    if (-not (Test-Path -Path $Path)) {
        Write-Error -Message "$Path is not a valid path"
    }

    # Validate passed in path is the root of a git dir
    $GitDir = Join-Path -Path $Path -ChildPath ".git"
    if (-not (Test-Path -Path $GitDir)) {
        Write-Error -Message "$Path is not the root of a git repo. $Path/.git does not exist"
    }

    $GitModules = Join-Path -Path $Path -ChildPath ".gitmodules"

    if (-not (Test-Path -Path $GitModules)) {
        return
    }

    # Parse .gitmodules file
    $gitmodulesRegex = '(?m)(^\[submodule "[^\s"\]]*"\]\r?$\s+path\s+=\s+([^\s]*)\r?$\s+url\s+=\s+([^\s]*)\r?$)*'
    $modules = Get-Content -Path $GitModules |
            Out-String |
            Select-String -Pattern $gitmodulesRegex -AllMatches |
                ForEach-Object { $_.Matches } |
                Where-Object { $_.Groups[3].Success } |
                ForEach-Object { @{path = (Join-Path -Path $Path -ChildPath $_.Groups[2].Value); url = $_.Groups[3].Value} }

    $modules = foreach ($module in $modules) {
        Push-Location -Path $module.path
        $hash = git rev-parse HEAD | Out-String
        Pop-Location
        $module + @{hash = $hash.Trim()}
    }

    # Recurse into modules
    $tempmodules = $modules
    foreach ($module in $modules) {
        $tempmodules += Find-Submodules -Path $module.path
    }
    $modules = $tempmodules
    $modules
}

$modules = Find-Submodules -Path $Path

# Parse cgmanifests.json
$manifestFile = Join-Path -Path $Path -ChildPath "cgmanifest.json"
if (-not (Test-Path -Path $manifestFile)) {
    Write-Error -Message "cgmanifest.json does not exist $manifestFile"
}
$manifest = Get-Content -Path $manifestFile | Out-String | ConvertFrom-Json
$registrations = $manifest.Registrations
$components = $registrations | ForEach-Object { $_.component }
$gitComponents = $components | Where-Object { $_.type -eq "git" } | ForEach-Object { $_.git }

# Verify that each submodule is correctly in the manifest
foreach ($module in $modules) {
    $url = $module.url
    $component = $gitComponents | Where-Object { $_.repositoryUrl -eq $url }
    if (-not $component) {
        Write-Error -Message "Submodule $($module.path) does not exist in $manifestFile. Repository url is $url. Commit Hash is $($module.hash)"
    }

    if (-not ($component | Where-Object { $_.commitHash -eq $module.hash })) {
        Write-Error -Message "Submodule $($module.path) does have current hash $($module.hash) listed in $manifestFile"
    }
}