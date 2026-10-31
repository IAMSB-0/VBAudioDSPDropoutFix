<!-- markdownlint-disable MD024 -->
# Changelog

All notable changes to **VB-Audio VoiceMeeter DSP Dropout Fix** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-11-01

### Added

- **Core Native Utility:** Initial public release of `VBAudioDSPDropoutFix.exe`, a lightweight, zero-dependency C / Win32 utility designed to eliminate VoiceMeeter DSP audio dropouts and crackles.
- **VoiceMeeter Auto-Detection:** Automatic process discovery across VoiceMeeter Standard, Banana, and Potato (`x86` and `x64` executables, including `voicemeeter8x64.exe`).
- **Audio Graph Priority Elevation:** Automatic elevation of Windows Audio Device Graph Isolation (`audiodg.exe`) to `HIGH_PRIORITY_CLASS` via `SeDebugPrivilege`.
- **Smart Single-Core Affinity:** CPU affinity isolation for `audiodg.exe` targeting Logical Processor 4 (mask `0x10`), with automatic fallback to the highest available logical processor on systems with 4 or fewer threads.
- **VoiceMeeter Priority Boost:** Elevation of active VoiceMeeter processes to `ABOVE_NORMAL_PRIORITY_CLASS` while preserving full multi-core CPU affinity.
- **Delayed-Startup Polling:** Six-pass execution schedule (`0s`, `24s`, `48s`, `72s`, `96s`, and `118s`) to reliably capture delayed Windows audio service and VoiceMeeter initialization before exiting cleanly.
- **Single-Instance Guard:** Cross-session instance protection using the `Global\VBAudioDSPDropoutFix_SingleInstance` named mutex to prevent overlapping runs.
- **Silent Background Execution:** Windowless GUI-subsystem (`-mwindows`) binary complete with an embedded application icon and UTF-8 `VERSIONINFO` resource metadata.
- **Task Scheduler Integration:** Preconfigured Windows Task Scheduler template (`VBAudioDSPDropoutFix.xml`) supporting user logon and console session-connect triggers.
- **Developer Tooling & Docs:** Automated VS Code build pipeline (`.vscode/tasks.json`) and MSYS2 UCRT64 compilation guide (`BUILDING.md`).

### Security

- **Hardened PE64 Binary:** Compiled with Position-Independent Executable (`-fPIE` / `-pie`), ASLR (`--dynamicbase`), 64-bit High-Entropy VA (`--high-entropy-va`), and DEP (`--nxcompat`).
- **Least-Privilege Process Handles:** Scoped Win32 `OpenProcess` calls strictly to `PROCESS_SET_INFORMATION`, requesting `PROCESS_QUERY_LIMITED_INFORMATION` only when evaluating system CPU affinity masks.

[1.0.0]: https://github.com/IAMSB-0/VBAudioDSPDropoutFix/releases/tag/v1.0.0