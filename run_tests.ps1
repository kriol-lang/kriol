# Windows counterpart of run_tests.sh.
# Usage: run_tests.ps1 <path-to-kriol.exe> <project-root>

param(
    [Parameter(Mandatory)] [string] $Kriol,
    [Parameter(Mandatory)] [string] $Root
)

$ErrorActionPreference = 'Stop'
# The compiler writes UTF-8, and the expected messages have accents.
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)

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

# The exit status and the combined output of one compiler run.
function Get-KriolResult([string[]] $Arguments) {
    # Windows PowerShell turns native stderr into terminating errors under 'Stop'.
    $ErrorActionPreference = 'Continue'
    $output = (& $Kriol @Arguments 2>&1 | ForEach-Object { "$_" }) -join "`n"
    return [pscustomobject]@{ Ok = ($LASTEXITCODE -eq 0); Output = $output }
}

# Runs a program on a stdin file; the exit status and both outputs, or $null on timeout.
function Invoke-Program([string] $Exe, [string] $Stdin) {
    $info = [Diagnostics.ProcessStartInfo]::new($Exe)
    $info.UseShellExecute = $false
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    # Programs write UTF-8 bytes whatever the console code page is.
    $info.StandardOutputEncoding = [Text.UTF8Encoding]::new($false)
    $info.StandardErrorEncoding = [Text.UTF8Encoding]::new($false)

    $process = [Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    try {
        $bytes = [IO.File]::ReadAllBytes($Stdin)
        $process.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length)
        $process.StandardInput.Close()
    } catch {
        # The program exited without reading all of its input.
    }

    if (-not $process.WaitForExit(5000)) {
        $process.Kill()
        return $null
    }
    $process.WaitForExit()
    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Stdout = $stdout.Result -replace "`r`n", "`n"
        Stderr = $stderr.Result
    }
}

# The value of the first '// <key>: <value>' line of a source file.
function Get-Directive([IO.FileInfo] $Source, [string] $Key) {
    Select-String -Path $Source.FullName -Pattern "^// ${Key}: (.*)$" |
        Select-Object -First 1 | ForEach-Object { $_.Matches[0].Groups[1].Value }
}

# A '<source>.stdout' fixture is the exact output the program must write.
function Test-Stdout([IO.FileInfo] $Source, [string] $Actual) {
    $fixture = "$($Source.FullName).stdout"
    return -not (Test-Path $fixture) -or ([IO.File]::ReadAllText($fixture) -eq $Actual)
}

# Compiles and runs a source on its stdin fixture; $null if it does not compile or times out.
function Invoke-Source([IO.FileInfo] $Source) {
    $exe = Join-Path $Work 'program.exe'
    Remove-Item $exe -ErrorAction SilentlyContinue

    $fixture = "$($Source.FullName).stdin"
    $stdin = if (Test-Path $fixture) { $fixture } else { $EmptyInput }

    if (-not (Invoke-Kriol @($Source.FullName, '-o', $exe))) { return $null }
    return Invoke-Program $exe $stdin
}

function Test-CompilesAndRuns([IO.FileInfo] $Source) {
    $run = Invoke-Source $Source
    $ok = $run -and $run.ExitCode -eq 0 -and (Test-Stdout $Source $run.Stdout)
    Write-Result ([IO.Path]::GetRelativePath($Root, $Source.FullName)) $ok
}

# A program that must stop with a runtime error: a non-zero exit status (or the
# '// exit:' one), the '// expect:' message on stderr, and only the expected output.
function Test-StopsAsExpected([IO.FileInfo] $Source) {
    $run = Invoke-Source $Source
    $status = Get-Directive $Source 'exit'
    $message = Get-Directive $Source 'expect'
    $ok = $run -and (Test-Stdout $Source $run.Stdout) -and
        $(if ($status) { $run.ExitCode -eq [int] $status } else { $run.ExitCode -ne 0 }) -and
        (-not $message -or $run.Stderr.Contains($message))
    Write-Result ([IO.Path]::GetRelativePath($Root, $Source.FullName)) $ok
}

Write-Host "`n~~ Running tests ~~`n"

foreach ($source in Get-Sources (Join-Path $Root 'examples') '.kriol') {
    Test-CompilesAndRuns $source
}

foreach ($source in Get-Sources (Join-Path $Root 'tests/pass') '.kr') {
    Test-CompilesAndRuns $source
}

foreach ($source in Get-Sources (Join-Path $Root 'tests/panic') '.kr') {
    Test-StopsAsExpected $source
}

$exe = Join-Path $Work 'text.exe'
$ok = Invoke-Kriol @('--text', 'fn inisiu() { mostran("Kualeh, Mundu!"); }', '-o', $exe)
$output = if ($ok) { & $exe } else { $null }
Write-Result 'inline source text' ($output -eq 'Kualeh, Mundu!')

foreach ($level in '0', '3') {
    $exe = Join-Path $Work 'optimized.exe'
    $ok = Invoke-Kriol @('--opt-lvl', $level, '--text', 'fn inisiu() { int[3] a = [1, 2, 3]; mostran(a[2] / a[0]); }', '-o', $exe)
    $output = if ($ok) { & $exe } else { $null }
    Write-Result "optimization level $level" ($output -eq '3')
}

