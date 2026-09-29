# Windows counterpart of run_tests.sh.
# Usage: run_tests.ps1 <path-to-kriol.exe> <project-root>

param(
    [Parameter(Mandatory)] [string] $Kriol,
    [Parameter(Mandatory)] [string] $Root
)

$ErrorActionPreference = 'Stop'

$Kriol = (Resolve-Path $Kriol).Path
$Root = (Resolve-Path $Root).Path
$Work = Join-Path ([IO.Path]::GetTempPath()) "kriol-tests-$PID"
New-Item -ItemType Directory $Work -Force | Out-Null
$EmptyInput = Join-Path $Work 'empty.stdin'
New-Item -ItemType File $EmptyInput -Force | Out-Null

$script:Passed = 0
$script:Failed = @()

function Write-Result([string] $Name, [bool] $Ok, [string] $Detail = '') {
    if ($Ok) {
        Write-Host ("  {0,-44} PASS{1}" -f $Name, $Detail)
        $script:Passed++
    } else {
        Write-Host ("  {0,-44} FAIL{1}" -f $Name, $Detail)
        $script:Failed += $Name
    }
}

function Get-Sources([string] $Dir, [string] $Extension) {
    if (-not (Test-Path $Dir)) { return @() }
    Get-ChildItem $Dir -File | Where-Object Extension -eq $Extension | Sort-Object Name
}

function Invoke-Kriol([string[]] $Arguments) {
    & $Kriol @Arguments *> $null
    return $LASTEXITCODE -eq 0
}

# Runs a compiled program with stdin from a file; false on failure or timeout.
function Invoke-Program([string] $Exe, [string] $Stdin) {
    $info = [Diagnostics.ProcessStartInfo]::new($Exe)
    $info.UseShellExecute = $false
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true

    $process = [Diagnostics.Process]::Start($info)
    $null = $process.StandardOutput.ReadToEndAsync()
    $null = $process.StandardError.ReadToEndAsync()
    try {
        $bytes = [IO.File]::ReadAllBytes($Stdin)
        $process.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length)
        $process.StandardInput.Close()
    } catch {
        # The program exited without reading all of its input.
    }

    if (-not $process.WaitForExit(5000)) {
        $process.Kill()
        return $false
    }
    $process.WaitForExit()
    return $process.ExitCode -eq 0
}

function Test-CompilesAndRuns([IO.FileInfo] $Source) {
    $exe = Join-Path $Work 'program.exe'
    Remove-Item $exe -ErrorAction SilentlyContinue

    $fixture = "$($Source.FullName).stdin"
    $stdin = if (Test-Path $fixture) { $fixture } else { $EmptyInput }

    $ok = (Invoke-Kriol @($Source.FullName, '-o', $exe)) -and (Invoke-Program $exe $stdin)
    Write-Result ([IO.Path]::GetRelativePath($Root, $Source.FullName)) $ok
}

Write-Host "`n~~ Running tests ~~`n"

foreach ($source in Get-Sources (Join-Path $Root 'examples') '.kriol') {
    Test-CompilesAndRuns $source
}

foreach ($source in Get-Sources (Join-Path $Root 'tests/pass') '.kr') {
    Test-CompilesAndRuns $source
}

$exe = Join-Path $Work 'text.exe'
$ok = Invoke-Kriol @('--text', 'fn inisiu() { mostran("Kuale, Mundu!"); }', '-o', $exe)
$output = if ($ok) { & $exe } else { $null }
Write-Result 'inline source text' ($output -eq 'Kuale, Mundu!')

foreach ($level in '-O0', '-O3') {
    $exe = Join-Path $Work 'optimized.exe'
    $ok = Invoke-Kriol @($level, '--text', 'fn inisiu() { nter[3] a = [1, 2, 3]; mostran(a[2] / a[0]); }', '-o', $exe)
    $output = if ($ok) { & $exe } else { $null }
    Write-Result "optimization level $level" ($output -eq '3')
}

# Intermediate object files must not overwrite files next to the output.
$outputDir = Join-Path $Work 'output'
New-Item -ItemType Directory $outputDir | Out-Null
$sentinels = @('program.o', 'program.obj', 'program.exe.obj') | ForEach-Object { Join-Path $outputDir $_ }
$sentinels | ForEach-Object { Set-Content $_ 'precious' }
$ok = Invoke-Kriol @('--text', 'fn inisiu() { mostran("x"); }', '-o', (Join-Path $outputDir 'program'))
$untouched = -not ($sentinels | Where-Object { (Get-Content $_ -ErrorAction SilentlyContinue) -ne 'precious' })
Write-Result 'output directory left untouched' ($ok -and $untouched)

foreach ($source in Get-Sources (Join-Path $Root 'tests/fail') '.kr') {
    $rejected = -not (Invoke-Kriol @($source.FullName, '-o', (Join-Path $Work 'rejected.exe')))
    $name = [IO.Path]::GetRelativePath($Root, $source.FullName)
    if ($rejected) {
        Write-Result $name $true ' (rejected)'
    } else {
        Write-Result $name $false ' (should have been rejected)'
    }
}

Remove-Item $Work -Recurse -Force -ErrorAction SilentlyContinue

$total = $script:Passed + $script:Failed.Count
Write-Host "`n  $($script:Passed)/$total passed`n"

if ($script:Failed.Count -ne 0) {
    Write-Host "Failed tests:"
    $script:Failed | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
