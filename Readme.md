# Windows MIDI logger

Simple command-line MIDI logger for Windows, using the Win32 MME API.

## How to build

1. Open the Command Prompt for VS (Visual Studio).

2. Run the Program Maintenance Utility (NMAKE):

    `nmake`

## How to use

3. List available MIDI input devices:

    `midilog.exe`

4. Start logging MIDI messages for MIDI input device #0:

    `midilog.exe 0`

## License

Copyright &copy; 2019-2023 Theo Niessink &lt;theo@taletn.com&gt;  
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
