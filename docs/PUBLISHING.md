# Publication plan

Prepared destination: `razbenaharon/TaskVitals`, public, default branch `main`.
No remote repository, push, release or public upload has been created by this preparation.

## Review before creating the repository

1. Read the root README and release notes; inspect the screenshots.
2. Check the license, [local verification results](VERIFICATION.md) and documented limits.
3. Confirm the repository owner/name and public visibility.
4. Review the staged files with `git diff --cached --stat` and `git diff --cached --check`.
5. Verify the ZIP checksum and run the packaged executable's self-test.

The first download is the locally tested x86 portable build. The configured MSVC workflow has not yet run. Source and binary archives are separate; executables and generated artifacts are ignored by Git.

## Create and push after approval

From the prepared repository, with an appropriate Git author identity configured:

```powershell
git commit -m "Prepare TaskVitals 1.7.0 public release"
gh repo create razbenaharon/TaskVitals --public --source . --remote origin --push --description "Lightweight native Windows CPU and RAM monitor with a transparent taskbar view."
```

Set repository topics: `windows`, `windows-11`, `cpu`, `ram`, `system-monitor`, `taskbar`, `lightweight`, `win32`, `cpp`, `desktop-widget`.

Check the Windows build workflow after pushing. It builds and self-tests MSVC x86/x64 and uploads portable build artifacts. It never publishes a release automatically.

## Create the release draft

After checking the pushed source and workflow results:

```powershell
gh release create v1.7.0 dist/TaskVitals-1.7.0-windows-x86.zip dist/TaskVitals-1.7.0-windows-x86.zip.sha256 --repo razbenaharon/TaskVitals --target main --draft --title "TaskVitals 1.7.0" --notes-file RELEASE_NOTES.md
```

Inspect the draft and download its asset to verify the file and checksum. Once approved:

```powershell
gh release edit v1.7.0 --repo razbenaharon/TaskVitals --draft=false --latest
```

The README download link becomes usable after the public repository and its first published release exist. Version `1.7.0` is the current publication candidate; there are no earlier public releases.
