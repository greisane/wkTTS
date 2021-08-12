#include "GameChat.h"

#include <regex>
#include "Hooks.h"
#include "Debug.h"

int(__fastcall* origDestroyGlobalContext)(int This, int EDX);
int __fastcall GameChat::hookDestroyGlobalContext(int This, int EDX)
{
	int ret = origDestroyGlobalContext(This, EDX);
	addrDDGame = 0;
	return ret;
}

DWORD origConstructDDGameWrapper;
DWORD __stdcall GameChat::hookConstructDDGameWrapper(DWORD DD_Game_a2, DWORD DD_Display_a3, DWORD DS_Sound_a4, DWORD DD_Keyboard_a5, DWORD DD_Mouse_a6, DWORD WAV_CDrom_a7, DWORD WS_GameNet_a8)
{
	DWORD DD_W2Wrapper, retv;
	_asm mov DD_W2Wrapper, edi

	addrDDGame = DD_Game_a2;
	debugf("ddgame: %X\n", addrDDGame);

	_asm push WS_GameNet_a8
	_asm push WAV_CDrom_a7
	_asm push DD_Mouse_a6
	_asm push DD_Keyboard_a5
	_asm push DS_Sound_a4
	_asm push DD_Display_a3
	_asm push DD_Game_a2
	_asm mov edi, DD_W2Wrapper
	_asm call origConstructDDGameWrapper
	_asm mov retv, eax

	return retv;
}

int(__stdcall* origShowChatMessage)(DWORD ddgame, int color, char* msg, int unk);
int __stdcall GameChat::hookShowChatMessage(DWORD ddgame, int color, char* msg, int unk)
{
	static const std::regex normalMessageRgx(R"(^\[([^\]]*)\] (.*)$)");
	static const std::regex anonymousMessageRgx(R"(^(?:.* )?\[(.*)\]$)");
	static const std::regex whisperToMessageRgx(R"(^\*([^\*]*)\* (.*)$)");
	static const std::regex whisperFromMessageRgx(R"(^<([^<]*)> (.*)$)");

	GameChatType type;
	std::string name, text;
	std::cmatch matches;
	if (std::regex_search(msg, matches, normalMessageRgx))
	{
		type = GameChatType::Normal;
		name = matches[1];
		text = matches[2];
	}
	else if (std::regex_search(msg, matches, anonymousMessageRgx))
	{
		type = GameChatType::Anonymous;
		text = matches[1];
	}
	else if (std::regex_search(msg, matches, whisperToMessageRgx))
	{
		type = GameChatType::WhisperTo;
		name = matches[1];
		text = matches[2];
	}
	else if (std::regex_search(msg, matches, whisperFromMessageRgx))
	{
		type = GameChatType::WhisperFrom;
		name = matches[1];
		text = matches[2];
	}
	else if (color == 11)
	{
		type = GameChatType::Action;
		text = msg;
	}
	else
	{
		type = GameChatType::System;
		text = msg;
	}

	for (GameChatCallback& callback : chatCallbacks)
	{
		callback(type, name, text, color);
	}

	return origShowChatMessage(ddgame, color, msg, unk);
}

int GameChat::onChatInput(int a1, char* msg, int a3)
{
	for (const CommandAndCallback& cb : commandCallbacks)
	{
		if (strcmp(msg, std::get<0>(cb).c_str()) == 0)
		{
			std::get<1>(cb)(nullptr);
			return 1;
		}
	}
	return 0;
}

DWORD origOnChatInput = 0;
int __stdcall callOriginalOnChatInput(int a1, char* msg, int a3)
{
	_asm mov ecx, a1
	_asm mov eax, msg
	_asm push a3
	_asm call origOnChatInput
}

#pragma optimize("", off)
char* onchat_eax;
int onchat_ecx;
void __stdcall GameChat::hookOnChatInput(int a3)
{
	_asm mov onchat_eax, eax
	_asm mov onchat_ecx, ecx
	if (!onChatInput(onchat_ecx, (char*)onchat_eax, a3))
	{
		callOriginalOnChatInput(onchat_ecx, onchat_eax, a3);
	}
}
#pragma optimize("", on)

void(__stdcall* addrShowChatMessage)(DWORD addrResourceObject, int color, char* msg, int unk);
void GameChat::print(const std::string msg)
{
	if (addrDDGame)
	{
		const int color = 11;
		origShowChatMessage(addrDDGame, color, (char*)msg.c_str(), 1);
	}
}

void GameChat::install()
{
	Hooks::scanPatternAndHook("DestroyGlobalContext",
		"\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x56\x8B\xF1\x89\x74\x24\x04\xC7\x06\x00\x00\x00\x00\x8B\xC6\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x83\x78\x5C\x00\x75\x07\x8B\xC6\xE8\x00\x00\x00\x00\x8B\xC6\xE8\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x85\xC0\x74\x09\x50\xE8\x00\x00\x00\x00\x83\xC4\x04",
		"???????xx????xxxx????xxxxxxxxxx????xxxxx?????x????x????xxxxxxxxx????xxx????xx????xxxxxx????xxx",
		(DWORD*)&hookDestroyGlobalContext,
		(DWORD*)&origDestroyGlobalContext);
	Hooks::scanPatternAndHook("ConstructDDGameWrapper",
		"\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x53\x8B\x5C\x24\x1C\x55\x8B\x6C\x24\x1C\x56\x8B\x74\x24\x1C\x33\xC0\x89\x86\x00\x00\x00\x00\x89\x44\x24\x14\x89\x86\x00\x00\x00\x00\x8B\xC7\xC7\x06\x00\x00\x00\x00\x89\x9E\x00\x00\x00\x00\x89\xAE\x00\x00\x00\x00\xE8\x00\x00\x00\x00",
		"????????x????xxxx????xxxxxxxxxxxxxxxxxxx????xxxxxx????xxxx????xx????xx????x????",
		(DWORD*)&hookConstructDDGameWrapper,
		(DWORD*)&origConstructDDGameWrapper);
	addrShowChatMessage = (void(__stdcall*)(DWORD, int, char*, int))Hooks::scanPatternAndHook("ShowChatMessage",
		"\x81\xEC\x00\x00\x00\x00\x53\x55\x8B\xAC\x24\x00\x00\x00\x00\x80\xBD\x00\x00\x00\x00\x00\x8B\x85\x00\x00\x00\x00\x8B\x48\x24\x56\x8B\xB1\x00\x00\x00\x00\x57",
		"??????xxxxx????xx?????xx????xxxxxx????x",
		(DWORD*)&hookShowChatMessage,
		(DWORD*)&origShowChatMessage);
	Hooks::scanPatternAndHook("OnChatInput",
		"\x81\xEC\x00\x00\x00\x00\x55\x56\x57\x8B\xF8\x8A\x07\x84\xC0\x8B\xF1\x0F\x84\x00\x00\x00\x00\x3C\x2F\x0F\x85\x00\x00\x00\x00\x8D\x44\x24\x40",
		"??????xxxxxxxxxxxxx????xxxx????xxxx",
		(DWORD*)&hookOnChatInput,
		(DWORD*)&origOnChatInput);
}

void GameChat::registerCommandCallback(std::string command, CommandCallback callback)
{
	commandCallbacks.push_back(std::make_tuple("/" + command, callback));
}

void GameChat::registerChatCallback(GameChatCallback callback)
{
	chatCallbacks.push_back(callback);
}
