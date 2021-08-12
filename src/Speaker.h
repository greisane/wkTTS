#ifndef WKTTS_SPEAKER_H
#define WKTTS_SPEAKER_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>

struct cst_voice_struct;
struct cst_utterance_struct;
struct cst_wave_struct;
struct cst_audiodev_struct;

class Speaker
{
public:
	Speaker(float volume = 1.f);
	~Speaker();

	static void loadVoice(const char* path);
	static int getNumVoices();
	void say(const std::string text, int voiceIndex);
	void shutUp();
	bool isBusy() const;
	float getVolume() const;
	void setVolume(float newVolume);
	std::string getVoiceName() const;
	std::string name;

	Speaker(const Speaker& rhs) = delete;
	Speaker& operator=(const Speaker& rhs) = delete;
	Speaker(Speaker&& rhs) = delete;
	Speaker& operator=(Speaker&& rhs) = delete;

private:
	typedef std::tuple<std::string, int> SayItem;
	static inline bool initialized = false;
	static inline std::vector<cst_voice_struct*> voices;

	mutable std::mutex mutex;
	std::thread thread;
	std::queue<SayItem> sayQueue;
	std::condition_variable cv;
	cst_voice_struct* voice;
	cst_utterance_struct* utt;
	cst_wave_struct* wave;
	cst_audiodev_struct* audiodev;
	int wavePos;
	int waveLen;
	float volume = 1.f;
	bool quit = false;

	void work();
};

#endif //WKTTS_SPEAKER_H
