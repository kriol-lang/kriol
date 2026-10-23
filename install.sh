#!/bin/sh

set -eu

REPO="kriol-lang/kriol"
INSTALL_DIR="$HOME/.kriol"

require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "error: required command not found: $1" >&2
        exit 1
    }
}

require curl
require tar
require mktemp
require awk

if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    echo "error: required command not found: sha256sum or shasum" >&2
    exit 1
fi

sha256() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d ' ' -f 1
        return
    fi
    shasum -a 256 "$1" | cut -d ' ' -f 1
}

PRE=0

while [ $# -gt 0 ]; do
    case "$1" in
        --pre)
            PRE=1
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
    shift
done

if [ "$PRE" -eq 1 ]; then
    echo "Fetching latest Kriol pre-release..."
    echo
    RELEASE_JSON=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases")
else
    echo "Fetching latest Kriol release..."
    echo
    RELEASE_JSON=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest")
fi

ASSET_URL=$(
    echo "$RELEASE_JSON" |
    grep '"browser_download_url":' |
    grep 'linux-x86_64.tar.xz"' |
    cut -d '"' -f 4 |
    head -n 1
)

if [ -z "$ASSET_URL" ]; then
    echo "error: could not find Linux x86_64 release asset" >&2
    exit 1
fi

ASSET_NAME=$(printf '%s' "${ASSET_URL##*/}" | sed 's/%2[Bb]/+/g')
SUMS_URL="${ASSET_URL%/*}/SHA256SUMS.txt"

if ! printf '%s\n' "$RELEASE_JSON" | grep -qF "\"$SUMS_URL\""; then
    echo "error: the release has no SHA256SUMS.txt, so $ASSET_NAME cannot be verified" >&2
    exit 1
fi

TMPDIR=$(mktemp -d)

cleanup() {
    rm -rf "$TMPDIR"
}

trap cleanup EXIT INT TERM

ARCHIVE="$TMPDIR/kriol.tar.xz"
SUMS="$TMPDIR/SHA256SUMS.txt"

echo "Downloading..."
curl -fL "$ASSET_URL" -o "$ARCHIVE"
curl -fsSL "$SUMS_URL" -o "$SUMS"

echo "Verifying..."
EXPECTED=$(
    awk -v name="$ASSET_NAME" '{ file = $2; sub(/^\*/, "", file); if (file == name) { print $1; exit } }' "$SUMS"
)

if [ -z "$EXPECTED" ]; then
    echo "error: SHA256SUMS.txt has no entry for $ASSET_NAME" >&2
    exit 1
fi

if [ "$(sha256 "$ARCHIVE")" != "$EXPECTED" ]; then
    echo "error: the checksum of $ASSET_NAME does not match SHA256SUMS.txt; the download may be corrupted" >&2
    exit 1
fi

echo "Extracting..."
tar -xJf "$ARCHIVE" -C "$TMPDIR"

BINARY=$(
    find "$TMPDIR" -type f -name kriol | head -n 1
)

if [ -z "$BINARY" ]; then
    echo "error: kriol binary not found in release archive" >&2
    exit 1
fi

mkdir -p "$INSTALL_DIR"

cp "$BINARY" "$INSTALL_DIR/kriol"
chmod +x "$INSTALL_DIR/kriol"

echo
echo "-> Kriol installed successfully at: $INSTALL_DIR/kriol"
echo

if ! printf '%s' "$PATH" | grep -q "$INSTALL_DIR"; then
    echo "-> Make sure to add the Kriol your shell env:"
    echo
    echo "     export PATH=\"\$HOME/.kriol:\$PATH\""
    echo

    echo "-> Then close and reopen the shell and try:"
    echo
    echo "     kriol --help"
    echo
fi
