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

$asset = $releases |
    ForEach-Object { $_.assets } |
    Where-Object { $_.name -like '*-windows-x86_64.zip' } |
    Select-Object -First 1

if (-not $asset) {
    throw 'Could not find a Windows x86_64 release asset. Try -Pre, or download a release manually.'
}

$temp = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName())
New-Item -ItemType Directory $temp | Out-Null

try {
    $archive = Join-Path $temp $asset.name
    Write-Host "Downloading $($asset.name)..."
    Invoke-WebRequest $asset.browser_download_url -OutFile $archive -UseBasicParsing

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
} finally {
    Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ''
Write-Host "-> Kriol installed successfully at: $InstallDir\kriol.exe"
Write-Host ''

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
