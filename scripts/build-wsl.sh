#!/usr/bin/env bash
#
# Build the CLCL Win32 binaries from WSL by driving the Windows MSBuild.
#
# Runs standalone (./scripts/build-wsl.sh) or via `make build-wsl`, which
# exports the same knobs as environment variables.
#
# One workaround is baked in: mt.exe cannot embed res/manifest.xml when the
# sources sit on a \\wsl.localhost UNC path (error c1010070 -> LNK1327). The
# tree is therefore mirrored to a drive-local directory and built there, with
# the artifacts copied back afterwards.
#
# TOOLSET and SDK are empty by default - the .vcxproj files decide. Set them to
# build against something else, e.g. TOOLSET=v145, or TOOLSET=v141_xp SDK=7.0
# to restore the original XP-targeting configuration on a machine that has it.
#
# WSL<->Windows interop here intermittently refuses to launch Windows processes
# ("UtilAcceptVsock:273: accept4 failed 110") and recovers on its own after
# some seconds, so the build waits for it rather than failing outright.

set -euo pipefail

CONFIG="${CONFIG:-Release}"
PLATFORM="${PLATFORM:-x86}"
TOOLSET="${TOOLSET:-}"
SDK="${SDK:-}"
WIN_BUILD="${WIN_BUILD:-/mnt/c/temp/clcl-build}"
VSWHERE="${VSWHERE:-/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe}"
INTEROP_TRIES="${INTEROP_TRIES:-12}"
INTEROP_DELAY="${INTEROP_DELAY:-5}"
BUILD_TRIES="${BUILD_TRIES:-3}"
# Word splitting is intended - this arrives from make as a plain string.
# shellcheck disable=SC2206
ARTIFACTS=(${ARTIFACTS:-CLCL.exe CLCLHook.dll CLCLSet.exe})

die() {
	echo "build-wsl: $*" >&2
	exit 1
}

# Block until a Windows process will actually start, so a flaky interop shows up
# as a wait instead of a bogus "MSBuild not found" or a half-finished build.
wait_for_interop() {
	local attempt
	for attempt in $(seq 1 "$INTEROP_TRIES"); do
		if "$VSWHERE" -help >/dev/null 2>&1; then
			return 0
		fi
		echo "==> WSL interop not responding, retrying ($attempt/$INTEROP_TRIES)"
		sleep "$INTEROP_DELAY"
	done
	return 1
}

find_msbuild() {
	"$VSWHERE" -latest -products '*' -requires Microsoft.Component.MSBuild \
		-find 'MSBuild\**\Bin\MSBuild.exe' 2>/dev/null | tr -d '\r' | head -n1
}

cd "$(dirname "${BASH_SOURCE[0]}")/.."

[ -x "$VSWHERE" ] || die 'vswhere.exe not found - is Visual Studio installed?'

wait_for_interop \
	|| die "WSL interop still down after $((INTEROP_TRIES * INTEROP_DELAY))s - try 'wsl --shutdown' from Windows."

msbuild="$(find_msbuild)"
[ -n "$msbuild" ] \
	|| die 'MSBuild not found - install the "Desktop development with C++" workload.'
msbuild="$(wslpath -u "$msbuild")"
echo "==> msbuild: $msbuild"

echo "==> mirroring to $WIN_BUILD"
mkdir -p "$WIN_BUILD"
# Release/ and Debug/ are excluded from --delete too, which keeps the mirror's
# object files around so repeat builds stay incremental.
rsync -a --delete \
	--exclude='.git/' --exclude='.claude/' \
	--exclude='Release/' --exclude='Debug/' --exclude='.vs/' \
	./ "$WIN_BUILD/"

props=(-p:Configuration="$CONFIG" -p:Platform="$PLATFORM")
if [ -n "$TOOLSET" ]; then
	props+=(-p:PlatformToolset="$TOOLSET")
fi
if [ -n "$SDK" ]; then
	props+=(-p:WindowsTargetPlatformVersion="$SDK")
fi

run_msbuild() {
	(
		cd "$WIN_BUILD"
		"$msbuild" CLCL.sln "${props[@]}" -m -v:minimal -nologo
	)
}

# Interop can also drop the MSBuild launch itself, after the check above passed.
# Only that failure is retried - a real build error fails on the first attempt.
log="$(mktemp)"
trap 'rm -f "$log"' EXIT

for attempt in $(seq 1 "$BUILD_TRIES"); do
	if run_msbuild 2>&1 | tee "$log"; then
		break
	fi
	if ! grep -q 'UtilAcceptVsock' "$log"; then
		die 'build failed'
	fi
	if [ "$attempt" -eq "$BUILD_TRIES" ]; then
		die "WSL interop dropped the MSBuild launch $BUILD_TRIES times - try 'wsl --shutdown' from Windows."
	fi
	echo "==> WSL interop dropped the MSBuild launch, retrying ($attempt/$BUILD_TRIES)"
	wait_for_interop \
		|| die "WSL interop still down - try 'wsl --shutdown' from Windows."
done

mkdir -p "$CONFIG"
for a in "${ARTIFACTS[@]}"; do
	cp -f "$WIN_BUILD/$CONFIG/$a" "$CONFIG/"
done

echo "==> $CONFIG/"
ls -l "${ARTIFACTS[@]/#/$CONFIG/}"