$unhandled = 'fn f() int : Erru { lansa Erru{mensage = "boom"}; } fn inisiu() { int x = f(); mostran("never"); }'
$exe = Join-Path $Work 'unhandled.exe'
$ok = Invoke-Kriol @('--text', $unhandled, '-o', $exe)
Write-Result 'unhandled error compiles' $ok
$output = if ($ok) { (& $exe 2>&1) -join "`n" } else { '' }
Write-Result 'unhandled error stops the program' ($ok -and $LASTEXITCODE -ne 0 -and $output -match 'boom' -and $output -notmatch 'never')
$strictRejected = -not (Invoke-Kriol @('--strict', '--text', $unhandled, '-o', (Join-Path $Work 'strict.exe')))
Write-Result '--strict rejects warnings' $strictRejected

$outputDir = Join-Path $Work 'output'
New-Item -ItemType Directory $outputDir | Out-Null
$sentinels = @('program.o', 'program.obj', 'program.exe.obj') | ForEach-Object { Join-Path $outputDir $_ }
$sentinels | ForEach-Object { Set-Content $_ 'precious' }
$ok = Invoke-Kriol @('--text', 'fn inisiu() { mostran("x"); }', '-o', (Join-Path $outputDir 'program'))
$untouched = -not ($sentinels | Where-Object { (Get-Content $_ -ErrorAction SilentlyContinue) -ne 'precious' })
Write-Result 'output directory left untouched' ($ok -and $untouched)

function Test-RejectedWith([string] $Name, [string] $Expected, [string[]] $Arguments) {
    $result = Get-KriolResult $Arguments
    if ($result.Ok) {
        Write-Result $Name $false ' (should have been rejected)'
    } elseif (-not $result.Output.Contains($Expected)) {
        Write-Result $Name $false " (expected: $Expected)"
    } else {
        Write-Result $Name $true
    }
}

$cliDir = Join-Path $Work 'cli'
New-Item -ItemType Directory (Join-Path $cliDir 'folder.kriol') -Force | Out-Null
$txtSource = Join-Path $cliDir 'program.txt'
Set-Content $txtSource 'fn inisiu() { mostran("txt"); }'
$cliOut = Join-Path $cliDir 'program.exe'
Test-RejectedWith 'cli: missing input' 'falta o programa a compilar' @('-o', $cliOut)
Test-RejectedWith 'cli: file and --text together' 'um ficheiro ou --text' @($txtSource, '--text', 'fn inisiu() {}')
Test-RejectedWith 'cli: missing file' 'não existe' @((Join-Path $cliDir 'missing.kriol'))
Test-RejectedWith 'cli: directory as input' 'não é um ficheiro' @((Join-Path $cliDir 'folder.kriol'))
Test-RejectedWith 'cli: unknown extension' 'não tem a extensão de um programa Kriol' @($txtSource)
Test-RejectedWith 'cli: invalid optimization level' 'o nível de otimização tem de ser' @('--text', 'fn inisiu() {}', '--opt-lvl', '5')
Test-RejectedWith 'cli: unknown target' 'não é suportado' @('--text', 'fn inisiu() {}', '--target', 'sparc')
Test-RejectedWith 'cli: unknown option' "a opção '--sem-isto' não existe" @('--sem-isto')
Test-RejectedWith 'cli: option without a value' "falta o valor da opção '-o'" @('--text', 'fn inisiu() {}', '-o')
Test-RejectedWith 'cli: two files' 'está a mais' @((Join-Path $cliDir 'a.kriol'), (Join-Path $cliDir 'b.kriol'))
Write-Result 'cli: --version' ((& $Kriol --version) -match '^Kriol v\d+\.\d+\.\d+$')

Write-Result 'cli: --help' (((& $Kriol --help) -join "`n").Contains('Utiliza'))

$ok = Invoke-Kriol @('--ignore-extension', $txtSource, '-o', $cliOut)
$output = if ($ok) { & $cliOut } else { $null }
Write-Result 'cli: --ignore-extension' ($output -eq 'txt')

$irFile = Join-Path $cliDir 'program.ll'
$ok = Invoke-Kriol @('--emit-ir', '--text', 'fn inisiu() {}', '-o', $irFile)
Write-Result 'cli: --emit-ir writes the IR' ($ok -and (Select-String -Path $irFile -Pattern 'define' -Quiet))

# A '// expect: <text>' line names a diagnostic the compiler must report.
foreach ($source in Get-Sources (Join-Path $Root 'tests/fail') '.kr') {
    $result = Get-KriolResult @($source.FullName, '-o', (Join-Path $Work 'rejected.exe'))
    $name = [IO.Path]::GetRelativePath($Root, $source.FullName)
    $expected = Select-String -Path $source.FullName -Pattern '^// expect: (.*)$' |
        Select-Object -First 1 | ForEach-Object { $_.Matches[0].Groups[1].Value }
    if ($result.Ok) {
        Write-Result $name $false ' (should have been rejected)'
    } elseif ($expected -and -not $result.Output.Contains($expected)) {
        Write-Result $name $false " (expected: $expected)"
    } else {
        Write-Result $name $true ' (rejected)'
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
