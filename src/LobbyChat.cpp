#include "LobbyChat.h"

#include "Hooks.h"
#include "Debug.h"

int(__stdcall* origConstructLobbyHostScreen)(int a1, int a2);
int __stdcall LobbyChat::hookConstructLobbyHostScreen(int a1, int a2)
{
	auto ret = origConstructLobbyHostScreen(a1, a2);
	lobbyHostScreen = a1;
	return ret;
}

int(__stdcall* origConstructLobbyHostEndScreen)(DWORD a1, unsigned int a2, char a3, int a4);
int __stdcall LobbyChat::hookConstructLobbyHostEndScreen(DWORD a1, unsigned int a2, char a3, int a4)
{
	auto ret = origConstructLobbyHostEndScreen(a1, a2, a3, a4);
	lobbyHostScreen = a1;
	return ret;
}

int(__fastcall* origDestructLobbyHostScreen)(void* This, int EDX, char a2);
int __fastcall LobbyChat::hookDestructLobbyHostScreen(void* This, int EDX, char a2)
{
	lobbyHostScreen = 0;
	return origDestructLobbyHostScreen(This, EDX, a2);
}

int(__stdcall* origDestructLobbyHostEndScreen)(int a1);
int __stdcall LobbyChat::hookDestructLobbyHostEndScreen(int a1)
{
	lobbyHostScreen = 0;
	return origDestructLobbyHostEndScreen(a1);
}

int(__stdcall* origConstructLobbyClientScreen)(int a1, int a2);
int __stdcall LobbyChat::hookConstructLobbyClientScreen(int a1, int a2)
{
	auto ret = origConstructLobbyClientScreen(a1, a2);
	lobbyClientScreen = a1;
	return ret;
}

int(__fastcall* origDestructLobbyClientScreen)(void* This, int EDX, char a2);
int __fastcall LobbyChat::hookDestructLobbyClientScreen(void* This, int EDX, char a2)
{
	lobbyClientScreen = 0;
	return origDestructLobbyClientScreen(This, EDX, a2);
}

int(__stdcall* origConstructLobbyClientEndScreen)(DWORD a1);
int __stdcall LobbyChat::hookConstructLobbyClientEndScreen(DWORD a1)
{
	auto ret = origConstructLobbyClientEndScreen(a1);
	lobbyClientScreen = a1;
	return ret;
}

void(__fastcall* origDestructCWnd)(int This);
void __fastcall LobbyChat::hookDestructCWnd(int This)
{
	if (lobbyClientScreen == This)
	{
		lobbyClientScreen = 0;
	}
	origDestructCWnd(This);
}

int(__stdcall* origLobbyDisplayMessage)(int a1, char* msg);
int __stdcall LobbyChat::hookLobbyDisplayMessage(int a1, char* msg)
{
	std::string msgs = std::string(msg);
	size_t sep0 = msgs.find(':', 0);
	size_t sep1 = msgs.find(':', sep0 + 1);
	size_t sep2 = msgs.find(':', sep1 + 1);
	std::string type = msgs.substr(0, sep0);
	std::string name = msgs.substr(sep0 + 1, sep1 - sep0 - 1);
	std::string team = msgs.substr(sep1 + 1, sep2 - sep1 - 1);
	std::string text = msgs.substr(sep2 + 1);
	for (LobbyChatCallback& callback : chatCallbacks)
	{
		callback(type, name, team, text);
	}

	return origLobbyDisplayMessage(a1, msg);
}

int(__fastcall* origLobbyClientCommands)(void* This, void* EDX, char** commandstrptr, char** argstrptr);
int __fastcall LobbyChat::hookLobbyClientCommands(void* This, void* EDX, char** commandstrptr, char** argstrptr)
{
	for (const CommandAndCallback& cb : clientCommandCallbacks)
	{
		if (strcmp(*commandstrptr, std::get<0>(cb).c_str()) == 0)
		{
			std::get<1>(cb)(*argstrptr);
			return 1;
		}
	}
	return origLobbyClientCommands(This, EDX, commandstrptr, argstrptr);
}

int(__fastcall* origLobbyHostCommands)(void* This, void* EDX, char** commandstrptr, char** argstrptr);
int __fastcall LobbyChat::hookLobbyHostCommands(void* This, void* EDX, char** commandstrptr, char** argstrptr)
{
	for (const CommandAndCallback& cb : hostCommandCallbacks)
	{
		if (strcmp(*commandstrptr, std::get<0>(cb).c_str()) == 0)
		{
			std::get<1>(cb)(*argstrptr);
			return 1;
		}
	}
	return origLobbyHostCommands(This, EDX, commandstrptr, argstrptr);
}

void LobbyChat::print(const std::string msg)
{
	std::string buff = "SYS:TTS:ALL:";
	buff += msg;

	if (lobbyClientScreen)
	{
		origLobbyDisplayMessage((int)lobbyClientScreen + 0x10318, const_cast<char*>(buff.c_str()));
	}
	else if (lobbyHostScreen)
	{
		origLobbyDisplayMessage((int)lobbyHostScreen + 0x10318, const_cast<char*>(buff.c_str()));
	}
}

