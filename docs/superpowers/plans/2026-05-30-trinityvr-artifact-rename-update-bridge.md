# q3vr → trinityvr Artifact Rename + Auto-Update Bridge Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename published release assets and the auto-updater contract from `q3vr` to `trinityvr`, repoint the updater at `ernie/trinity-vr`, and ship a transitional bridge so existing `q3vr.exe` installs migrate onto the new code without stranding or update loops.

**Architecture:** Three C-level constant/string edits change what *new* builds fetch (`autoupdate.c`, `common.c`). The release workflow (`release.yml`) is fixed to upload the already-renamed binaries, then publishes each platform's archive under *both* the legacy `q3vr-*` name (carrying legacy-named exe copies so old clients trigger their exe-swap-and-relaunch) and the clean `trinityvr-*` name. Bridge retirement is a documented follow-up, not coded here.

**Tech Stack:** C (ioquake3/Quake3e engine), GitHub Actions YAML, 7-Zip (`7z`) for archiving, CMake build via `./build.sh`.

**Spec:** `docs/superpowers/specs/2026-05-30-trinityvr-artifact-rename-update-bridge-design.md`

---

## File Structure

- `code/qcommon/autoupdate.c` — modify: 2 `#define`s + 2 doc-comment examples. The auto-update contract (`UPDATE_GITHUB_REPO`, `UPDATE_ASSET_PREFIX`).
- `code/qcommon/common.c` — modify: 1 line. `com_engine` cvar default string.
- `.github/workflows/release.yml` — modify: upload-artifact globs in 3 build jobs (fix already-broken `q3vr.exe` references); add a legacy-exe-copy step and rework the "Create archives" step in the release job (the bridge).

No new files. No new tests files (no test harness exists for these compile-time constants / CI workflow; verification is by grep-assertion, local release build, in-headset log line, and a CI dry-run).

---

## Task 1: Repoint the auto-update contract (`autoupdate.c`)

**Files:**
- Modify: `code/qcommon/autoupdate.c:57-63` (the two `#define`s), `:187` and `:221` (doc comments)

- [ ] **Step 1: Change `UPDATE_GITHUB_REPO`**

In `code/qcommon/autoupdate.c`, replace:

```c
#ifndef UPDATE_GITHUB_REPO
#define UPDATE_GITHUB_REPO "q3vr"
#endif
```

with:

```c
#ifndef UPDATE_GITHUB_REPO
#define UPDATE_GITHUB_REPO "trinity-vr"
#endif
```

- [ ] **Step 2: Change `UPDATE_ASSET_PREFIX`**

Immediately below, replace:

```c
#ifndef UPDATE_ASSET_PREFIX
#define UPDATE_ASSET_PREFIX "q3vr"
#endif
```

with:

```c
#ifndef UPDATE_ASSET_PREFIX
#define UPDATE_ASSET_PREFIX "trinityvr"
#endif
```

Leave `UPDATE_GITHUB_OWNER "ernie"` unchanged.

- [ ] **Step 3: Fix the stale doc-comment in `Update_GetCurrentVersion`**

At `code/qcommon/autoupdate.c:187`, replace the comment line:

```c
Extract version string from com_engine cvar ("q3vr/vX.Y.Z")
```

with:

```c
Extract version string from com_engine cvar ("trinity-vr/vX.Y.Z")
```

- [ ] **Step 4: Fix the stale doc-comment in `Update_BuildAssetName`**

At `code/qcommon/autoupdate.c:221`, replace the comment line:

```c
e.g., "q3vr-windows-msvc-x86_64.zip"
```

with:

```c
e.g., "trinityvr-windows-msvc-x86_64.zip"
```

- [ ] **Step 5: Verify no stray `q3vr` literals remain in the active update logic**

Run (Git Bash):
```bash
grep -n 'q3vr' code/qcommon/autoupdate.c
```
Expected: **no output** (every former `q3vr` occurrence — defines and the two comments — is now `trinity-vr`/`trinityvr`).

- [ ] **Step 6: Commit**

```bash
git add code/qcommon/autoupdate.c
git commit -m "autoupdate: point at ernie/trinity-vr, use trinityvr asset prefix"
```

---

## Task 2: Rebrand the `com_engine` string (`common.c`)

**Files:**
- Modify: `code/qcommon/common.c:2797`

- [ ] **Step 1: Change the `com_engine` default**

In `code/qcommon/common.c`, replace:

```c
	Cvar_Get( "com_engine", va( "q3vr/%s", Q3VR_VERSION ), CVAR_ROM );
```

with:

```c
	Cvar_Get( "com_engine", va( "trinity-vr/%s", Q3VR_VERSION ), CVAR_ROM );
```

This is safe for the updater: `Update_GetCurrentVersion` does `strrchr(engine,'/')` and returns the substring after the last `/`, so `"trinity-vr/1.0.6"` still yields `1.0.6` (the hyphen is before the slash and is ignored).

- [ ] **Step 2: Verify the edit**

