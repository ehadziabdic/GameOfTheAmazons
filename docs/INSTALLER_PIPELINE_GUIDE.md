# natID Installer Pipeline Guide

**Self-contained, memory-free guide** for applying the cross-platform CI/CD installer pipeline to any natID project.

---

## What This Pipeline Produces

| Platform | Artifact | Contents |
|----------|----------|----------|
| Windows | `GameOfTheAmazons-win.zip` | `.msi` + `Install_*.exe` (keep together) |
| macOS Silicon | `GameOfTheAmazons-macOS-Silicon.zip` | `.app` bundle (M1/M2/M3/M4) |
| macOS Intel | `GameOfTheAmazons-macOS-Intel.zip` | `.app` bundle (2016-2020 Intel Macs) |
| Linux | `GameOfTheAmazons-linux.zip` | `.deb` package |

All artifacts published to **GitHub Releases** on tag push.

---

## Files to Copy to New Project

```
.github/workflows/release-all.yml    # Main CI workflow (edit project-specific values)
packaging/<ProjectName>.xml          # SetupCollector config (edit paths/names)
packaging/GTK4.xml                   # Patched GTK4 package manifest (commit this!)
res/DevRes.xml                       # Product metadata (MUST have displayName, etc.)
```

---

## Project-Specific Substitutions

In `release-all.yml`, replace:
- `GameOfTheAmazons` → your project name (everywhere: artifact names, paths, release body)
- `AmazonsGame` → your CMake target / executable name
- `ehadziabdic/GameOfTheAmazons` → your GitHub repo
- `ehadziabdi1@etf.unsa.ba` → your email
- `https://github.com/ehadziabdic/GameOfTheAmazons` → your repo URL

In `packaging/<ProjectName>.xml`:
- `executableName="AmazonsGame"` → your executable name
- `dev="~Desktop/GameOfTheAmazons"` → your project folder name
- `executable="$RAMDisk/Out/AmazonsGame/Release"` → your build output path
- `out="$RAMDisk/Setup/AmazonsGame"` → your staging output path
- `lin:icon="$HOME/Desktop/GameOfTheAmazons/res/appIcon/lnxApp128"` → your Linux icon path

In `res/DevRes.xml`:
- `displayName="GameOfTheAmazons"` → your app name (determines .app bundle name on macOS!)
- `maintainer`, `email`, `homePage`, `majorVersion`, `minorVersion`, `versionDate` → your info

---

## 10 Critical Rules (Exact Failure Symptoms)

| # | Rule | If Violated → Symptom |
|---|------|----------------------|
| 1 | **Patch SDK's GTK4.xml in-place** — copy `packaging/GTK4.xml` over `$HOME/natID.SDK/DevEnv/SetupCollectors/Packages/GTK4.xml` in CI | SetupCollector uses `GTK/release/bin` (wrong), DLLs not found, installer missing GTK |
| 2 | **GTK4.xml has line break between XML attributes** — regex must handle multi-line `<Group type="dynLib"\n  path=...>` | Single-line regex silently fails, patch doesn't apply, Windows installer broken |
| 3 | **DevRes.xml root MUST have all 7 attributes** — `displayName`, `maintainer`, `email`, `homePage`, `majorVersion`, `minorVersion`, `versionDate` | "Product displayName cannot be empty!" error |
| 4 | **macOS .app bundle named after `displayName`** (from DevRes.xml), NOT `executableName` | Staging step can't find `.app`, codesign fails |
| 5 | **Windows installer output at `R:\Setup\`** (parent), not `R:\Setup\AmazonsGame\` | Staging step finds nothing, artifact upload fails |
| 6 | **SetupCollector loads Packages/*.xml from SDK dir** (`natID.SDK/DevEnv/SetupCollectors/Packages/`), NOT config dir | Local works (edited SDK), CI fails (pristine SDK) |
| 7 | **RAMDisk path per OS**: Win=`R:`, macOS=`/Volumes/RAMDisk`, Linux=`/media/RAMDisk` — create symlinks in CI | Build output not found, SetupCollector fails |
| 8 | **macOS: codesign + xattr -cr on .app BEFORE zip, xattr -c on zip AFTER** | Gatekeeper "damaged or incomplete" on download |
| 9 | **macOS: use `ditto -c -k --keepParent --sequesterRsrc`** (not `zip`) | Executable permissions lost, .app won't launch |
| 10 | **macOS: use `7zz` not `7z`** for LZMA2 SDK binaries | Extraction fails on macOS runners |

---

## Workflow Structure Per OS

### Windows (`windows-latest`)
```yaml
- checkout
- Install natID SDK (git clone + download bin_win_x64_20260710.7z + 7z extract)
- Build (cmake -B $HOME/natID.RAMDisk/build → cmake --build)
- Collect (copy SDK SetupCollectors, overwrite GTK4.xml, run SetupCollector)
- Stage (search R:\Setup\ recursively for *.msi, Install*.exe)
- Zip (Compress-Archive)
- Upload artifact
```

### macOS Silicon (`macos-latest`)
```yaml
- checkout
- Install natID SDK (git clone + 7zz extract bin_macOS_arm64_20260710.7z)
- Symlink checkout to ~/Desktop/GameOfTheAmazons (config expects it)
- Build (cmake → cmake --build)
- Collect (sudo ln -s $HOME/natID.RAMDisk /Volumes/RAMDisk, run SetupCollector)
- Stage (cp from $HOME/natID.RAMDisk/Setup/ALL_Bundles/ or /AmazonsGame/)
- **Re-sign & clear quarantine:**
  ```bash
  codesign --force --deep --sign - "installer-output/YourApp.app"
  codesign --verify --verbose=4 "installer-output/YourApp.app"
  xattr -cr "installer-output/YourApp.app"
  xattr -l "installer-output/YourApp.app"  # verify: no output = good
  ```
