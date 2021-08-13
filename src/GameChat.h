#ifndef WKTTS_GAMECHAT_H
#define WKTTS_GAMECHAT_H

#include <string>
#include <vector>

typedef unsigned long DWORD;

enum class GameChatType
{
	Normal,
	Team,
	Action,
	Anonymous,
	WhisperTo,
	WhisperFrom,
	System,
};

class GameChat
{
public:
	typedef void(*GameChatCallback)(GameChatType type, std::string name, std::string text, int color);
	typedef void(*CommandCallback)(std::string args);
	typedef std::tuple<std::string, CommandCallback> CommandAndCallback;

	static void install();
	static void print(const std::string msg);
	static bool isInGame();

	static void registerCommandCallback(std::string command, CommandCallback callback);
	static void registerChatCallback(GameChatCallback callback);

private:
	static inline DWORD addrDDGame = 0;
	static int __fastcall hookDestroyGlobalContext(int This, int EDX);
	static DWORD __stdcall hookConstructDDGameWrapper(DWORD DD_Game_a2, DWORD DD_Display_a3, DWORD DS_Sound_a4, DWORD DD_Keyboard_a5, DWORD DD_Mouse_a6, DWORD WAV_CDrom_a7, DWORD WS_GameNet_a8);

	static int __stdcall hookShowChatMessage(DWORD ddgame, int color, char* msg, int unk);
	static void __stdcall hookOnChatInput(int a3);
	static int onChatInput(int a1, char* msg, int a3);

	static inline std::vector<CommandAndCallback> commandCallbacks;
	static inline std::vector<GameChatCallback> chatCallbacks;
};

#endif //WKTTS_GAMECHAT_H
