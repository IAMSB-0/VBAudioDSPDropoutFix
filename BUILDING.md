# Building VB-Audio VoiceMeeter DSP Dropout Fix

This guide explains how to compile **VBAudioDSPDropoutFix** from source on Windows using **MSYS2 UCRT64**, GCC, and GNU `windres`.

---

## 🛠️ Prerequisites

- **OS:** Windows 10 or Windows 11 (64-bit)
- **Toolchain:** [MSYS2](https://www.msys2.org/) with the **UCRT64** environment (`gcc` and `windres`)
- **Editor *(Optional)*:** [Visual Studio Code](https://code.visualstudio.com/) (for single-shortcut task builds)

Open an **MSYS2 UCRT64** terminal and install the required toolchain packages:

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils
```

> [!NOTE]
> If `pacman -Syu` updates core MSYS2 runtime packages and closes the terminal, reopen **MSYS2 UCRT64** afterward and run the second command.

### Verified Build Environment

| Component | Environment / Version |
| :--- | :--- |
| **Toolchain** | MSYS2 UCRT64 (MinGW-w64 `x86_64`) |
| **Compiler** | GCC 16.1.0 (Rev5, MSYS2) |
| **Resource Compiler** | GNU `windres` (Binutils 2.46.1) |
| **Editor** | Visual Studio Code (`📦 Build : Secure Release`) |

*(Other 64-bit MinGW-w64 UCRT64 releases supporting PIE and High-Entropy VA are also compatible.)*

---

## 📂 Repository & Build Layout

Clone the repository and navigate to the project root:

```powershell
git clone [https://github.com/IAMSB-0/VBAudioDSPDropoutFix.git](https://github.com/IAMSB-0/VBAudioDSPDropoutFix.git)
cd VBAudioDSPDropoutFix
```

```text
VBAudioDSPDropoutFix/
├── .vscode/
│   └── tasks.json                  # Automated VS Code build pipeline
├── assets/
│   └── icons/
│       └── VB-Audio.ico            # Application icon embedded by resources.rc
├── build/                          # Generated automatically; holds VBAudioDSPDropoutFix.exe
├── src/
│   ├── main.c                      # Native Win32 C source
│   └── resources.rc                # UTF-8 icon & VERSIONINFO resource script
├── BUILDING.md
├── CHANGELOG.md
├── LICENSE
└── README.md
```

---

## ⚙️ Configure the Compiler `PATH`

To build from **VS Code** or **PowerShell**, ensure your MSYS2 UCRT64 `bin` directory is added to your Windows `PATH` environment variable (default installation path):

```text
C:\msys64\ucrt64\bin
```

Verify both tools are reachable from a new PowerShell window before building:

```powershell
gcc --version
windres --version
```

> [!IMPORTANT]
> Always invoke tools from the same **UCRT64** environment—do not mix binaries from `MINGW64`, `CLANG64`, or legacy MinGW installations.

---

## 🚀 Option 1: Build with Visual Studio Code (Recommended)

1. Open the repository root folder (`VBAudioDSPDropoutFix`) in **VS Code**.
2. Press <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>B</kbd> from any open tab.
3. If prompted, select **`📦 Build : Secure Release`**.

The `.vscode/tasks.json` pipeline executes four steps automatically:

1. Creates the `build/` output directory if it does not exist.
2. Runs `windres` inside `src/` to compile `resources.rc` into `build/resources.o`.
3. Compiles `src/main.c` and links `build/resources.o` into `build/VBAudioDSPDropoutFix.exe`.
4. Removes the temporary `build/resources.o` object file.

*(Note: `"version": "2.0.0"` inside `.vscode/tasks.json` refers to Microsoft's VS Code task-runner schema version, not the application version.)*

---

## 💻 Option 2: Build Manually via PowerShell

Run the following commands from the repository root:

```powershell
New-Item -ItemType Directory -Force -Path "build" | Out-Null

Push-Location "src"
windres -c 65001 resources.rc -o "../build/resources.o"
Pop-Location

gcc -fdiagnostics-color=always -Wall -Wextra -O2 -s -fPIE `
    src/main.c build/resources.o `
    -o build/VBAudioDSPDropoutFix.exe `
    -mwindows -pie `
    -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va

Remove-Item -LiteralPath "build/resources.o" -ErrorAction SilentlyContinue
```

<details>
<summary><strong>Alternative: Build directly inside MSYS2 UCRT64 Bash (no Windows PATH edit required)</strong></summary>

```bash
mkdir -p build
(cd src && windres -c 65001 resources.rc -o ../build/resources.o)
gcc -fdiagnostics-color=always -Wall -Wextra -O2 -s -fPIE \
    src/main.c build/resources.o \
    -o build/VBAudioDSPDropoutFix.exe \
    -mwindows -pie \
    -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va
rm -f build/resources.o
```

</details>

### Compiler & Linker Flags Reference

| Flag | Purpose |
| :--- | :--- |
| `-Wall -Wextra` | Enables strict compiler warnings for clean static checks |
| `-O2` | Optimizes execution speed and binary footprint without size bloat |
| `-s` | Strips symbol and debug tables from the release binary (preserving `.reloc` for ASLR) |
| `-mwindows` | Targets the Windows GUI subsystem to prevent console window flashes |
| `-fPIE` / `-pie` | Generates a Position-Independent Executable |
| `-Wl,--dynamicbase` | Enables Address Space Layout Randomization (ASLR) |
| `-Wl,--nxcompat` | Marks the binary compatible with Data Execution Prevention (DEP) |
| `-Wl,--high-entropy-va` | Enables 64-bit High-Entropy ASLR (`HIGH_ENTROPY_VA`) |

### Verify Binary Security Mitigations *(Optional)*

You can inspect the compiled PE64 header to confirm `HIGH_ENTROPY_VA`, `DYNAMIC_BASE`, and `NX_COMPAT` are enabled:

```powershell
objdump -p .\build\VBAudioDSPDropoutFix.exe | Select-String "DllCharacteristics" -Context 0,4
```

---

## 🏷️ Updating Version Metadata (For Maintainers)

Before tagging a new release, update both the numeric and string version fields in `src/resources.rc`:

```rc
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
```

```rc
VALUE "FileVersion",     "1.0.0.0"
VALUE "ProductVersion",  "1.0.0.0"
```

---

## 🧩 Build Troubleshooting

| Issue | Resolution |
| :--- | :--- |
| **`gcc` or `windres` not recognized** | Add `C:\msys64\ucrt64\bin` (or your custom MSYS2 UCRT64 path) to Windows `PATH` and restart VS Code / PowerShell. |
| **`../assets/icons/VB-Audio.ico` not found** | Ensure `assets/icons/VB-Audio.ico` exists and `windres` is invoked with `src/` as its working directory. |
| **Linker rejects `--high-entropy-va`** | Confirm `gcc -dumpmachine` outputs `x86_64-w64-mingw32` (64-bit UCRT64 toolchain, not 32-bit `i686`). |
| **Corrupted `©` symbol in File Properties** | Keep `#pragma code_page(65001)`, `windres -c 65001`, and Unicode block `040904B0` (`1200`) intact in `src/resources.rc`. |

---

## 📦 Preparing GitHub Release Assets

Compiled binaries (`build/` and `*.exe`) are excluded via `.gitignore` and distributed exclusively through [GitHub Releases](https://github.com/IAMSB-0/VBAudioDSPDropoutFix/releases) alongside `VBAudioDSPDropoutFix.xml`.

After compiling the final release binary, generate SHA-256 checksums for the release notes:

```powershell
Get-FileHash .\build\VBAudioDSPDropoutFix.exe, .\VBAudioDSPDropoutFix.xml -Algorithm SHA256
```

*(Always recalculate hashes after any rebuild or XML edit prior to publishing.)*