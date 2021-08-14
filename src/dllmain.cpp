#include <Windows.h>
#include <exception>
#include <string>
#include <vector>
#include <array>
#include <tinyformat.h>
#include "Speaker.h"
#include "LobbyChat.h"
#include "GameChat.h"
#include "Debug.h"

bool useDebugPrint = false;
bool readPlayerName = false;
bool readPlayerChat = false;
bool readSystemMessages = false;
bool speaking = true;
std::vector<Speaker*> speakers;
std::thread loaderThread;

unsigned short crc16(const unsigned char* data_p, unsigned char length)
{
	// From https://stackoverflow.com/a/23726131
	unsigned char x;
	unsigned short crc = 0xFFFF;

	while (length--)
	{
		x = crc >> 8 ^ *data_p++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x << 5)) ^ ((unsigned short)x);
	}
	return crc;
}

void printChat(std::string msg)
{
	if (GameChat::isInGame())
	{
		GameChat::print(msg);
	}
	else if (LobbyChat::isInLobby())
	{
		LobbyChat::print(msg);
	}
}

void speak(std::string name, std::string text)
{
	if (!speaking)
	{
		return;
	}
	if(loaderThread.joinable())
	{
		loaderThread.join();
	}

	Speaker* chosen = nullptr;
	for (Speaker* speaker : speakers)
	{
		if (speaker->name == name)
		{
			chosen = speaker;
			break;
		}
		else if (!chosen && !speaker->isBusy())
		{
			chosen = speaker;
		}
	}

	if (chosen)
	{
		int voiceIndex = 0;
		if (!name.empty())
		{
			// Select a voice based on the name
			const unsigned short hash = crc16((const unsigned char*)(name.c_str()), name.length());
			voiceIndex = hash % Speaker::getNumVoices();
		}

		chosen->name = name;
		chosen->say(text, voiceIndex);
	}
}

void speakLobbyChatMessage(std::string type, std::string name, std::string team, std::string text)
{
	if (type == "GLB" && readPlayerChat)
	{
		speak(name, readPlayerName ? tfm::format("%s says: %s", name, text) : text);
	}
	else if (type == "SYS" && readSystemMessages)
	{
		// Unfortunately this includes /me messages, there is no way of telling them apart
		speak("", text);
	}
}

void speakGameChatMessage(GameChatType type, std::string name, std::string text, int color)
{
	static const std::array<std::string, 2> systemMsgsEndWith = {
		"has turned on the crate finder feature of wkRubberWorm (http://worms2d.info/RubberWorm).",
		"has turned off the crate finder.",
	};

	if (type == GameChatType::Normal && readPlayerChat)
	{
		speak(name, readPlayerName ? tfm::format("%s says: %s", name, text) : text);
	}
	else if (type == GameChatType::Team && readPlayerChat)
	{
		speak(name, readPlayerName ? tfm::format("%s says to team: %s", name, text) : text);
	}
	else if (type == GameChatType::Anonymous && readPlayerChat)
	{
		speak(name, readPlayerName ? tfm::format("Anonymous message: %s", text) : text);
	}
	else if (type == GameChatType::WhisperTo && readPlayerChat)
	{
		// TODO whats my name
		speak("", readPlayerName ? tfm::format("Whisper to %s: %s", name, text) : text);
	}
	else if (type == GameChatType::WhisperFrom && readPlayerChat)
	{
		speak(name, readPlayerName ? tfm::format("Whisper from %s: %s", name, text) : text);
	}
	else if (type == GameChatType::Action
		&& (readSystemMessages || std::none_of(systemMsgsEndWith.begin(), systemMsgsEndWith.end(), [&text](std::string s) { return text.ends_with(s); })))
	{
		speak("", text);
	}
	else if (type == GameChatType::System && readSystemMessages)
	{
		speak("", text);
	}
}

void shutUp(std::string args)
{
	for (Speaker* speaker : speakers)
	{
		speaker->shutUp();
	}
}

void setVolume(std::string args)
{
	try
	{
		const int volume = std::clamp(std::stoi(args), 0, 100);
		for (Speaker* speaker : speakers)
		{
			speaker->setVolume(volume / 100.f);
		}
		printChat(tfm::format("Text to speech volume set to %d%%.", volume));
	}
	catch (std::exception)
	{
		speaking = !speaking;
		if (speaking)
		{
			printChat("Text to speech enabled.");
		}
		else
		{
			for (Speaker* speaker : speakers)
			{
				speaker->shutUp();
			}
			printChat("Text to speech disabled.");
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			try {
				auto start = std::chrono::high_resolution_clock::now();
				decltype(start) finish;

				const char iniPath[] = ".\\wkTTS.ini";
				const int moduleEnabled = GetPrivateProfileIntA("General", "EnableModule", 1, iniPath);
				if (!moduleEnabled) {
					return TRUE;
				}
				useDebugPrint = GetPrivateProfileIntA("General", "UseDebugPrint", 0, iniPath);
				speaking = GetPrivateProfileIntA("TTS", "StartEnabled", 0, iniPath);
				readPlayerName = GetPrivateProfileIntA("TTS", "ReadPlayerName", 0, iniPath);
				readPlayerChat = GetPrivateProfileIntA("TTS", "ReadPlayerChat", 1, iniPath);
				readSystemMessages = GetPrivateProfileIntA("TTS", "ReadSystemMessages", 0, iniPath);

				LobbyChat::install();
				LobbyChat::registerChatCallback(&speakLobbyChatMessage);
				LobbyChat::registerCommandCallback("shutup", &shutUp);
				LobbyChat::registerCommandCallback("tts", &setVolume);

				GameChat::install();
				GameChat::registerChatCallback(&speakGameChatMessage);
				GameChat::registerCommandCallback("shutup", &shutUp);
				GameChat::registerCommandCallback("tts", &setVolume);

				loaderThread = std::thread([=](){
					try {
						const int maxVoices = 50;
						char voicePath[MAX_PATH];
						for (int i = 0; i < maxVoices; i++) {
							GetPrivateProfileStringA("TTS", tfm::format("Voice%d", i).c_str(), "", (LPSTR) &voicePath, sizeof(voicePath), iniPath);
							if (strlen(voicePath)) {
								Speaker::loadVoice(voicePath);
							}
						}

						const float volume = GetPrivateProfileIntA("TTS", "Volume", 100, iniPath) / 100.f;
						const int maxSpeakers = 8;
						for (int i = 0; i < maxSpeakers; i++) {
							speakers.push_back(new Speaker(volume));
						}
					}
					catch (std::exception &e) {
						MessageBoxA(0, e.what(), PROJECT_NAME " " PROJECT_VERSION " (" __DATE__ ")", MB_ICONERROR);
					}
				});
				loaderThread.detach();

				finish = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = finish - start;
				debugf("wkTTS startup took %lf seconds\n", elapsed.count());
			}
			catch (std::exception &e) {
				MessageBoxA(0, e.what(), PROJECT_NAME " " PROJECT_VERSION " (" __DATE__ ")", MB_ICONERROR);
			}
			break;
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		default:
			break;
	}
	return TRUE;
}
