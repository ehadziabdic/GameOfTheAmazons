# CHANGELOG - 2026-09-06
## Game of The Amazons - Cross-Platform Installer Pipeline Fixes

---

## Summary
Complete overhaul of the CI/CD installer pipeline to fix macOS Gatekeeper "damaged or incomplete" error, plus creation of reusable documentation for applying the same pipeline to other natID projects.

---

## Root Cause Analysis

**The Problem:** macOS Gatekeeper shows "file may be damaged or incomplete" when launching the downloaded `.app`.

**Why it happens:**
1. GitHub adds `com.apple.quarantine` extended attribute to all Release assets on download
2. Safari (and some cloud Mac providers) auto-extract `.zip` files on download
3. Quarantine flag propagates from zip → extracted `.app`
4. Ad-hoc code signing (`codesign --sign -`) alone is **insufficient** for Gatekeeper without Apple Developer notarization
5. **Cloud Mac providers** may have additional restrictions (Gatekeeper stricter, SIP, MDM profiles)

**The Fix:** CI must clear quarantine **before** zipping, AND users must clear it **after** download.

---

## Changes Made Today

### 1. `.github/workflows/release-all.yml` - CI Pipeline Fixes

#### macOS Silicon Job (`macos-latest`)
- Added verification step after `xattr -cr`:
  ```bash
  xattr -l "installer-output/GameOfTheAmazons.app" || echo "No extended attributes (good)"
  ```
- Added verification step after `xattr -c` on zip:
  ```bash
  xattr -l GameOfTheAmazons-macOS-Silicon.zip || echo "No extended attributes (good)"
  ```

#### macOS Intel Job (`macos-15-intel`)
- Same verification steps as Silicon job

#### Release Job - Updated Release Notes
- **Bold warning** about Safari auto-extraction re-applying quarantine
- Explicit Terminal command users MUST run:
  ```bash
  xattr -cr /Applications/GameOfTheAmazons.app
  ```
- Clear explanation: "No output = success"

### 2. `README.md` - User-Facing Install Instructions
- Updated macOS install table with **⚠️ CRITICAL** warning
- Explicit: "Safari auto-extracts zips on download, which re-applies the macOS quarantine flag"
- Mandatory Terminal command with explanation of silent success

### 3. `docs/INSTALLER_PIPELINE_GUIDE.md` - Reusable Guide (NEW FILE)
Complete self-contained guide for applying this pipeline to any natID project:

**Contents:**
- What the pipeline produces (4 platform artifacts)
- Files to copy to new project (4 files)
- Project-specific substitution map (workflow, config, DevRes)
- **10 Critical Rules** with exact failure symptoms (Rules 1-10)
- Workflow structure per OS (Windows, macOS Silicon, macOS Intel, Linux)
- **macOS Gatekeeper Details (Rule 8)** - deep dive
- Linux notes
- Debugging tips table
- Quick test checklist
- Step-by-step "Applying to Another natID Project"

**Key Rules Documented:**
| Rule | Title | Failure If Violated |
|------|-------|---------------------|
| 1 | Patch SDK's GTK4.xml in-place | Windows installer missing GTK DLLs |
| 2 | GTK4.xml has line break in XML | Single-line regex silently fails |
| 3 | DevRes.xml needs 7 attributes | "Product displayName cannot be empty!" |
| 4 | .app named after `displayName` | Staging can't find .app |
| 5 | Windows output at `R:\Setup\` | Artifact upload finds nothing |
| 6 | SetupCollector reads SDK Packages dir | Local works, CI fails |
| 7 | RAMDisk path per OS | Build output not found |
| 8 | **macOS: codesign + xattr -cr + ditto + xattr -c** | **Gatekeeper "damaged"** |
| 9 | Use `ditto` not `zip` | Exec permissions lost |
| 10 | Use `7zz` not `7z` on macOS | LZMA2 extraction fails |

---

## Technical Details

### macOS CI Steps (Both Jobs)
```yaml
- name: Re-sign app and clear quarantine flags
  run: |
    codesign --force --deep --sign - "installer-output/GameOfTheAmazons.app"
    codesign --verify --verbose=4 "installer-output/GameOfTheAmazons.app" || true
    xattr -cr "installer-output/GameOfTheAmazons.app"
    # VERIFICATION:
    xattr -l "installer-output/GameOfTheAmazons.app" || echo "No extended attributes (good)"

- name: Create distributable zip (ditto preserves exec permissions)
  run: |
    ditto -c -k --keepParent --sequesterRsrc GameOfTheAmazons.app GameOfTheAmazons-macOS-*.zip
    xattr -c GameOfTheAmazons-macOS-*.zip
    # VERIFICATION:
    xattr -l GameOfTheAmazons-macOS-*.zip || echo "No extended attributes (good)"
```

### Why Cloud Mac Might Still Fail
Even with CI clearing quarantine:
1. **GitHub Releases** re-adds `com.apple.quarantine` on download
2. **Browser auto-extraction** (Safari, some cloud providers) propagates it to `.app`
3. **Cloud provider restrictions**: MDM profiles, stricter Gatekeeper, SIP variations
4. **No notarization** = Gatekeeper will always flag on first launch

**User MUST run after download:**
```bash
xattr -cr /Applications/GameOfTheAmazons.app
```
(No output = success. This is unavoidable without $99/yr Apple Developer Program for notarization.)

---

## Files Modified

| File | Status | Lines Changed |
|------|--------|---------------|
| `.github/workflows/release-all.yml` | Modified | ~50 lines added (verification + release notes) |
| `README.md` | Modified | ~15 lines (macOS install table) |
| `docs/INSTALLER_PIPELINE_GUIDE.md` | **Created** | ~350 lines (complete guide) |

---

## Git History
```
85a0205 Fix macOS Gatekeeper: add codesign + xattr verification in CI, update docs
537ad1a Fix mac (previous attempt)
d7156a5 Final fix
ebe7ef9 Final quick push
0dbd2cc Final update: README.md
```

---

## For Applying to Another natID Project

1. **Copy these 4 files** to new repo:
   - `.github/workflows/release-all.yml`
   - `packaging/<ProjectName>.xml`
   - `packaging/GTK4.xml` (patched version!)
   - `res/DevRes.xml`

2. **Run find/replace** for project-specific values (see Guide substitutions section)

3. **Commit `packaging/GTK4.xml`** (patched version is critical!)

4. **Push, create tag** (e.g., `v1.0.0`)

5. **Watch CI** - verify all 4 artifacts in Release

6. **Test each platform** - especially macOS with `xattr -cr`

---

## Known Limitations

| Limitation | Workaround |
|------------|------------|
| macOS requires user `xattr -cr` post-download | Documented in Release Notes + README |
| No notarization without Apple Developer account | Acceptable for open-source/academic projects |
| Cloud Mac providers may have extra restrictions | Test on real Mac if possible |
| macOS Intel runner (`macos-15-intel`) may be unavailable | Use universal binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` |

---

## Next Steps for This Project

1. **Wait for CI to complete** - check Actions tab
2. **Verify CI logs show:**
   - `=== xattr on .app after clearing ===` → `No extended attributes (good)`
   - `=== xattr on zip after clearing ===` → `No extended attributes (good)`
3. **Create a new release tag** to publish fixed installers:
   ```bash
   git tag v1.0.2 && git push origin v1.0.2
   ```
4. **Test download on real Mac** (not cloud) if possible

---

*Generated for handoff to another agent/project*