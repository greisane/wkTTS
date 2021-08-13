#ifndef WKTTS_LOBBYCHAT_H
#define WKTTS_LOBBYCHAT_H

#include <string>
#include <vector>

typedef unsigned long DWORD;

class LobbyChat
{
public:
	typedef void(*LobbyChatCallback)(std::string type, std::string name, std::string team, std::string text);
	typedef void(*CommandCallback)(std::string args);
	typedef std::tuple<std::string, CommandCallback> CommandAndCallback;

	static void install();
	static void print(const std::string msg);
	static bool isInLobby();

	static void registerHostCommandCallback(std::string command, CommandCallback callback);
	static void registerClientCommandCallback(std::string command, CommandCallback callback);
	static void registerCommandCallback(std::string command, CommandCallback callback);
	static void registerChatCallback(LobbyChatCallback callback);

private:
	static inline DWORD lobbyHostScreen = 0;
	static int __stdcall hookConstructLobbyHostScreen(int a1, int a2);
	static int __fastcall hookDestructLobbyHostScreen(void* This, int EDX, char a2);
	static int __stdcall hookConstructLobbyHostEndScreen(DWORD a1, unsigned int a2, char a3, int a4);
	static int __stdcall hookDestructLobbyHostEndScreen(int a1);

	static inline DWORD lobbyClientScreen = 0;
	static int __stdcall hookConstructLobbyClientScreen(int a1, int a2);
	static int __fastcall hookDestructLobbyClientScreen(void* This, int EDX, char a2);
	static int __stdcall hookConstructLobbyClientEndScreen(DWORD a1);
	static void __fastcall hookDestructCWnd(int This);

	static int __fastcall hookLobbyClientCommands(void* This, void* EDX, char** commandstrptr, char** argstrptr);
	static int __fastcall hookLobbyHostCommands(void* This, void* EDX, char** commandstrptr, char** argstrptr);
	static int __stdcall hookLobbyDisplayMessage(int a1, char *msg);

	static inline std::vector<CommandAndCallback> hostCommandCallbacks;
	static inline std::vector<CommandAndCallback> clientCommandCallbacks;
	static inline std::vector<LobbyChatCallback> chatCallbacks;
};

#endif //WKTTS_LOBBYCHAT_H
