//---------------------------------------------------------------------------

#pragma hdrstop
#include "commons.h"
#include "commons_usr.h"
#include "trace.h"
#include <sstream>
#include <iomanip>

std::vector<std::string> supdates;
std::string xline;
std::string currentline;

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

// Get file size - getting file size after download for check pack integrity ******************************************
off_t getfsize(char *filename) {
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}

bool sil(std::string name)
{
	if (strstr(currentline.c_str(), name.c_str()))	return true; else return false;
}

bool CHECKUPDATES(bool check)
{

	std::string line;

	if (check)
	{
		WriteLog("Checking updates...");
		downloadfile("/!NOVAMASZYNA/updates.txt", "updates.txt", 110, 0);

		std::ifstream file("updates/updates.txt");
		

		
		while (std::getline(file, line))
		{

			int isrevline;
			isrevline = line.find('@');
			if (isrevline <0)
			{
				supdates = split(line, ',');
				WriteLog(line.c_str());

				supdates[0] = trim(supdates[0]);
				supdates[1] = trim(supdates[1]);
				supdates[2] = trim(supdates[2]);

				downloadfile(supdates[0], supdates[1], stoi(supdates[2]), 1);
			}
			else WriteLogSS("UPDATE:", line.substr(1,255));
		}
		
		Sleep(3200);
		Beep(5300, 600); Sleep(100);
		Beep(5300, 600); Sleep(100);

		WriteLog("Applying updates...");
		WriteLog("");
		DeleteFile("updates.txt");
		Sleep(1200);
	}
	return true;
}

// Drawing download progress bar **************************************************************************************
void DoProgress(char label[], int step, int total)
{
	//progress width
	const int pwidth = 72;

	//minus label len
	int width = pwidth - strlen(label);
	int pos = (step * width) / total;


	int percent = (step * 100) / total;

	//set green text color, only on Windows
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
	printf("%s[", label);

	//fill progress bar with =
	for (int i = 0; i < pos; i++)  printf("%c", '=');

	//fill progress bar with spaces
	printf("% *c", width - pos + 1, ']');
	printf(" %3d%%\r", percent);

	//reset text color, only on Windows
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x08);
}

// Downloadingprocess**************************************************************************************************
bool downloadfile(std::string url, std::string loc, int fsize, bool chksize)
{
	// Use WinHttpOpen to obtain a session handle
	HINTERNET hInternet = WinHttpOpen(L"Askyb Downloader",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);

	if (hInternet == NULL)
	{
		printf("Failed to initialize http session.\n");
		return 0;
	}

	// Specify an HTTP server
	HINTERNET hConnected = WinHttpConnect(hInternet, L"www.eu07.es", INTERNET_DEFAULT_HTTP_PORT, 0);

	if (hConnected == NULL)
	{
		WinHttpCloseHandle(hInternet);
		return 0;
	}

	std::wstring stemp = s2ws(url);
	LPCWSTR result = stemp.c_str();

	// Create an HTTP Request handle
	HINTERNET hRequest = WinHttpOpenRequest(hConnected, L"GET", result,
		NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_REFRESH);

	if (hRequest == NULL)
	{
		
		::WinHttpCloseHandle(hConnected);
		::WinHttpCloseHandle(hInternet);
		return 0;
	}

	// Send a Request
	if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnected);
		WinHttpCloseHandle(hInternet);
		return 0;
	}

	// Receive a Response
	if (WinHttpReceiveResponse(hRequest, NULL) == FALSE)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnected);
		WinHttpCloseHandle(hInternet);
		
		return 0;
	}

	if (hRequest)
	{
		// Create a sample binary file 
		HANDLE hFile = CreateFile(loc.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
		DWORD dwSize = 0;
		bool bRetValue = false;

		char szBuffer[100];

		if (chksize) sprintf_s(szBuffer, "Downloading %s", url.c_str());
		if (chksize) WriteLog(szBuffer);
		//sprintf("Downloading %s........", url.c_str());

		int progress = 0;
		int total = 0;
		do
		{
			// Check for available data.
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
			{
				printf("Data not available\n");
			}

			// Allocate space for the buffer.
			BYTE *pchOutBuffer = new BYTE[dwSize + 1];
			if (!pchOutBuffer)
			{
				printf("Http Request is out of memory.\n");
				dwSize = 0;
			}
			else
			{
				// Read the Data.
				DWORD dwDownloaded = 0;
				ZeroMemory(pchOutBuffer, dwSize + 1);




				if (!WinHttpReadData(hRequest, (LPVOID)pchOutBuffer, dwSize, &dwDownloaded))
				{
					printf("Http read data error.\n");
					WriteLog("Http read data error.\n:");
				}
				else
				{
					//sprintf_s(szBuffer, ".");
					//WriteLog(szBuffer);
					progress += dwSize;
					//std::cerr << int(dwSize) << ", "<< int(progress) << std::endl;
					if (chksize) DoProgress("Download: ", progress, fsize);
					// Write buffer to sample binary file
					DWORD wmWritten;
					WriteFile(hFile, pchOutBuffer, dwSize, &wmWritten, NULL);
				}

				delete[] pchOutBuffer;
			}

			//do some action


		} while (dwSize > 0);


		if (chksize) printf("Done\n");
		LARGE_INTEGER size;
		DWORD dwFileSize;
		//if (!GetFileSizeEx(hFile, &size))
		//{
		//	CloseHandle(hFile);
		//	return -1; // error condition, could call GetLastError to find out more
		//}

		// Housekeeping
		CloseHandle(hFile);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnected);
		WinHttpCloseHandle(hInternet);

		int fs = getfsize(stdstrtochar(loc));

		//dwFileSize = GetFileSize(hFile, NULL);
		char buffer[50];
		if (chksize) sprintf(buffer, "%lu", fs);

		// delete file if sizes is not equal
		if (chksize)
		{
			if (fs != fsize)
			{
				DeleteFile(loc.c_str());
				WriteLogSS("FSIZE:", chartostdstr(buffer) + ", size is not equal (file deleted)");
				Beep(250, 130);
			}
			else
			{
				WriteLogSS("FSIZE:", chartostdstr(buffer) + ", OK");
				MoveFileEx(loc.c_str(), stdstrtochar("updates/" + loc), MOVEFILE_REPLACE_EXISTING);
				Beep(950, 130);
			}
		}
		else // w przeciwnym wypadku znaczy to, ze jest to updates.txt wiec nie sprawdzamy rozmiaru bo moze byc rozny
		{
			if (chksize)  WriteLogSS("FSIZE:", chartostdstr(buffer) + ", OK");
		 MoveFileEx(loc.c_str(), stdstrtochar("updates/" + loc), MOVEFILE_REPLACE_EXISTING);
		 Beep(1950, 130);
		}
		Sleep(800);
	}
	return true;
}

//---------------------------------------------------------------------------
