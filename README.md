# WinSwitch: Windows Alt-Tab manager

Based on the original Alexander Avdonin <a href="https://www.ntwind.com/software/alttabter/old-taskswitchxp.html">TaskSwitchXP</a>.

* Copyright 2004-2006 by Alexander Avdonin. All rights reserved.
* Copyright 2007-2018 by Konstantin Lvov. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

Neither the name of the author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Usage
Install TaskSwitchXP from <a href="https://www.ntwind.com/software/alttabter/old-taskswitchxp.html">the original installer</a>, then replace TaskSwitchXP.exe with version from
this repository and import following .reg file to bind appearing of WinSwitch window to Ctrl-Win-K:

```ini
REGEDIT4

[HKEY_CURRENT_USER\Software\Alexander Avdonin\TaskSwitchXP\2.0]
"AlternateAppListHotKey"=dword:00000a4b
```

### Hotkeys
(search in .cpp files by phrase "alternative keyboard handling proc")
* Backspace - works as expected (back space current typing by one character)
* Enter - select (bring to front) current task
* Up, Down, Home, End - moving across task list, work as expected
* Insert - select current task (add it to current selection)
* Del - remove current task from list, but do not close it's window
* F1 - reset current typing and show all tasks
* F2 - minimize all selected tasks
* F4 - close all selected tasks
* F5 - update current task (currently does not work as expected)
* F6 - restore all selected tasks
* F7 - maximize all selected tasks
* Ctrl-A - select all
* Ctrl-Shift-A - select all instances of current task process (does not work)
* Ctrl-D - clear selection (mnemonic for D is "deselect all tasks")
* Ctrl-C - cascade selected windows
* Ctrl-H - tile selected windows horizontally
* Ctrl-V - tile selected windows vertically

### Hotkey info (from MSDN)
MOD_ALT       0x0001        Either ALT key must be held down.

MOD_CONTROL   0x0002        Either CTRL key must be held down.

MOD_NOREPEAT  0x4000        Changes the hotkey behavior so that the keyboard auto-repeat does not yield multiple hotkey notifications.
                            Windows Vista and Windows XP/2000:  This flag is not supported.

MOD_SHIFT     0x0004        Either SHIFT key must be held down.

MOD_WIN       0x0008        Either WINDOWS key was held down. These keys are labeled with the Windows logo.
                            Keyboard shortcuts that involve the WINDOWS key are reserved for use by the operating system.


Для комбинации функциональных клавиш используется побитовая операция OR. Так, для того, чтобы задать
сочетание Win-Alt, используется значение MOD_ALT | MOD_WIN, то есть 0x0001 | 0x0008, то есть 0x0009

* 0x41     A key
* 0x42     B key
* 0x43     C key
* 0x44     D key
* 0x45     E key
* 0x46     F key
* 0x47     G key
* 0x48     H key
* 0x49     I key
* 0x4A     J key
* 0x4B     K key
* 0x4C     L key
* 0x4D     M key
* 0x4E     N key
* 0x4F     O key
* 0x50     P key
* 0x51     Q key
* 0x52     R key
* 0x53     S key
* 0x54     T key
* 0x55     U key
* 0x56     V key
* 0x57     W key
* 0x58     X key
* 0x59     Y key
* 0x5A     Z key
