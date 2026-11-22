# AGENTS.md

Maintained fork of TaskSwitchXP (WinSwitch): a Win32 C++ Alt-Tab replacement, built for both 32-bit and 64-bit Windows. No tests, no lint, no CI. Verification is a build plus manual use of the switcher.

## Agent workflow

The default agent is `architect` (`.opencode/agents/architect.md`). It plans and delegates: `developer` (`mode: subagent`) makes the edits and verifies builds; `reviewer` (`mode: subagent`) only inspects and reports findings — it has no `edit` permission and a read-only shell allowlist. Do not bypass this split: hand implementation to `developer`, then ask `reviewer` to check the result. Config lives in `.opencode/opencode.jsonc`; existing sessions keep their selected agent, so `/agent architect` (or a new session) is needed to pick it up.

## Build

MSBuild (verified working):

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  TaskSwitchXP.sln /p:Configuration=Release /p:Platform=x86 /t:Build
```

Swap `Release` for `Debug` for the ~1.1 MB debug build with asserts and `/RTC1`. Both output `TaskSwitchXP.exe` (Release ~180 KB) in the matching `Debug\`/`Release\` directory.

64-bit builds (also verified) use the same solution with `/p:Platform=x64` and land in `x64\Debug\` / `x64\Release\` (Release ~220 KB):

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  TaskSwitchXP.sln /p:Configuration=Release /p:Platform=x64 /t:Build
```

