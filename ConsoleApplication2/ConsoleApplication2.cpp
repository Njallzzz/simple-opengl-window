#include <windows.h>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <psapi.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

void *
FindBasePtr(HANDLE hProcess) {
	DWORD cbNeeded, mNeeded = MAX_PATH;
	BOOL ret;
	wchar_t module[MAX_PATH];
	HMODULE hMods[1024];

	ret = QueryFullProcessImageNameW(hProcess, 0, module, &mNeeded);
	if (ret == 0)
		return nullptr;

	ret = EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded);
	if (ret == 0)
		return nullptr;

	for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
		TCHAR szModName[MAX_PATH];
		ret = GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR));
		if (ret) {
			if (lstrcmpi(module, szModName) == 0) {
				MODULEINFO mod;
				BOOL res = GetModuleInformation(hProcess, hMods[i], &mod, sizeof(mod));
				return (void *)mod.lpBaseOfDll;
			}
		}
	}

	return nullptr;
}

template<typename T>
inline int32_t
GetProcessValueByLocation(HANDLE hProcess, void *location, T *result) {
	size_t read;

	BOOL res = ReadProcessMemory(hProcess, location, result, sizeof(T), &read);
	if (res == 0)
		return -1;
	if (read != sizeof(T))
		return -1;

	return 0;
}

void *
FindDMAAddy(HANDLE hProcess, void *base, std::vector<uint64_t> offsets) {
	char *addr = (char *)base;

	for (size_t i = 0; i < offsets.size(); i++) {
		void *buffer;
		addr = addr + offsets[i];

		int res = GetProcessValueByLocation<void *>(hProcess, addr, &buffer);
		if (res)
			return nullptr;
		if (buffer == nullptr)
			return nullptr;

		addr = (char *)buffer;
	}

	return (void *)addr;
}


int main(int argc, char *argv[]) {
	int width, height;
	size_t buffer_size = 1280 * 720 * 3;
	char *buffer = new char[buffer_size];
	void *base_ptr = nullptr;
	char *data;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	wchar_t command[] = L"ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\zeevee.flv -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\asd.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f rawvideo udp://127.0.0.1:1234";

	BOOL res = CreateProcess(NULL,	// No module name (use command line)
		command,			// Command line
		NULL,				// Process handle not inheritable
		NULL,				// Thread handle not inheritable
		FALSE,				// Set handle inheritance to FALSE
		CREATE_NO_WINDOW,	// Creation flags, CREATE_NO_WINDOW | CREATE_SUSPENDED
		NULL,			// Use parent's environment block
		NULL,			// Use parent's starting directory 
		&si,			// Pointer to STARTUPINFO structure
		&pi);			// Pointer to PROCESS_INFORMATION structure

	if (!res) {
		cout << "CreateProcess failed, error:" << GetLastError() << endl;
		system("pause");
		return res;
	}

	cout << "Spawned process, pid: " << pi.dwProcessId << endl;
	Sleep(3000);

	base_ptr = FindBasePtr(pi.hProcess);
	if (base_ptr == nullptr) {
		cout << "Unable to find base pointer of ffmpeg" << endl;
		goto close;
	}
	else
		cout << "FFmpeg Base pointer: " << (void *)base_ptr << endl;

	data = (char *)FindDMAAddy(pi.hProcess, base_ptr, { 0x48B3030, 0x20, 0x8, 0x18, 0x8 });
	if (data == nullptr) {
		cout << "Unable to find picture frame in ffmpeg" << endl;
		goto close;
	}
	else
		cout << "FFmpeg frame pointer: " << (void *)data << endl;

	res = GetProcessValueByLocation<int>(pi.hProcess, nullptr, &width);
	if (res) {
		cout << "Unable to get width of frame" << endl;
		goto close;
	}

	res = GetProcessValueByLocation<int>(pi.hProcess, nullptr, &height);
	if (res) {
		cout << "Unable to get height of frame" << endl;
		goto close;
	}

	data += 128;	// Move to start of picture data

	{
		size_t read;
		res = ReadProcessMemory(pi.hProcess, data, buffer, buffer_size, &read);
		if (read != buffer_size)
			res = 0;
		else
			stbi_write_bmp("test_image.bmp", 1280, 720, 3, buffer);
	}
	if (res == 0)
		goto close;

	/*while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
		system("pause");
		DWORD res_val = SuspendThread(pi.hThread);
		cout << "Suspended thread" << endl;

		system("pause");
		res_val = ResumeThread(pi.hThread);
		cout << "Resuming thread" << endl;
	}*/

close:
	delete[] buffer;

	system("pause");
	TerminateProcess(pi.hProcess, 0);

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	cout << "Process ended, closing..." << endl;
	system("pause");
	return 0;
}

