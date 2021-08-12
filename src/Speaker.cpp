#include "Speaker.h"

#include <stdexcept>
#include <tinyformat.h>
#include "../lang/usenglish/usenglish.h"
#include "../lang/cmulex/cmu_lex.h"
#include "../lang/cmu_grapheme_lang/cmu_grapheme_lang.h"
#include "../lang/cmu_grapheme_lex/cmu_grapheme_lex.h"
#include "flite.h"
#include "native_audio.h"

Speaker::Speaker(float volume)
	: voice(nullptr)
	, utt(nullptr)
	, wave(nullptr)
	, audiodev(nullptr)
	, volume(volume)
{
	thread = std::thread(&Speaker::work, this);
}

Speaker::~Speaker()
{
	std::unique_lock lock(mutex);
	quit = true;
	lock.unlock();
	cv.notify_all();

	// Wait for threads to finish
	if (thread.joinable())
	{
		thread.join();
	}

	// Clean up
	if (utt)
	{
		delete_utterance(utt);
		utt = nullptr;
		wave = nullptr;
	}

	if (audiodev)
	{
		audio_close(audiodev);
		audiodev = nullptr;
	}
}

void Speaker::loadVoice(const char* path)
{
	if (!initialized)
	{
		initialized = true;
		flite_init();
		flite_add_lang("eng", usenglish_init, cmu_lex_init);
		flite_add_lang("usenglish", usenglish_init, cmu_lex_init);
		flite_add_lang("cmu_grapheme_lang", cmu_grapheme_lang_init, cmu_grapheme_lex_init);
	}

	cst_voice* voice = flite_voice_load(path);
	if (!voice)
	{
		throw std::runtime_error(tfm::format("Failed to load voice at: %s", path));
	}
	voices.push_back(voice);
	printf("Loaded voice %s\n", path);
}

int Speaker::getNumVoices()
{
	return voices.size();
}

void Speaker::say(const std::string text, int voiceIndex)
{
	std::unique_lock lock(mutex);
	sayQueue.push(std::make_tuple(text, voiceIndex));
	lock.unlock();
	cv.notify_one();
}

void Speaker::shutUp()
{
	std::unique_lock lock(mutex);
	wavePos = waveLen;
	sayQueue = std::queue<SayItem>();
}

bool Speaker::isBusy() const
{
	std::unique_lock lock(mutex);
	return quit || (utt && wave && audiodev && wavePos < waveLen) || !sayQueue.empty();
}

float Speaker::getVolume() const
{
	std::unique_lock lock(mutex);
	return volume;
}

void Speaker::setVolume(float newVolume)
{
	std::unique_lock lock(mutex);
	volume = std::clamp(newVolume, 0.f, 1.f);
}

std::string Speaker::getVoiceName() const
{
	std::unique_lock lock(mutex);
	if (voice)
	{
		return feat_string(voice->features, "name");
	}
	return std::string();
}

void Speaker::work()
{
	std::unique_lock lock(mutex);

	do
	{
		// Work while there is speech being played, text queued or received a quit signal
		cv.wait(lock, [this] {
			return (wave || sayQueue.size() || quit);
		});

		if (!quit && utt && wave && audiodev)
		{
			const int numShorts = min(waveLen - wavePos, CST_AUDIOBUFFSIZE);
			short* out = cst_alloc(short, numShorts);
			for (int i = 0; i < numShorts; ++i)
			{
				out[i] = wave->samples[wavePos + i] * volume;
			}

			lock.unlock();
			const int numBytesWritten = audio_write(audiodev, out, numShorts * 2);
			cst_free(out);
			lock.lock();

			wavePos += numBytesWritten / 2;
			if (numBytesWritten <= 0)
			{
				printf("Failed to write %d bytes\n", numShorts);
				wavePos = waveLen;
			}
			if (wavePos >= waveLen)
			{
				delete_utterance(utt);
				utt = nullptr;
				wave = nullptr;
			}
		}
		else if (!quit && sayQueue.size())
		{
			const auto [text, voiceIndex] = std::move(sayQueue.front());
			sayQueue.pop();

			if (utt)
			{
				delete_utterance(utt);
				utt = nullptr;
				wave = nullptr;
			}

			if (voiceIndex < 0 || voiceIndex >= voices.size())
			{
				printf("Invalid voice index %d", voiceIndex);
				continue;
			}

			if (voice = voices[voiceIndex])
			{
				if (utt = flite_synth_text(text.c_str(), voice))
				{
					if (wave = utt_wave(utt))
					{
						if (!audiodev || audiodev->sps != wave->sample_rate || audiodev->channels != wave->num_channels || audiodev->fmt != CST_AUDIO_LINEAR16)
						{
							audiodev = audio_open(wave->sample_rate, wave->num_channels, CST_AUDIO_LINEAR16);
						}
						if (audiodev)
						{
							wavePos = 0;
							waveLen = wave->num_samples * wave->num_channels;
						}
					}
				}
			}


			/*utt = flite_synth_text(text.c_str(), voice);
			if (!utt)
			{
				printf("Failed to synthesize utterance");
				continue;
			}

			wave = utt_wave(utt);
			if (!wave)
			{
				printf("Failed to synthesize wave");
				continue;
			}

			if (!audiodev || audiodev->sps != wave->sample_rate || audiodev->channels != wave->num_channels || audiodev->fmt != CST_AUDIO_LINEAR16)
			{
				audiodev = audio_open(wave->sample_rate, wave->num_channels, CST_AUDIO_LINEAR16);
			}
			if (!audiodev)
			{
				printf("Failed to open audio device");
				continue;
			}

			wavePos = 0;
			waveLen = wave->num_samples * wave->num_channels;*/
		}
	} while (!quit);
}