Scripted alternatives (also verified): `cmd /c do_compile.cmd` then `cmd /c do_link.cmd` for the debug build, `cmd /c do_compile_release.cmd` then `cmd /c do_link_release.cmd` for the release build, or `cmd /c do_compile_release_x64.cmd` then `cmd /c do_link_release_x64.cmd` for the x64 release build (output in `x64\Release\`). The x64 pair uses the `Hostx64\x64` tools and `/MACHINE:X64` with no `/SAFESEH`. The scripts do not call `vcvars*`; they hardcode `VS2022\...\MSVC\14.40.33807` and `Windows Kits\10.0.22621.0` in `PATH`/`INCLUDE`/`LIB`. Update those paths in all six scripts if the toolchain moves, and keep the release scripts in sync with the project's link settings.

- Both configurations start through the CRT (`wWinMain` in `main.cpp`) and link the static CRT, so the exe needs no VC++ redistributable. Do **not** reintroduce `/ENTRY:_myWinMain` to save the ~14 KB of CRT startup: without CRT init the legacy `wcstok` in `_AddNewTaskWindow` faults (0xC0000005) because it needs CRT per-thread data, `MyEnumDesktopWindows` catches the fault and reports zero tasks, and the switcher never opens while the tray icon still does.
- Both Win32 and x64 are supported. `res/TaskSwitchXP.manifest` uses `processorArchitecture="*"` so one manifest works for both (the Common-Controls dependency too); do not put `X86` back or 64-bit builds fail to activate their context. The x86 scripts link `/MACHINE:X86` (with `/SAFESEH`), the x64 scripts `/MACHINE:X64` (no `/SAFESEH`), and the project configs pick the machine from the platform.
- `/Debug`, `/Release`, `/x64` and `/.vs` are gitignored. Never commit a built `.exe` (the previously tracked root exe was intentionally deleted in `f5d0f09`); do not restore `TaskSwitchXP.exe` in the repository root.

## Testing and runtime quirks

- No automated tests. Run `Debug\TaskSwitchXP.exe` (or the `Release\` build), press the hotkey from the README (or `/list`, `/inst-list`), and exercise the list/preview/menus manually.
- Second launch does not start a second copy: `main.cpp` finds the hidden main window and either offers to kill the running instance or forwards `/list`, `/inst-list`, `/escape`, `/nocheckver` as `WM_REMOTECMD`. Kill the running/installed instance before swapping binaries.
- Startup calls `CheckVersion()` and aborts with "requires Windows XP/2003" unless `/nocheckver` is passed. It uses deprecated `GetVersionEx`.
- Settings are registry-only here: `HKCU\Software\Alexander Avdonin\TaskSwitchXP\2.0`; names and bit flags are in `registry.h`. The `ConfigTsXP` settings UI exists only under `orig-src/` and is not built by this solution.
- `PreviewCaptureMode` (`RS_PVCAPTUREMODE` in `registry.h`) controls preview capture: `0` = legacy single `PrintWindow(hwnd, hdc, 0)`, `1` = candidate cascade with frame validation (default when missing/invalid), `2` = same cascade plus per-candidate `OutputDebugString` diagnostics (HWND/pid/process, window/DWM/native rects, flags, native and preview sizes, timing, score, selected flags). Diagnostic mode is silent in `0`/`1`; the icon fallback in `DrawTaskPreview` is used only when no `PrintWindow` candidate succeeds. Full-content candidates (`PW_RENDERFULLCONTENT` `0x2` and diagnostic `0x3`) are captured into a DIB sized to the window's native physical bounds (DWM extended frame bounds via dynamic `dwmapi.dll`, fallback `GetWindowRect`) with `GM_COMPATIBLE`/`MM_TEXT` and identity XFORM, then downscaled with `StretchBlt` into the preview; the legacy candidate `0` stays preview-sized with the window-to-preview `XFORM` and is copied 1:1 with `BitBlt`. WGC/WinRT/D3D11/DXGI are deliberately not used at this stage, and no new `dwmapi.lib` dependency is added (DWM frame bounds are queried dynamically for capture sizing and logging only).
- `LangLoadString` prefers `lang\<LangFile>.lng` next to the exe; the file must be UTF-16LE with BOM and contain `ID="text"` entries with IDs >= 31000. Otherwise it falls back to the `.rc` string table.

## Code map

- `main.cpp` — entrypoint, command line, hidden message window, tray icon, global hotkeys, command dispatch.
- `TaskSwitchXP.cpp` — the switcher window: task list layout/painting, Alt-Tab state machine, type-to-filter, hotkey hint. Keyboard handling lives in the block marked `//klvov: alternative keyboard handling proc` (~line 3739).
- `generic.cpp` — window enumeration/icons/wallpaper/tray helpers; `MyPrintWindow` renders preview thumbnails via a `PrintWindow` candidate cascade: `PW_RENDERFULLCONTENT` on Win8.1+ is captured at native window size and downscaled into the preview, then legacy flags `0` keeps the preview-sized XFORM path; lightweight frame scoring/selection and a fallback to legacy or the task icon are included. Non-task-preview captures (`MPW_DESKTOP` backgrounds and the tray-window check) keep the plain `PrintWindow(..., 0)` path.
- `tscontrol.cpp` — list operations: select/multi-select, close, minimize, restore/maximize, cascade/tile, sort.
- `registry.h` — all setting names and `TSF_*`/`TSFL_*` flag bits.
- `TaskSwitchXP.rc` + `res/` — version info (2.0.11.0), manifest, icons. `orig-src/` is the untouched upstream source (VS2005 projects): reference only, do not edit or build.

## Gotchas

- Both `Debug` and `Release` use `wWinMain` from `main.cpp`, so CRT startup runs normally. Keep it that way: this code relies on CRT state (per-thread data, `_initterm`, security cookie), and Release-only "works but does nothing" bugs are easy to introduce by bypassing it.
- `_AddNewTaskWindow` falls back to `QueryFullProcessImageName` when `EnumProcessModulesEx` fails (normal for 64-bit targets seen from the Win32 build, and for WOW64 targets seen from the x64 build). `MyEnumDesktopWindows` catches access violations and reports "no tasks", so a fault on that path shows up as "the switcher does nothing" instead of a crash. Keep that fallback free of CRT-state-dependent helpers.
- If a configured hotkey is already registered system-wide, `LoadSettings` (running inside `WM_CREATE`) reports the error with `OutputDebugString` instead of a modal box, so startup is not blocked. The same applies to hotkey/hook registration failures in `ReplaceAltTab` and `HotKeyError`. Other `ReportError` dialogs (e.g. `IDS_ERR_EXISTS`, `IDS_ERR_CONFIGTSXP`, `IDS_ERR_LOADSETTINGS`, `IDS_ERR_LANGFILE`) remain modal. Check `HKCU\...\2.0` hotkey values and debug output before blaming the build.
- Keep the code arch-neutral: MSVC does not support `__asm` on x64, so the x87 blocks that used to compute the preview/wallpaper `XFORM` in `generic.cpp` and `TaskSwitchXP.cpp` are now plain C (checked numerically against the old asm for 32-bit). Use `DWORD_PTR`/`LONG_PTR` for `GetWindowLongPtr` results and `SendMessageTimeout` out-params; a `DWORD*` fails to compile for x64.
- Minimum OS is set in `stdafx.h` (`_WIN32_WINNT 0x0600`, Vista). The Release project also passes `0x0501`, so the C4005 "macro redefinition" warning is expected; change the version in `stdafx.h` only.
- Release uses a `stdafx.h` PCH; Debug compiles without one. Keep `#include "stdafx.h"` first in every `.cpp`.
- Files are CRLF; `generic.cpp` contains a few LF-only lines from the snapshot commit. Edit in place rather than normalizing line endings or reformatting whole files.
- Root `afxres.h` is a local MinGW shim included by `TaskSwitchXP.rc` because MFC is not installed. Keep it.
- `TaskSwitchXP.vcproj` is a stale VS2005 file; the solution builds `TaskSwitchXP.vcxproj`.
- Sources are UTF-8 without BOM; MSVC builds with `/utf-8` and RC with `/c65001`. `orig-src/` has been converted to UTF-8 too but remains reference only: do not edit or build it.
