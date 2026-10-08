#include "Memory.hpp"
#include <tlhelp32.h>

uint32_t pProcess::FindProcessIdByProcessName(const char* ProcessName)
{
	std::wstring wideProcessName;
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, ProcessName, -1, nullptr, 0);
	if (wideCharLength > 0)
	{
		wideProcessName.resize(wideCharLength);
		MultiByteToWideChar(CP_UTF8, 0, ProcessName, -1, &wideProcessName[0], wideCharLength);
	}

	HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32W process_entry_{ };
	process_entry_.dwSize = sizeof(PROCESSENTRY32W);

	DWORD pid = 0;
	if (Process32FirstW(hPID, &process_entry_))
	{
		do
		{
			if (!wcscmp(process_entry_.szExeFile, wideProcessName.c_str()))
			{
				pid = process_entry_.th32ProcessID;
				break;
			}
		} while (Process32NextW(hPID, &process_entry_));
	}
	CloseHandle(hPID);
	return pid;
}

uint32_t pProcess::FindProcessIdByWindowName(const char* WindowName)
{
	DWORD process_id = 0;
	HWND windowHandle = FindWindowA(nullptr, WindowName);
	if (windowHandle)
		GetWindowThreadProcessId(windowHandle, &process_id);
	return process_id;
}

HWND pProcess::GetWindowHandleFromProcessId(DWORD ProcessId) {
	HWND hwnd = NULL;
	do {
		hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == ProcessId) {
			TCHAR windowTitle[MAX_PATH];
			GetWindowText(hwnd, windowTitle, MAX_PATH);
			if (IsWindowVisible(hwnd) && windowTitle[0] != '\0') {
				return hwnd;
			}
		}
	} while (hwnd != NULL);
	return NULL; // No main window found for the given process ID
}

  bool pProcess::AttachProcess(const char* ProcessName)
{
	this->pid_ = this->FindProcessIdByProcessName(ProcessName);

	if (pid_)
	{
		HMODULE modules[0xFF];
		MODULEINFO module_info;
		DWORD _;

		handle_ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                PROCESS_VM_READ, FALSE, pid_);

		if (!handle_)
			return false;

		EnumProcessModulesEx(this->handle_, modules, sizeof(modules), &_, LIST_MODULES_64BIT);
		base_client_.base = (uintptr_t)modules[0];

		GetModuleInformation(this->handle_, modules[0], &module_info, sizeof(module_info));
		base_client_.size = module_info.SizeOfImage;

		hwnd_ = this->GetWindowHandleFromProcessId(pid_);

		return true;
	}

	return false;
}

bool pProcess::AttachWindow(const char* WindowName)
{
	this->pid_ = this->FindProcessIdByWindowName(WindowName);

	if (pid_)
	{
		HMODULE modules[0xFF];
		MODULEINFO module_info;
		DWORD _;

		handle_ = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid_);

		EnumProcessModulesEx(this->handle_, modules, sizeof(modules), &_, LIST_MODULES_64BIT);
		base_client_.base = (uintptr_t)modules[0];

		GetModuleInformation(this->handle_, modules[0], &module_info, sizeof(module_info));
		base_client_.size = module_info.SizeOfImage;

		hwnd_ = this->GetWindowHandleFromProcessId(pid_);

		return true;
	}
	return false;
}

bool pProcess::UpdateHWND()
{
	hwnd_ = this->GetWindowHandleFromProcessId(pid_);
	return hwnd_ != nullptr;
}

