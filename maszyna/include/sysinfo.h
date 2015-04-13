//---------------------------------------------------------------------------

#ifndef sysinfoH
#define sysinfoH

#include "../commons.h"
#include "globals.h"


/*********************************************************************************************************************
WIN32 command line parser function
**********************************************************************************************************************/

inline int ParseCommandline()
{
	int    argc, BuffSize, i;
	WCHAR  *wcCommandLine;
	LPWSTR *argw;

	// Get a WCHAR version of the parsed commande line
	wcCommandLine = GetCommandLineW();
	argw = CommandLineToArgvW(wcCommandLine, &argc);

	// Create the first dimension of the double array
	Global::argv = (char **)GlobalAlloc(LPTR, argc + 1);

	// convert eich line of wcCommandeLine to MultiByte and place them
	// to the argv[] array
	for (i = 0; i < argc; i++)
	{
		BuffSize = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], -1, NULL, 0, NULL, NULL);
		Global::argv[i] = (char *)GlobalAlloc(LPTR, BuffSize);
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], BuffSize * sizeof(WCHAR), Global::argv[i], BuffSize, NULL, NULL);
	}

	// return the number of argument
	return argc;
}


/*********************************************************************************************************************
Retrieve file version infos
**********************************************************************************************************************/
inline BOOL GetAppVersion(char *LibName, WORD *MajorVersion, WORD *MinorVersion, WORD *BuildNumber, WORD *RevisionNumber)
{
	DWORD dwHandle, dwLen;
	UINT BufLen;
	LPTSTR lpData;
	VS_FIXEDFILEINFO *pFileInfo;
	dwLen = GetFileVersionInfoSize(LibName, &dwHandle);
	if (!dwLen)
		return FALSE;

	lpData = (LPTSTR)malloc(dwLen);
	if (!lpData)
		return FALSE;

	if (!GetFileVersionInfo(LibName, dwHandle, dwLen, lpData))
	{
		free(lpData);
		return FALSE;
	}
	if (VerQueryValue(lpData, "\\", (LPVOID *)&pFileInfo, (PUINT)&BufLen))
	{
		*MajorVersion = HIWORD(pFileInfo->dwFileVersionMS);
		*MinorVersion = LOWORD(pFileInfo->dwFileVersionMS);
		*BuildNumber = HIWORD(pFileInfo->dwFileVersionLS);
		*RevisionNumber = LOWORD(pFileInfo->dwFileVersionLS);
		free(lpData);
		return TRUE;
	}
	free(lpData);
	return FALSE;
}


/**********************************************************************************************************************
 Assign the current thread to one processor. This ensures that timing code runs on only one processor, 
 and will not suffer any ill effects from power management. 
 Based on DXUTSetProcessorAffinity() function from the DXUT framework. 
**********************************************************************************************************************/

inline void SetProcessorAffinity()
{

	DWORD_PTR dwProcessAffinityMask = 0;
	DWORD_PTR dwSystemAffinityMask = 0;
	HANDLE hCurrentProcess = GetCurrentProcess();

	if (!GetProcessAffinityMask(hCurrentProcess, &dwProcessAffinityMask, &dwSystemAffinityMask))
		return;

	if (dwProcessAffinityMask)
	{
		// Find the lowest processor that our process is allowed to run against.

		DWORD_PTR dwAffinityMask = (dwProcessAffinityMask & ((~dwProcessAffinityMask) + 1));

		// Set this as the processor that our thread must always run against.
		// This must be a subset of the process affinity mask.

		HANDLE hCurrentThread = GetCurrentThread();

		if (hCurrentThread != INVALID_HANDLE_VALUE)
		{
			SetThreadAffinityMask(hCurrentThread, dwAffinityMask);
			CloseHandle(hCurrentThread);
		}
	}

	CloseHandle(hCurrentProcess);
}

inline double CALCULATEFPS()
{
	double newTime = glfwGetTime();

	Global::frameTime = newTime - Global::previousFrameTime;

	Global::fdt = Global::frameTime;

	Global::previousFrameTime = newTime;

	Global::timeAccumulator += Global::frameTime;

	Global::frameCount++;

	// If our timeAccumulator passes 1 second, it means we should take a sample of our FPS
	if (Global::timeAccumulator > Global::fpsMeasureInterval)
	{
		Global::FPS = (double)Global::frameCount;
		Global::frameCount = 0;
		Global::timeAccumulator = 0;
	}
 return Global::FPS;
}

#endif
