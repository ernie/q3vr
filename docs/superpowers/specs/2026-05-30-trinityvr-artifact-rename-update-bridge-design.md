# Release-artifact rename `q3vr` → `trinityvr` + auto-update bridge — design

**Date:** 2026-05-30
**Author:** Ernie Miller (with Claude)
**Status:** Draft for review

## Summary

Rename the published release assets and the auto-updater's matching contract
from `q3vr` to `trinityvr`, repoint the updater at the already-renamed
`ernie/trinity-vr` repository, and ship a **transitional bridge** so existing
`q3vr` installs migrate onto the new code without stranding or update loops.

The binary identity (`trinityvr.exe` / `gltrinityvr.exe`, `PROJECT_NAME`,
`CLIENT_NAME`) was already renamed in commit `de5db407`. This change finishes
the rename at the *release/distribution* layer.

## Background: how auto-update actually works

Two layers, often conflated, must be understood separately:

1. **Asset filename** (`q3vr-windows-msvc-x86_64.zip`) — governs *download
   discovery*. The active updater (`code/qcommon/autoupdate.c`) is the GitHub
   Releases client: on startup it queries
   `api.github.com/repos/<owner>/<repo>/releases/latest`, builds an expected
   asset name from `UPDATE_ASSET_PREFIX` + platform/arch
   (`Update_BuildAssetName`), and downloads the release asset whose `name`
   matches **literally** (`Q_stricmp` — case-insensitive but otherwise exact).
   A release with no asset by that name yields "Asset not found" and the client
   is stranded. This matcher is compiled into every deployed client and cannot
   be fixed retroactively.

2. **Executable filename** (`q3vr.exe`) — governs *which code actually runs
   after an update*, and is the **real migration unit**. `Sys_ApplyPendingUpdate`
   (`sys_win32.c` / `sys_unix.c`) applies a staged update before `Com_Init`: it
   moves each manifested file into place, and only sets `exeSwapped` (→
   relaunch) when it replaces *the currently-running* exe (`dstPath ==
   GetModuleFileName`). `Sys_RestartProcess` then relaunches via
   `GetModuleFileName(NULL,…)` — i.e. the **same filename** that was running; it
   cannot switch the launch target from `q3vr.exe` to `trinityvr.exe`.

The legacy ioquake3 updater in `code/autoupdater/autoupdater.c`
(signed manifests, `upd.ioquake3.org`) is **not** the active path and is out of
scope. The `code/vrcommon/vr_updates.c` checker (`RippeR37/q3vr`) is dead code,
also out of scope.

### Why an asset-name-only bridge backfires

Because the exe was renamed, duplicating only the *zip name* is worse than a
clean break. An existing `q3vr.exe` client:

1. Finds `q3vr-*.zip` (discovery works), stages the payload.
2. The staged manifest lists `trinityvr.exe`, DLLs, paks — **no `q3vr.exe`**.
3. On apply, `trinityvr.exe` lands *next to* the still-running `q3vr.exe`; the
   exe-swap check never matches → no relaunch into new code.
4. The user keeps launching the **old `q3vr.exe`**, which reports its **old**
   compiled version → sees the release as "newer" *every startup* →
   re-downloads and re-applies forever. **An update loop that never runs the new
   code.**

A working bridge therefore needs **two coordinated pieces**: dual asset *names*
**and** legacy exe *copies* in the payload.

### Verified migration path (full bridge)

An old `q3vr.exe` client (old code, prefix `q3vr`, repo `q3vr`):

1. Queries `ernie/q3vr` → GitHub 301-redirects to `ernie/trinity-vr`; curl
   follows it (`CURLOPT_FOLLOWLOCATION`). Expects `q3vr-…zip` → bridge provides
   it.
2. Stages a payload whose manifest now **includes `q3vr.exe`** (a copy of the
   new binary).
3. On apply, replaces its *own running* `q3vr.exe` → `exeSwapped` fires →
   relaunches into new code.
4. New code reports the new version, prefix `trinityvr`, repo `trinity-vr` →
   sees itself up-to-date → **no loop, no stranding**.

The user now runs new code (as `q3vr.exe`), with `trinityvr.exe` also present
for when they switch their shortcut. (`Update_GetCurrentVersion` does
`strrchr(engine,'/')`, so `com_engine = "trinity-vr/1.0.6"` parses correctly to
`1.0.6` — the hyphen is harmless.)

## Decisions (locked)

- **Asset name:** `trinityvr` (matches `CLIENT_NAME` / `PROJECT_NAME` / the exe;
  no third spelling).
- **Repo:** already renamed to `ernie/trinity-vr`; `UPDATE_GITHUB_REPO` updated
  to match (stops relying on the redirect). Owner stays `ernie`.
- **`com_engine`:** `trinity-vr/<ver>` (hyphenated repo spelling).
- **Bridge:** full bridge (dual asset names + legacy exe copies), differentiated
  zips — legacy exe only in the `q3vr-*.zip`, clean `trinityvr-*.zip`.
- **Install base:** small; a straggler that never updates during the bridge
  window gets a one-time manual reinstall (acceptable).

## Changes

### A. `code/qcommon/autoupdate.c` — the contract

