#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <map>
#include <windows.h>

static const unsigned int VST_MAX_OUTPUT_CHANNELS_SUPPORTED = 2;

struct AEffect;
struct VstEvents;
struct VstMidiEvent;
struct PluginLoader;

class Plugin
{
public:
	Plugin(unsigned long sampleRate, unsigned long blockSize);
	~Plugin();

	bool Load(std::string name);
	bool Unload();
	bool Show(HINSTANCE win, int nCmdShow);
	bool Hide();

	void PlayNoteOn(float deltaFrames, short pitch, short velocity, short length);
	void PlayNoteOff(float deltaFrames, short pitch);

	void Process(float** buffer, unsigned long numFrames);

	unsigned short GetNumOutputs();

	AEffect* GetVstEffect() { return effect; }

private:
	unsigned long mSampleRate;
	unsigned long mBlockSize;
	bool mLoaded;
	bool mShown;
	AEffect* effect;
	VstEvents* vstEvents;
	VstMidiEvent* globalEvent;
	PluginLoader* pluginLoader;
	HINSTANCE mWindowInstance;
	HWND mWindow;
};

#endif