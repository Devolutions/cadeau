#!/usr/bin/env pwsh

[CmdletBinding()]
param(
    [string] $VenvRoot = (Join-Path $HOME ".conan-venvs"),
    [string] $ShimDir = (Join-Path (Join-Path $HOME ".local") "bin"),
    [string] $Conan1Version = "1.66.0",
    [string] $Conan2Version = ">=2,<3",
    [switch] $SkipConan1,
    [switch] $SkipConan2,
    [switch] $NoUserPathUpdate
)

$ErrorActionPreference = "Stop"

if ($SkipConan1 -and $SkipConan2) {
    throw "At least one Conan version must be installed."
}

function Get-PythonLauncher {
    $Candidates = @()

    if ($IsWindows) {
        $Candidates += [pscustomobject]@{ Command = "py"; Arguments = @("-3") }
    }

    $Candidates += [pscustomobject]@{ Command = "python3"; Arguments = @() }
    $Candidates += [pscustomobject]@{ Command = "python"; Arguments = @() }

    foreach ($Candidate in $Candidates) {
        $Command = @(Get-Command $Candidate.Command -CommandType Application -ErrorAction SilentlyContinue)[0]
        if (-not $Command) {
            continue
        }

        & $Command.Source @($Candidate.Arguments) --version > $null 2>&1
        if ($LASTEXITCODE -eq 0) {
            return [pscustomobject]@{
                Command = $Command.Source
                Arguments = [string[]] $Candidate.Arguments
            }
        }
    }

    throw "Python 3 was not found. Install Python 3 and rerun this script."
}

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory=$true)]
        [string] $FilePath,
        [string[]] $Arguments = @()
    )

    $Output = & $FilePath @Arguments 2>&1
    $ExitCode = $LASTEXITCODE

    if ($Output) {
        $Output | ForEach-Object { Write-Host $_ }
    }

    if ($ExitCode -ne 0) {
        throw "Command failed with exit code $ExitCode`: $FilePath $($Arguments -join ' ')"
    }
}

function Invoke-BasePython {
    param(
        [Parameter(Mandatory=$true)]
        [pscustomobject] $Python,
        [string[]] $Arguments = @()
    )

    Invoke-CheckedCommand $Python.Command ([string[]]($Python.Arguments + $Arguments))
}

function Get-VenvCommandPath {
    param(
        [Parameter(Mandatory=$true)]
        [string] $VenvPath,
        [Parameter(Mandatory=$true)]
        [string] $CommandName
    )

    $BinDir = if ($IsWindows) {
        Join-Path $VenvPath "Scripts"
    } else {
        Join-Path $VenvPath "bin"
    }

    $ExecutableName = if ($IsWindows) {
        "$CommandName.exe"
    } else {
        $CommandName
    }

    Join-Path $BinDir $ExecutableName
}

function Install-ConanVenv {
    param(
        [Parameter(Mandatory=$true)]
        [pscustomobject] $Python,
        [Parameter(Mandatory=$true)]
        [string] $Name,
        [Parameter(Mandatory=$true)]
        [string] $PackageSpec,
        [string[]] $ExtraPackages = @()
    )

    $VenvPath = Join-Path $VenvRoot $Name

    if (-not (Test-Path $VenvPath)) {
        Write-Host "Creating $Name virtual environment at $VenvPath"
        Invoke-BasePython $Python @("-m", "venv", $VenvPath)
    } else {
        Write-Host "Reusing $Name virtual environment at $VenvPath"
    }

    $VenvPython = Get-VenvCommandPath $VenvPath "python"
    if (-not (Test-Path $VenvPython)) {
        throw "Expected Python executable was not found in $VenvPath"
    }

    Invoke-CheckedCommand $VenvPython @("-m", "pip", "install", "--upgrade", "pip")

    $Packages = @($PackageSpec) + $ExtraPackages
    Invoke-CheckedCommand $VenvPython (@("-m", "pip", "install", "--upgrade") + $Packages)

    $VenvPath
}

