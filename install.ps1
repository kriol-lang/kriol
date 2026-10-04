# Installs the latest Kriol release for Windows.
#
#   irm https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.ps1 | iex
#
# Options need the script-block form:
#
#   & ([scriptblock]::Create((irm https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.ps1))) -Pre
#
# Requires Windows PowerShell 5.1 (included in Windows 10 and later; installable
# on Windows 7 through WMF 5.1) or PowerShell 7.

param(
    # Install the newest release even when it is a pre-release.
    [switch] $Pre,
    # Where kriol.exe and ld.lld.exe go.
    [string] $InstallDir = (Join-Path $env:LOCALAPPDATA 'Programs\kriol'),
    # Leave the user PATH alone.
    [switch] $NoModifyPath
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'  # the progress bar slows downloads a lot

$Repo = 'kriol-lang/kriol'

# GitHub requires TLS 1.2, which older Windows PowerShell does not enable.
[Net.ServicePointManager]::SecurityProtocol =
    [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

if ($Pre) {
    Write-Host 'Fetching latest Kriol pre-release...'
    $releases = Invoke-RestMethod "https://api.github.com/repos/$Repo/releases"
} else {
    Write-Host 'Fetching latest Kriol release...'
    $releases = @(Invoke-RestMethod "https://api.github.com/repos/$Repo/releases/latest")
}

$AssetPattern = '*-windows-x86_64.zip'
$release = $releases |
    Where-Object { $_.assets | Where-Object { $_.name -like $AssetPattern } } |
    Select-Object -First 1

if (-not $release) {
    throw 'Could not find a Windows x86_64 release asset. Try -Pre, or download a release manually.'
}
$asset = $release.assets | Where-Object { $_.name -like $AssetPattern } | Select-Object -First 1
$checksums = $release.assets | Where-Object { $_.name -eq 'SHA256SUMS.txt' } | Select-Object -First 1

$temp = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName())
New-Item -ItemType Directory $temp | Out-Null

try {
    $archive = Join-Path $temp $asset.name
    Write-Host "Downloading $($asset.name)..."
    Invoke-WebRequest $asset.browser_download_url -OutFile $archive -UseBasicParsing

    if ($checksums) {
        $sumsFile = Join-Path $temp $checksums.name
        Invoke-WebRequest $checksums.browser_download_url -OutFile $sumsFile -UseBasicParsing
        $expected = Get-Content $sumsFile |
            ForEach-Object { $hash, $file = $_ -split '\s+', 2; if ($file -and $file.TrimStart('*') -eq $asset.name) { $hash } } |
            Select-Object -First 1
        if (-not $expected) {
            throw "$($checksums.name) has no entry for $($asset.name)."
        }
        if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expected) {
            throw "The checksum of $($asset.name) does not match $($checksums.name); the download may be corrupted."
        }
    } else {
        Write-Warning "The release has no SHA256SUMS.txt, so $($asset.name) was not verified."
    }

    Write-Host 'Extracting...'
    Expand-Archive $archive -DestinationPath $temp

    $kriol = Get-ChildItem $temp -Recurse -Filter kriol.exe | Select-Object -First 1
    if (-not $kriol) {
        throw 'kriol.exe not found in the release archive.'
    }

    # kriol.exe links programs with the ld.lld.exe next to it, so the whole
    # folder is installed together.
    New-Item -ItemType Directory $InstallDir -Force | Out-Null
    Copy-Item (Join-Path $kriol.DirectoryName '*') $InstallDir -Recurse -Force
    $installed = Get-ChildItem $kriol.DirectoryName -Recurse -File |
        ForEach-Object { Join-Path $InstallDir $_.FullName.Substring($kriol.DirectoryName.Length) }
} finally {
    Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
}

# The binaries are not code-signed yet, so drop any downloaded-from-the-internet
# mark from the installed files and check that Windows actually lets kriol.exe run.
$installed | Unblock-File -ErrorAction SilentlyContinue
$runs = $false
try {
    & (Join-Path $InstallDir 'kriol.exe') --version *> $null
    $runs = $LASTEXITCODE -eq 0
} catch {}

Write-Host ''
if ($runs) {
    Write-Host "-> Kriol installed successfully at: $InstallDir\kriol.exe"
    Write-Host ''
} else {
    Write-Host "-> Kriol installed at: $InstallDir\kriol.exe"
    Write-Host ''
    Write-Warning 'Windows did not let kriol.exe run. Windows support is experimental and'
    Write-Warning 'the binaries are not code-signed yet, so Smart App Control or an App'
    Write-Warning 'Control policy may block them. For workarounds, see:'
    Write-Warning "  https://github.com/$Repo#if-windows-blocks-kriolexe"
    Write-Host ''
}

$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$onPath = ($userPath -split ';') -contains $InstallDir

if ($NoModifyPath) {
    if (-not $onPath) {
        Write-Host "-> Add $InstallDir to your PATH to use kriol from any terminal."
    }
} elseif (-not $onPath) {
    # Read the raw value so entries such as %USERPROFILE%\bin stay unexpanded.
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $true)
    try {
        $raw = $key.GetValue('Path', '', 'DoNotExpandEnvironmentNames')
        $newPath = if ($raw) { "$($raw.TrimEnd(';'));$InstallDir" } else { $InstallDir }
        $key.SetValue('Path', $newPath, 'ExpandString')
    } finally {
        $key.Close()
    }
    # Setting any user variable broadcasts the change to new terminals.
    [Environment]::SetEnvironmentVariable('KRIOL_INSTALL_REFRESH', $null, 'User')
    $env:Path = "$env:Path;$InstallDir"

    Write-Host "-> Added $InstallDir to your user PATH."
    Write-Host '   Open a new terminal if kriol is not found.'
}

Write-Host ''
Write-Host '-> Try:'
Write-Host ''
Write-Host '     kriol --help'
Write-Host ''
