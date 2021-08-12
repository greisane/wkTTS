#include "Hooks.h"

#include <stdexcept>
#include <string>
#include <PatternScanner.h>
#include <MinHook.h>
#include "Debug.h"
#include "tinyformat.h"

DWORD Hooks::scanPattern(const char* name, const char* pattern, const char* mask)
{
	uintptr_t ret = hl::FindPatternMask(pattern, mask);
	debugf("ScanPattern: %s = 0x%X\n", name, ret);
	if (!ret)
	{
		throw std::runtime_error(tfm::format("Failed to find memory pattern: %s", name));
	}
	return ret;
}

void Hooks::hook(std::string name, DWORD pTarget, DWORD* pDetour, DWORD* ppOriginal)
{
	if (!initialized)
	{
		initialized = true;
		if (MH_Initialize() != MH_OK)
		{
			throw std::runtime_error("Failed to initialize MinHook");
		}
	}

	if (MH_CreateHook((LPVOID)pTarget, pDetour, (LPVOID*)ppOriginal) != MH_OK)
	{
		throw std::runtime_error(tfm::format("Failed to create hook: %s", name));
	}

	if (MH_EnableHook((LPVOID)pTarget) != MH_OK)
	{
		throw std::runtime_error(tfm::format("Failed to enable hook: %s", name));
	}
}

DWORD Hooks::scanPatternAndHook(const char* name, const char* pattern, const char* mask, DWORD* pDetour, DWORD* ppOriginal)
{
	DWORD pTarget = scanPattern(name, pattern, mask);
	hook(name, pTarget, pDetour, ppOriginal);
	return pTarget;
}