function New-ConanShim {
    param(
        [Parameter(Mandatory=$true)]
        [string] $Name,
        [Parameter(Mandatory=$true)]
        [string] $VenvPath
    )

    New-Item -ItemType Directory -Force -Path $ShimDir | Out-Null

    $ConanExecutable = Get-VenvCommandPath $VenvPath "conan"
    if (-not (Test-Path $ConanExecutable)) {
        throw "Expected Conan executable was not found in $VenvPath"
    }

    if ($IsWindows) {
        $ShimPath = Join-Path $ShimDir "$Name.cmd"
        Set-Content -Path $ShimPath -Encoding ASCII -Value @(
            "@echo off",
            ('"{0}" %*' -f $ConanExecutable)
        )
    } else {
        $ShimPath = Join-Path $ShimDir $Name
        Set-Content -Path $ShimPath -Encoding ASCII -Value @(
            "#!/usr/bin/env sh",
            ('exec "{0}" "$@"' -f $ConanExecutable)
        )
        Invoke-CheckedCommand "chmod" @("+x", $ShimPath)
    }

    Write-Host "Wrote $Name shim to $ShimPath"
    $ShimPath
}

function Normalize-PathEntry {
    param([string] $PathEntry)

    if ([string]::IsNullOrWhiteSpace($PathEntry)) {
        return $null
    }

    $TrimChars = [char[]]@(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )

    [System.IO.Path]::GetFullPath($PathEntry).TrimEnd($TrimChars)
}

function Test-PathListContains {
    param(
        [string] $PathValue,
        [string] $Entry
    )

    $NormalizedEntry = Normalize-PathEntry $Entry
    if (-not $NormalizedEntry) {
        return $false
    }

    $Comparison = if ($IsWindows) {
        [System.StringComparison]::OrdinalIgnoreCase
    } else {
        [System.StringComparison]::Ordinal
    }

    foreach ($ExistingEntry in ($PathValue -split [System.Text.RegularExpressions.Regex]::Escape([System.IO.Path]::PathSeparator))) {
        $NormalizedExistingEntry = Normalize-PathEntry $ExistingEntry
        if ($NormalizedExistingEntry -and [string]::Equals($NormalizedExistingEntry, $NormalizedEntry, $Comparison)) {
            return $true
        }
    }

    $false
}

function Add-ShimDirectoryToPath {
    New-Item -ItemType Directory -Force -Path $ShimDir | Out-Null
    $ResolvedShimDir = (Resolve-Path $ShimDir).Path
    $PathSeparator = [System.IO.Path]::PathSeparator

    if (-not (Test-PathListContains $Env:PATH $ResolvedShimDir)) {
        $Env:PATH = if ([string]::IsNullOrWhiteSpace($Env:PATH)) {
            $ResolvedShimDir
        } else {
            "$ResolvedShimDir$PathSeparator$Env:PATH"
        }
    }

    if ($Env:GITHUB_PATH) {
        Add-Content -Path $Env:GITHUB_PATH -Value $ResolvedShimDir
    }

    if ($IsWindows -and -not $NoUserPathUpdate -and -not $Env:GITHUB_ACTIONS) {
        $UserPath = [System.Environment]::GetEnvironmentVariable("PATH", "User")

        if (-not (Test-PathListContains $UserPath $ResolvedShimDir)) {
            $NewUserPath = if ([string]::IsNullOrWhiteSpace($UserPath)) {
                $ResolvedShimDir
            } else {
                "$ResolvedShimDir$PathSeparator$UserPath"
            }

            [System.Environment]::SetEnvironmentVariable("PATH", $NewUserPath, "User")
            Write-Host "Added $ResolvedShimDir to the user PATH. Open a new terminal to use conan1 and conan2."
        }
    } elseif (-not $IsWindows -and -not $NoUserPathUpdate -and -not $Env:GITHUB_ACTIONS) {
        Write-Host "Add $ResolvedShimDir to PATH in your shell profile to use the shims in new terminals."
    }
}

$Python = Get-PythonLauncher
Write-Host "Using Python launcher: $($Python.Command) $($Python.Arguments -join ' ')"

New-Item -ItemType Directory -Force -Path $VenvRoot | Out-Null

$ShimPaths = @()

if (-not $SkipConan1) {
    $Conan1Venv = Install-ConanVenv `
        -Python $Python `
        -Name "conan1" `
        -PackageSpec "conan==$Conan1Version" `
        -ExtraPackages @("invoke", "Jinja2", "urllib3", "chardet", "requests")
    $ShimPaths += New-ConanShim -Name "conan1" -VenvPath $Conan1Venv
}

if (-not $SkipConan2) {
    $Conan2Venv = Install-ConanVenv `
        -Python $Python `
        -Name "conan2" `
        -PackageSpec "conan$Conan2Version"
    $ShimPaths += New-ConanShim -Name "conan2" -VenvPath $Conan2Venv
}

Add-ShimDirectoryToPath

foreach ($ShimPath in $ShimPaths) {
    Invoke-CheckedCommand $ShimPath @("--version")
}

Write-Host "Conan setup complete."
