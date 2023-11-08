CPPFLAGS = /O2 /W3 /D _CRT_SECURE_NO_WARNINGS /nologo

all : midilog.exe

midilog.exe : midilog.cpp
	$(CPP) $(CPPFLAGS) $**

clean :
	@for %i in (midilog.obj midilog.exe) do @if exist %i del %i | echo del %i