void LobbyChat::install()
{
	DWORD addrConstructLobbyHostScreen = Hooks::scanPatternAndHook("ConstructLobbyHostScreen",
		"\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x5C\x53\x8B\x5C\x24\x74\x55\x8B\x6C\x24\x74\x57\x8D\xBD\x00\x00\x00\x00\x57\x53\x68\x00\x00\x00\x00",
		"???????xx????xxxx????xxxxxxxxxxxxxxxx????xxx????",
		(DWORD*)&hookConstructLobbyHostScreen,
		(DWORD*)&origConstructLobbyHostScreen);
	DWORD* addrLobbyHostScreenVtable = *(DWORD**)(addrConstructLobbyHostScreen + 0x41);
	DWORD addrDestructLobbyHostScreen = addrLobbyHostScreenVtable[1];
	Hooks::hook("DestructLobbyHostScreen",
		addrDestructLobbyHostScreen,
		(DWORD*)&hookDestructLobbyHostScreen,
		(DWORD*)&origDestructLobbyHostScreen);
	Hooks::scanPatternAndHook("ConstructLobbyHostEndScreen",
		"\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x8B\x44\x24\x14\x64\x89\x25\x00\x00\x00\x00\x53\x8B\x5C\x24\x1C\x56\x57\x8B\x7C\x24\x1C\x53\x50\x57\xE8\x00\x00\x00\x00\x6A\x01\x8D\x8F\x00\x00\x00\x00\xC7\x44\x24\x00\x00\x00\x00\x00\x51",
		"??????xxx????xxxxxxxx????xxxxxxxxxxxxxxx????xxxx????xxx?????x",
		(DWORD*)&hookConstructLobbyHostEndScreen,
		(DWORD*)&origConstructLobbyHostEndScreen);
	Hooks::scanPatternAndHook("DestructLobbyHostEndScreen",
		"\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x56\x8B\x74\x24\x14\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\xC7\x44\x24\x00\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x85\xC0\x74\x09\x50\xE8\x00\x00\x00\x00\x83\xC4\x04",
		"??????xxx????xxxx????xxxxxxx????xx?????xxx?????xx????xxxxxx????xxx",
		(DWORD*)&hookDestructLobbyHostEndScreen,
		(DWORD*)&origDestructLobbyHostEndScreen);

	Hooks::scanPatternAndHook("ConstructLobbyClientScreen",
		"\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x5C\x53\x8B\x5C\x24\x74\x55\x8B\x6C\x24\x74\x56\x57\x53\x55\xB9\x00\x00\x00\x00",
		"???????xx????xxxx????xxxxxxxxxxxxxxxxxx????",
		(DWORD*)&hookConstructLobbyClientScreen,
		(DWORD*)&origConstructLobbyClientScreen);
	Hooks::scanPatternAndHook("ConstructLobbyClientEndScreen",
		"\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x53\x56\x57\x8B\x7C\x24\x1C\x6A\x01\x57\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8D\x87\x00\x00\x00\x00\x50\x33\xDB\x68\x00\x00\x00\x00\x53",
		"??????xxx????xxxx????xxxxxxxxxxx????x????xx????xxxx????x",
		(DWORD*)&hookConstructLobbyClientEndScreen,
		(DWORD*)&origConstructLobbyClientEndScreen);

	Hooks::scanPatternAndHook("LobbyHostCommands",
		"\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x56\x8B\x75\x08\x8B\x06\x57\x68\x00\x00\x00\x00\x50\x8B\xF9\xE8\x00\x00\x00\x00\x83\xC4\x08\x85\xC0\x0F\x84\x00\x00\x00\x00\x8B\x06\x68\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00",
		"??????xx????xxx????xxxx????xx????xxxxxxxxx????xxxx????xxxxxxx????xxx????xx????",
		(DWORD*)&hookLobbyHostCommands,
		(DWORD*)&origLobbyHostCommands);
	Hooks::scanPatternAndHook("LobbyClientCommands",
		"\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x40\x53\x56\x8B\x75\x08\x8B\x06\x57\x8B\xD9\x68\x00\x00\x00\x00\x50\x89\x5C\x24\x1C\xE8\x00\x00\x00\x00\x83\xC4\x08\x85\xC0",
		"??????xxx????xx????xxxx????xxxxxxxxxxxxxx????xxxxxx????xxxxx",
		(DWORD*)&hookLobbyClientCommands,
		(DWORD*)&origLobbyClientCommands);
	Hooks::scanPatternAndHook("LobbyDisplayMessage",
		"\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x30\x53\x56\x57\xE8\x00\x00\x00\x00\x33\xC9\x85\xC0\x0F\x95\xC1\x85\xC9\x75\x0A\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00",
		"??????xx????xxx????xxxx????xxxxxxx????xxxxxxxxxxxx????x????",
		(DWORD*)&hookLobbyDisplayMessage,
		(DWORD*)&origLobbyDisplayMessage);
}

void LobbyChat::registerHostCommandCallback(std::string command, CommandCallback callback)
{
	hostCommandCallbacks.push_back(std::make_tuple(command, callback));
}

void LobbyChat::registerClientCommandCallback(std::string command, CommandCallback callback)
{
	clientCommandCallbacks.push_back(std::make_tuple(command, callback));
}

void LobbyChat::registerCommandCallback(std::string command, CommandCallback callback)
{
	hostCommandCallbacks.push_back(std::make_tuple(command, callback));
	clientCommandCallbacks.push_back(std::make_tuple(command, callback));
}

void LobbyChat::registerChatCallback(LobbyChatCallback callback)
{
	chatCallbacks.push_back(callback);
}