Run (Git Bash):
```bash
grep -n 'com_engine' code/qcommon/common.c
```
Expected: the line shows `va( "trinity-vr/%s", Q3VR_VERSION )` and no `q3vr/` remains.

- [ ] **Step 3: Commit**

```bash
git add code/qcommon/common.c
git commit -m "common: rebrand com_engine string to trinity-vr"
```

---

## Task 3: Fix the already-broken release-build artifact globs (`release.yml`)

Context: commit `de5db407` renamed the binaries to `trinityvr.exe`/`gltrinityvr.exe` (and `trinityvr`/`gltrinityvr` on Linux), but the three build jobs still glob the old names with `if-no-files-found: error`, so the next release would fail. This task only fixes the filenames; the `build/Release/**/` path prefix is left exactly as-is (it already resolves correctly for all three generators today).

**Files:**
- Modify: `.github/workflows/release.yml` — windows-msvc job (~lines 36-37), windows-mingw job (~lines 65-66), linux job (~lines 111-112)

- [ ] **Step 1: Fix the windows-msvc upload globs**

In the `windows-msvc` job's `upload-artifact` `path:` block, replace:

```yaml
            build/Release/**/q3vr.exe
            build/Release/**/glq3vr.exe
```

with:

```yaml
            build/Release/**/trinityvr.exe
            build/Release/**/gltrinityvr.exe
```

- [ ] **Step 2: Fix the windows-mingw upload globs**

In the `windows-mingw` job's `upload-artifact` `path:` block, replace the same two lines (`q3vr.exe` / `glq3vr.exe`) with `trinityvr.exe` / `gltrinityvr.exe` (identical change to Step 1).

- [ ] **Step 3: Fix the linux upload globs**

In the `linux` job's `upload-artifact` `path:` block, replace (note: no `.exe` on Linux):

```yaml
            build/Release/**/q3vr
            build/Release/**/glq3vr
```

with:

```yaml
            build/Release/**/trinityvr
            build/Release/**/gltrinityvr
```

- [ ] **Step 4: Verify no `q3vr` binary names remain in the build-job globs**

Run (Git Bash):
```bash
grep -nE 'build/Release/\*\*/(gl)?q3vr' .github/workflows/release.yml
```
Expected: **no output** (all six glob lines now reference `trinityvr`/`gltrinityvr`).

- [ ] **Step 5: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "release: upload renamed trinityvr binaries from build jobs"
```

---

## Task 4: Add the auto-update bridge to the release job (`release.yml`)

The release job runs on `ubuntu-latest` with `bash` and `7z` available. Add a step that copies the new binaries to their legacy names, then rework "Create archives" so the `q3vr-*.zip` archives carry the legacy-named exe copies (old clients need `q3vr`/`q3vr.exe` in the manifest to trigger their exe-swap-and-relaunch) while the `trinityvr-*.zip` archives stay clean (new installs must not inherit a stray legacy exe).

**Files:**
- Modify: `.github/workflows/release.yml` — release job, between the existing "Bundle update scripts" step and the "Create archives" step (~lines 150-161)

- [ ] **Step 1: Add the "Create legacy-named binary copies (bridge)" step**

In the `release` job, immediately **after** the `Bundle update scripts` step and **before** the `Create archives` step, insert:

```yaml
      # Transitional bridge: old q3vr installs match the q3vr-* asset and need a
      # q3vr-named exe in the payload so Sys_ApplyPendingUpdate replaces their
      # running exe and relaunches into the new code. Remove this step (and the
      # q3vr-* archive lines below) once the install base has migrated.
      - name: Create legacy-named binary copies (bridge)
        run: |
          for dir in windows-msvc windows-mingw; do
            if [ -d "$dir" ]; then
              find "$dir" -name trinityvr.exe   -exec sh -c 'cp "$1" "$(dirname "$1")/q3vr.exe"'   _ {} \;
              find "$dir" -name gltrinityvr.exe -exec sh -c 'cp "$1" "$(dirname "$1")/glq3vr.exe"' _ {} \;
            fi
          done
          if [ -d linux ]; then
            find linux -name trinityvr   -exec sh -c 'cp "$1" "$(dirname "$1")/q3vr"'   _ {} \;
            find linux -name gltrinityvr -exec sh -c 'cp "$1" "$(dirname "$1")/glq3vr"' _ {} \;
          fi
