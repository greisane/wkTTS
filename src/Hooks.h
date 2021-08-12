#ifndef WKTTS_HOOKS_H
#define WKTTS_HOOKS_H

#include <string>

typedef unsigned long DWORD;

class Hooks
{
public:
	static DWORD scanPattern(const char* name, const char* pattern, const char* mask);
	static void hook(std::string name, DWORD pTarget, DWORD* pDetour, DWORD* ppOriginal);
	static DWORD scanPatternAndHook(const char* name, const char* pattern, const char* mask, DWORD* pDetour, DWORD* ppOriginal);

private:
	static inline bool initialized = false;
};

#endif //WKTTS_HOOKS_H