| `#define` | Old | New |
|-----------|-----|-----|
| `UPDATE_GITHUB_REPO` | `q3vr` | `trinity-vr` |
| `UPDATE_ASSET_PREFIX` | `q3vr` | `trinityvr` |
| `UPDATE_GITHUB_OWNER` | `ernie` | unchanged |

Also update the stale comment examples: `Update_GetCurrentVersion` doc
(`"q3vr/vX.Y.Z"` → `"trinity-vr/vX.Y.Z"`) and `Update_BuildAssetName` example
(`"q3vr-windows-msvc-x86_64.zip"` → `"trinityvr-windows-msvc-x86_64.zip"`).

### B. `code/qcommon/common.c` — cosmetic

`Cvar_Get( "com_engine", va( "q3vr/%s", Q3VR_VERSION ), CVAR_ROM )` →
`va( "trinity-vr/%s", Q3VR_VERSION )`. Feeds the HTTP User-Agent and the engine
identity string; the updater parses only the numeric part after `/`, so this is
branding-only and safe.

### C. `.github/workflows/release.yml` — build jobs (fix already-broken globs)

The binary rename in `de5db407` left the upload-artifact globs stale; with
`if-no-files-found: error` the Windows jobs fail on the next release regardless
of this rename. In all three build jobs (windows-msvc, windows-mingw, linux):

- `build/Release/**/q3vr.exe` → `build/Release/**/trinityvr.exe`
- `build/Release/**/glq3vr.exe` → `build/Release/**/gltrinityvr.exe`
- (linux) `build/Release/**/q3vr` → `build/Release/**/trinityvr`,
  `build/Release/**/glq3vr` → `build/Release/**/gltrinityvr`

### D. `.github/workflows/release.yml` — release job (the bridge)

After the existing asset/script bundling steps and **before** "Create archives",
add a step that makes legacy-named copies of the new binaries in each platform
dir:

- windows-msvc, windows-mingw: copy `trinityvr.exe` → `q3vr.exe`,
  `gltrinityvr.exe` → `glq3vr.exe`
- linux: copy `trinityvr` → `q3vr`, `gltrinityvr` → `glq3vr`

Then rework "Create archives" so the `q3vr-*.zip` (bridge) archive contains the
legacy copies and the `trinityvr-*.zip` archive does not. Pattern per platform:

```bash
# legacy copies present → zip the q3vr-named (bridge) archive
7z a -r q3vr-windows-msvc-x86_64.zip      ./windows-msvc/*
# remove legacy copies → zip the clean trinityvr-named archive
rm ./windows-msvc/q3vr.exe ./windows-msvc/glq3vr.exe
7z a -r trinityvr-windows-msvc-x86_64.zip ./windows-msvc/*
```

Repeat for `windows-mingw` (`q3vr.exe`/`glq3vr.exe`) and `linux` (`q3vr`/`glq3vr`,
no extension). The `softprops/action-gh-release` `files: *.zip` glob already
uploads all six archives.

Rationale for differentiated zips: old clients need `q3vr.exe` in the manifest
to trigger the exe swap; new installs should not inherit a stray `q3vr.exe`.

### E. Bridge retirement (documented follow-up, not coded now)

After ~2–3 releases (or a chosen date), delete step D's legacy-copy step and the
three `q3vr-*.zip` archive lines, leaving only `trinityvr-*.zip`. From then on,
any old-code `q3vr.exe` that never updated during the window gets a one-time
"asset not found" and must reinstall. Tracked so the bridge does not linger
indefinitely.

## The invariant

For each platform, `UPDATE_ASSET_PREFIX` in code must match the
`trinityvr-*.zip` filename in `release.yml`. During the bridge window, *also*
keep emitting `q3vr-*.zip` plus the legacy exe copies so deployed old clients can
cross over.

## Out of scope (recorded)

- `code/vrcommon/vr_updates.c` dead `RippeR37/q3vr` checker — untouched.
- `code/autoupdater/autoupdater.c` legacy ioquake3 updater — not the active path.
- `pakQ3VR.pk3` asset name, `Q3VR_VERSION*` cmake var / compile defines,
  `win_resource.rc` `IDS_STRING1 "Quake3"` — internal/asset identifiers
  unrelated to release artifacts.

## Testing / verification

- `workflow_dispatch` a test release; confirm six archives publish — both
  `q3vr-*.zip` (containing `q3vr.exe`/`glq3vr.exe`) and `trinityvr-*.zip` (no
  `q3vr.exe`) for each platform.
- Fresh `trinityvr` install, run with `+set logfile 1`: log shows
  `Update: checking ernie/trinity-vr…` and it finds
  `trinityvr-windows-msvc-x86_64.zip` (or cleanly reports up-to-date).
- If feasible, an old `q3vr.exe` install: confirm it updates once, relaunches,
  then reports up-to-date with **no** re-download loop.

## Risks

- A `q3vr.exe` that never launches during the bridge window is never migrated and
  is stranded when the bridge retires (one-time manual reinstall). Acceptable for
  the small install base.
- Bridge releases carry duplicate archives and an extra pair of legacy exes —
  modest, transient size cost.