```

(`find … -exec sh -c` is used because the binaries may sit in a nested subdirectory of the artifact dir, matching how the existing "Bundle Trinity assets" step locates `baseq3`/`missionpack`.)

- [ ] **Step 2: Replace the "Create archives" step with the differentiated dual-name version**

Replace the existing step:

```yaml
      - name: Create archives
        run: |
          7z a -r q3vr-windows-msvc-x86_64.zip    ./windows-msvc/*
          7z a -r q3vr-windows-mingw-x86_64.zip   ./windows-mingw/*
          7z a -r q3vr-linux-x86_64.zip            ./linux/*
```

with:

```yaml
      - name: Create archives
        run: |
          # Bridge (q3vr-*) archives include the legacy-named exe copies made above.
          7z a -r q3vr-windows-msvc-x86_64.zip    ./windows-msvc/*
          7z a -r q3vr-windows-mingw-x86_64.zip   ./windows-mingw/*
          7z a -r q3vr-linux-x86_64.zip           ./linux/*

          # Remove the legacy-named copies so the canonical (trinityvr-*) archives stay clean.
          find windows-msvc  -name 'q3vr.exe'   -delete
          find windows-msvc  -name 'glq3vr.exe' -delete
          find windows-mingw -name 'q3vr.exe'   -delete
          find windows-mingw -name 'glq3vr.exe' -delete
          find linux         -name 'q3vr'       -delete
          find linux         -name 'glq3vr'     -delete

          7z a -r trinityvr-windows-msvc-x86_64.zip  ./windows-msvc/*
          7z a -r trinityvr-windows-mingw-x86_64.zip ./windows-mingw/*
          7z a -r trinityvr-linux-x86_64.zip         ./linux/*
```

The existing `Create release` step's `files: *.zip` glob already uploads all six archives — no change needed there.

- [ ] **Step 3: Verify the workflow is syntactically well-formed and contains both archive name sets**

Run (Git Bash):
```bash
python -c "import yaml,sys; yaml.safe_load(open('.github/workflows/release.yml')); print('YAML OK')"
grep -cE '7z a -r q3vr-' .github/workflows/release.yml
grep -cE '7z a -r trinityvr-' .github/workflows/release.yml
```
Expected: `YAML OK`, then `3` and `3` (three bridge archives, three canonical archives).

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "release: dual-publish q3vr-* bridge + trinityvr-* archives"
```

---

## Task 5: Local build verification

Confirms the C edits compile and the produced binaries carry the renamed identity. (Runtime update behavior — the `Update: checking ernie/trinity-vr…` log line — can only be confirmed in-headset per the project's testing constraint; see Step 3.)

**Files:** none (verification only)

- [ ] **Step 1: Release build**

Run (Git Bash, repo root):
```bash
./build.sh
```
Expected: build completes without errors; `autoupdate.c` and `common.c` compile clean.

- [ ] **Step 2: Confirm the renamed binaries exist in the build output**

Run (Git Bash):
```bash
find build -name 'trinityvr.exe' -o -name 'gltrinityvr.exe' | sort
find build -name 'q3vr.exe' -o -name 'glq3vr.exe' | sort
```
Expected: the first command lists `trinityvr.exe` and `gltrinityvr.exe`; the second lists **nothing** (no legacy-named binaries are produced by the build itself — those copies only exist transiently inside the release job).

- [ ] **Step 3: In-headset runtime check (manual)**

Per the project testing constraint, launch the built client in the headset with `+set logfile 1`, then inspect `D:\Games\Quake 3 VR\baseq3\qconsole.log`. Expected: a line reading `Update: checking ernie/trinity-vr...` (and, if a `trinityvr-*` release asset exists, either an "available" or "already up to date" message — never "Asset … not found").

- [ ] **Step 4: No commit**

This task produces no source changes; nothing to commit.

---

## Task 6 (documented follow-up — not implemented now): Retire the bridge

Recorded so the bridge does not linger indefinitely. **Do not perform during this plan.** After ~2–3 releases (or a chosen trigger version/date), in `.github/workflows/release.yml`:

1. Delete the "Create legacy-named binary copies (bridge)" step (Task 4 Step 1).
2. In "Create archives", delete the three `7z a -r q3vr-*` lines and the six legacy `find … -delete` lines, leaving only the three `7z a -r trinityvr-*` lines.

After this, only `trinityvr-*.zip` ships; any old-code `q3vr.exe` that never updated during the window gets a one-time "Asset not found" and must reinstall.

---

## Self-Review

**Spec coverage:**
- Spec §A (autoupdate.c repo + prefix + comments) → Task 1 ✓
- Spec §B (com_engine) → Task 2 ✓
- Spec §C (build-job glob fix) → Task 3 ✓
- Spec §D (bridge: legacy copies + differentiated dual-name archives) → Task 4 ✓
- Spec §E (retirement, documented follow-up) → Task 6 ✓
- Spec "Testing / verification" (build, log line, archive contents) → Task 5 + Task 4 Step 3 ✓
- Spec "Out of scope" (vr_updates.c, autoupdater.c, pakQ3VR, Q3VR_VERSION, win_resource.rc) → not touched by any task ✓

**Placeholder scan:** No TBD/TODO/"handle edge cases"; every code/YAML step shows the exact before/after content. Task 6 is explicitly a non-implemented follow-up, not a placeholder.

**Type/string consistency:** `trinity-vr` (hyphenated) is used consistently for the repo (`UPDATE_GITHUB_REPO`) and `com_engine`; `trinityvr` (no hyphen) is used consistently for the asset prefix and binary names, matching `CLIENT_NAME`. Archive names (`trinityvr-<platform>-<arch>.zip`) match the platform/arch strings `Update_BuildAssetName` constructs from `UPDATE_ASSET_PREFIX`.
