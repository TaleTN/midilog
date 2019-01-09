#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "winmm.lib")

// Two buffers might already be enough, but RtMidi uses four.
#define NUM_BUFS 4

// SysEx messages larger than BUF_SIZE will be split up.
#define BUF_SIZE 1024

MIDIHDR sysexHdr[NUM_BUFS];
CHAR sysexBuf[NUM_BUFS][BUF_SIZE];

int error(HMIDIIN const hMidiIn, const char* const err, const int ret = 0)
{
	if (hMidiIn)
	{
		midiInReset(hMidiIn);
		midiInStop(hMidiIn);

		for (int i = 0; i < NUM_BUFS; ++i)
		{
			midiInUnprepareHeader(hMidiIn, &sysexHdr[i], sizeof(MIDIHDR));
		}

		midiInClose(hMidiIn);
	}

	puts(err);
	return ret;
}

int error(HMIDIIN const hMidiIn, const MMRESULT err, const int ret = 0)
{
	char str[MAXERRORLENGTH];
	if (midiInGetErrorText(err, str, MAXERRORLENGTH) != MMSYSERR_NOERROR)
	{
		sprintf(str, "Unknown error 0x%08x, WTF?!", err);
	}

	return error(hMidiIn, str, ret);
}

void CALLBACK midiInProc(HMIDIIN const hMidiIn, const UINT wMsg, DWORD_PTR /* dwInstance */, const DWORD_PTR dwParam1, const DWORD_PTR dwParam2)
{
	switch (wMsg)
	{
		case MIM_DATA:
		case MIM_ERROR:
		{
			static const char chMsg[16] =
			{
				2, 2, 2, 2, 2, 2, 2, 2,
				2, 2, 2, 2, 1, 1, 2, 2
			};

			static const char sysMsg[16] =
			{
				2, 1, 2, 1, 2, 2, 0, 2,
				0, 2, 0, 0, 0, 2, 0, 0
			};

			const DWORD dwMidiMessage = (DWORD)dwParam1;
			const DWORD dwTimestamp = (DWORD)dwParam2;

			const BYTE status = (BYTE)dwMidiMessage;
			const BYTE data1 = (BYTE)(dwMidiMessage >> 8);
			const BYTE data2 = (BYTE)(dwMidiMessage >> 16);

			const BYTE buf[3] = { status, data1, data2 };
			const int n = status < 0xF0 ? chMsg[status >> 4] : sysMsg[status - 0xF0];

			if (wMsg == MIM_ERROR) printf("Error! ");
			printf("[%u]", dwTimestamp);

			for (int i = 0; i <= n; ++i)
			{
				putchar(' ');
				printf("%02X", buf[i]);
			}
			putchar('\n');
			break;
		}

		case MIM_LONGDATA:
		case MIM_LONGERROR:
		{
			LPMIDIHDR const lpMidiHdr = (LPMIDIHDR)dwParam1;
			const DWORD dwTimestamp = (DWORD)dwParam2;

			// It seems that lpMidiHdr == &sysexHdr[lpMidiHdr->dwUser], but
			// RtMidi explicitly uses global sysexHdr[lpMidiHdr->dwUser] in
			// certain cases, which might be safer?
			LPMIDIHDR const globalMidiHdr = &sysexHdr[lpMidiHdr->dwUser];

			// According to Microsoft docs midiInReset() "returns all
			// pending input buffers to [...] callback function", which
			// apparently means setting dwBytesRecorded to zero.
			if (!globalMidiHdr->dwBytesRecorded) break;

			if (wMsg == MIM_LONGERROR) printf("Error! ");
			printf("[%u]", dwTimestamp);

			for (DWORD i = 0; i < lpMidiHdr->dwBytesRecorded; ++i)
			{
				if ((i & 15) || !i) putchar(' '); else printf("\n\t");
				printf("%02X", (BYTE)lpMidiHdr->lpData[i]);
			}
			putchar('\n');

			midiInAddBuffer(hMidiIn, globalMidiHdr, sizeof(MIDIHDR));
			break;
		}
	}
}

void printCaps(const UINT uDeviceID, LPMIDIINCAPS const lpMidiInCaps)
{
	const BYTE majorVersion = (BYTE)(lpMidiInCaps->vDriverVersion >> 8);
	const BYTE minorVersion = (BYTE)lpMidiInCaps->vDriverVersion;

	printf("%u - %04X %04X %s v%u.%u\n", uDeviceID,
		lpMidiInCaps->wMid, lpMidiInCaps->wPid, lpMidiInCaps->szPname,
		majorVersion, minorVersion);
}

int midiLog(const UINT uDeviceID)
{
	MIDIINCAPS caps;
	MMRESULT err = midiInGetDevCaps(uDeviceID, &caps, sizeof(MIDIINCAPS));
	if (err != MMSYSERR_NOERROR) return error(NULL, err, 1);

	printCaps(uDeviceID, &caps);

	HMIDIIN hMidiIn;
	err = midiInOpen(&hMidiIn, uDeviceID, (DWORD_PTR)midiInProc, 0, CALLBACK_FUNCTION);
	if (err != MMSYSERR_NOERROR) return error(NULL, err, 2);

	for (int i = 0; i < NUM_BUFS; ++i)
	{
		LPMIDIHDR const hdr = &sysexHdr[i];

		hdr->lpData = sysexBuf[i];
		hdr->dwBufferLength = BUF_SIZE;
		hdr->dwUser = i;
		hdr->dwFlags = 0;

		err = midiInPrepareHeader(hMidiIn, hdr, sizeof(MIDIHDR));
		if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 3);

		err = midiInAddBuffer(hMidiIn, hdr, sizeof(MIDIHDR));
		if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 4);
	}

	err = midiInStart(hMidiIn);
	if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 5);

	for (;;) { if (_kbhit()) break; else Sleep(100);  }
	_getch();

	err = midiInReset(hMidiIn);
	if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 6);

	err = midiInStop(hMidiIn);
	if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 7);

	for (int i = 0; i < NUM_BUFS; ++i)
	{
		err = midiInUnprepareHeader(hMidiIn, &sysexHdr[i], sizeof(MIDIHDR));
		if (err != MMSYSERR_NOERROR) return error(hMidiIn, err, 8);
	}

	err = midiInClose(hMidiIn);
	if (err != MMSYSERR_NOERROR) return error(NULL, err, 9);

	return 0;
}

int main(const int argc, const char* const* const argv)
{
	int id = -1;

	if (argc == 2)
	{
		const int i = atoi(argv[1]);
		if (i || !strcmp(argv[1], "0")) id = i;
	}

	if (id < 0)
	{
		printf("Usage: %s <device ID>\n\n", argv[0]);

		const UINT n = midiInGetNumDevs();
		if (!n) return error(NULL, "Oops, no MIDI input devices", 2);

		MIDIINCAPS caps;
		for (UINT i = 0; i < n; ++i)
		{
			MMRESULT err = midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
			if (err == MMSYSERR_NOERROR)
			{
				printCaps(i, &caps);
			}
			else
			{
				printf("%u - ", i);
				error(NULL, err);
			}
		}
		return 1;
	}

	int ret = midiLog(id);
	if (ret) ret += 2;

	return ret;
}