ProcessModule pProcess::GetModule(const char* lModule)
{
	std::wstring wideModule;
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, lModule, -1, nullptr, 0);
	if (wideCharLength > 0)
	{
		wideModule.resize(wideCharLength);
		MultiByteToWideChar(CP_UTF8, 0, lModule, -1, &wideModule[0], wideCharLength);
		wideModule.resize(wcslen(wideModule.c_str())); // drop the trailing null
	}

	this->last_module_error_ = 0;

	// 1) Primary: walk the loader list through PSAPI using our existing handle (no snapshot needed)
	if (this->handle_)
	{
		std::vector<HMODULE> modules(1024);
		DWORD needed = 0;

		for (int tries = 0; tries < 3; tries++)
		{
			if (!EnumProcessModulesEx(this->handle_, modules.data(), static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &needed, LIST_MODULES_ALL))
			{
				this->last_module_error_ = GetLastError(); // ERROR_PARTIAL_COPY while the game is still loading, etc.
				break;
			}

			if (needed <= modules.size() * sizeof(HMODULE))
			{
				modules.resize(needed / sizeof(HMODULE));

				for (HMODULE mod : modules)
				{
					wchar_t name[MAX_PATH]{};
					if (!GetModuleBaseNameW(this->handle_, mod, name, MAX_PATH))
						continue;

					if (_wcsicmp(name, wideModule.c_str()) != 0)
						continue;

					MODULEINFO info{};
					GetModuleInformation(this->handle_, mod, &info, sizeof(info));
					return { reinterpret_cast<uintptr_t>(mod), static_cast<uintptr_t>(info.SizeOfImage) };
				}
				break; // enumerated everything, module just isn't loaded (yet)
			}

			modules.resize(needed / sizeof(HMODULE) + 64); // list grew, retry with a bigger buffer
		}
	}

	// 2) Fallback: Toolhelp snapshot (properly started with Module32FirstW)
	HANDLE handle_module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_);
	if (handle_module == INVALID_HANDLE_VALUE)
	{
		if (!this->last_module_error_)
			this->last_module_error_ = GetLastError();
		return { 0, 0 };
	}

	MODULEENTRY32W module_entry_{};
	module_entry_.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(handle_module, &module_entry_))
	{
		do
		{
			if (!_wcsicmp(module_entry_.szModule, wideModule.c_str()))
			{
				CloseHandle(handle_module);
				return { (uintptr_t)module_entry_.modBaseAddr, (uintptr_t)module_entry_.modBaseSize };
			}
		} while (Module32NextW(handle_module, &module_entry_));
	}

	CloseHandle(handle_module);
	return { 0, 0 };
}

LPVOID pProcess::Allocate(size_t size_in_bytes)
{
	return VirtualAllocEx(this->handle_, NULL, size_in_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

uintptr_t pProcess::FindSignature(std::vector<uint8_t> signature)
{
	std::unique_ptr<uint8_t[]> data;
	data = std::make_unique<uint8_t[]>(this->base_client_.size);

	if (!ReadProcessMemory(this->handle_, (void*)(this->base_client_.base), data.get(), this->base_client_.size, NULL)) {
		return 0x0;
	}

	for (uintptr_t i = 0; i < this->base_client_.size; i++)
	{
		for (uintptr_t j = 0; j < signature.size(); j++)
		{
			if (signature.at(j) == 0x00)
				continue;

			if (*reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(&data[i + j])) == signature.at(j))
			{
				if (j == signature.size() - 1)
					return this->base_client_.base + i;
				continue;
			}
			break;
		}
	}
	return 0x0;
}

uintptr_t pProcess::FindSignature(ProcessModule target_module, std::vector<uint8_t> signature)
{
	std::unique_ptr<uint8_t[]> data;
	data = std::make_unique<uint8_t[]>(0xFFFFFFF);

	if (!ReadProcessMemory(this->handle_, (void*)(target_module.base), data.get(), 0xFFFFFFF, NULL)) {
		return NULL;
	}

	for (uintptr_t i = 0; i < 0xFFFFFFF; i++)
	{
		for (uintptr_t j = 0; j < signature.size(); j++)
		{
			if (signature.at(j) == 0x00)
				continue;

			if (*reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(&data[i + j])) == signature.at(j))
			{
				if (j == signature.size() - 1)
					return this->base_client_.base + i;
				continue;
			}
			break;
		}
	}
	return 0x0;
}

uintptr_t pProcess::FindCodeCave(uint32_t length_in_bytes)
{
	std::vector<uint8_t> cave_pattern = {};

	for (uint32_t i = 0; i < length_in_bytes; i++) {
		cave_pattern.push_back(0x00);
	}

	return FindSignature(cave_pattern);
}

void pProcess::Close()
{
	CloseHandle(handle_);
}
