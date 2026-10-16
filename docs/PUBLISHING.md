# Releases

Public repository: [razbenaharon/TaskVitals](https://github.com/razbenaharon/TaskVitals), default branch `main`.

Version 1.7.0 is the first public release. The Windows download is the locally tested MinGW x86 portable build. Windows CI also builds and self-tests MSVC x86/x64; interactive GUI checks apply to the supplied MinGW binary. Source and binary archives are separate; generated artifacts are ignored by Git.

## Preparing a future release

1. Update the CMake version, README download filename, changelog and release notes.
2. Build and run the relevant tests in [TESTING.md](TESTING.md), including GPU off/on and placement checks when those features change.
3. Review screenshots, [verification](VERIFICATION.md), documented limits and staged files. Keep local paths and credentials out of the repository.
4. Package the executable with `scripts/package-release.ps1`; verify its checksum and extracted self-test.
5. Commit and push the source, then check Windows CI.
6. Archive the release commit with `git archive --format=zip --prefix=TaskVitals/ --output=dist/TaskVitals-VERSION-source.zip RELEASE_COMMIT` and create its SHA-256 sidecar.
7. Create a draft GitHub release targeting the exact release commit; upload both ZIPs and their checksums.
8. Download the draft assets, compare checksums, and publish the verified release as latest.

Keep the ready-to-run Windows ZIP distinct from source archives. Do not publish MSVC CI artifacts as GUI-tested downloads unless those exact binaries have undergone interactive checks.
