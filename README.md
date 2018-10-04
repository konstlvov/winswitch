#WinSwitch: Windows Alt-Tab manager

Based on the original Alexander Avdonin <a href="https://www.ntwind.com/software/alttabter/old-taskswitchxp.html">TaskSwitchXP</a>.

* Copyright 2004-2006 by Alexander Avdonin. All rights reserved.
* Copyright 2007-2018 by Konstantin Lvov. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

Neither the name of the author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Usage
Install TaskSwitchXP from the original installer, then replace TaskSwitchXP.exe with version from
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