- **Create zip with ditto:**
  ```bash
  ditto -c -k --keepParent --sequesterRsrc YourApp.app YourApp-macOS-Silicon.zip
  xattr -c YourApp-macOS-Silicon.zip
  xattr -l YourApp-macOS-Silicon.zip  # verify: no output = good
  ```
- Upload artifact

### macOS Intel (`macos-15-intel`)
Same as Silicon but:
- SDK binary: `bin_macOS_x64_20260710.7z`
- SetupCollector: `$HOME/natID/natID.Utils/macOS/x64/SetupCollector`

### Linux (`ubuntu-24.04`)
```yaml
- checkout
- Install deps (cmake, build-essential, libgtk-4-dev, libadwaita-1-dev, p7zip-full)
- Install natID SDK (git clone + 7zz extract bin_linux_x64_20260710.7z)
- Build
- Collect (sudo ln -s $HOME/natID.RAMDisk /media/RAMDisk, run SetupCollector)
- Stage (find .deb)
- Zip
- Upload artifact
```

### Release Job
- Downloads all 4 artifacts
- Creates GitHub Release with `softprops/action-gh-release@v2`
- Release body includes **macOS quarantine warning** (see below)

---

## macOS Gatekeeper Details (Rule 8)

**The Problem:** GitHub adds `com.apple.quarantine` to downloads. Browser (Safari) auto-extracts zip → quarantine propagates to `.app`. Ad-hoc signing alone is insufficient.

**The Fix (in CI):**
1. `codesign --force --deep --sign - "YourApp.app"` — ad-hoc signature
2. `xattr -cr "YourApp.app"` — clear quarantine from .app recursively
3. `ditto -c -k --keepParent --sequesterRsrc` — create zip preserving exec perms
4. `xattr -c "YourApp.zip"` — clear quarantine from zip

**User Must Still Run After Download:**
```bash
xattr -cr /Applications/YourApp.app
```
(No output = success. This is unavoidable without Apple Developer notarization.)

**Release Notes Must Include:**
> ⚠️ **macOS**: Safari auto-extracts zips on download, re-applying quarantine. After moving to Applications, run `xattr -cr /Applications/YourApp.app` in Terminal (no output = success). If "damaged or incomplete" appears, the flag wasn't cleared — run the command.

---

## Linux Notes

- `lin:depends="$DepGTKv4"` in SetupCollector config → auto-generates `.deb` with GTK4 dependency
- Runner must have `libgtk-4-dev`, `libadwaita-1-dev` for build
- `.deb` installs via `apt` (pulls runtime deps automatically)

---

## Debugging Tips

| Issue | Check |
|-------|-------|
| "Product displayName cannot be empty!" | DevRes.xml missing required attributes |
| Windows installer missing GTK DLLs | GTK4.xml patch not applied to SDK dir (Rule 1, 2) |
| macOS staging finds no .app | Wrong name — must match DevRes.xml `displayName` (Rule 4) |
| Windows staging finds no .msi | Search `R:\Setup\` not `R:\Setup\Project\` (Rule 5) |
| macOS "damaged or incomplete" | Quarantine not cleared in CI OR user didn't run xattr post-download (Rule 8) |
| Linux .deb missing | SetupCollector `lin:depends` not set, or GTK4 package excluded |
| CI upload fails "no files found" | Staging step output path wrong, check `ls -laR` logs |

---

## Quick Test Checklist Before Tagging

- [ ] Windows: `.msi` + `.exe` in artifact, installs cleanly
- [ ] macOS Silicon: `.app` launches after `xattr -cr` (test on real Mac if possible)
- [ ] macOS Intel: same (or use universal binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`)
- [ ] Linux: `.deb` installs via `apt`, runs
- [ ] Release body has macOS quarantine warning
- [ ] README install table matches artifact names

---

## Applying to Another natID Project

1. Copy the 4 files listed above to new repo
2. Run find/replace for project-specific values (see substitutions section)
3. Commit `packaging/GTK4.xml` (patched version!)
4. Push, create a tag (e.g., `v1.0.0`)
5. Watch CI, verify all 4 artifacts in Release
6. Test each platform (especially macOS with `xattr -cr`)

---

*Generated from GameOfTheAmazons pipeline — works with natID SDK 4.2.1*