// GT_HelloWorldWin32.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "luainc.h"
#include "luafuncs.h"
#include "music.h"
#include "Plugin.h"
#include "portaudio.h"

using namespace std;

static int portaudioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );
bool StartAudio();
bool StopAudio();

static const unsigned long AUDIO_SAMPLE_RATE = 44100;
static const int AUDIO_OUTPUT_CHANNELS = 2;
static const unsigned long AUDIO_FRAMES_PER_BUFFER = 512;
//static const int VST_MAX_EVENTS = 512;

PaStream *stream = NULL;
bool audioStarted = false;
static float** vstOutputBuffer = NULL;

vector<Track*> tracks;
vector<Plugin*> plugins;

vector<Event> songEvents;
vector<float> songOffsets;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
	songEvents.reserve(100);
	songOffsets.reserve(100);

	Plugin* plug = new Plugin(AUDIO_SAMPLE_RATE, AUDIO_FRAMES_PER_BUFFER);
	plug->Load("C:\\Program Files\\VSTPlugins\\Abakos_2\\HERCs Abakos\\Abakos.dll", "Sun Cult [KEYS]");

	plug->Show(hInstance, nCmdShow);
	plugins.push_back(plug);

	plug = new Plugin(AUDIO_SAMPLE_RATE, AUDIO_FRAMES_PER_BUFFER);
	plug->Load("C:\\Program Files\\VSTPlugins\\4FrontPiano\\4Front Piano.dll");
	plug->Show(hInstance, nCmdShow);
	plugins.push_back(plug);

	tracks.push_back(new Track);
	tracks.push_back(new Track);
	
	Pattern myPattern(1000);
	WeightedEvent noteOn;
	WeightedEvent rest;
	rest.type = REST;
	rest.length.push_back(1);
	rest.length.push_back(2);
	rest.length.push_back(4);
	rest.lengthWeight.push_back(1);
	rest.lengthWeight.push_back(2);
	rest.lengthWeight.push_back(3);

	noteOn.type = NOTE_ON;

	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 3, 1));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 3, 4));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 3, 5));
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);

	noteOn.velocity.push_back(50);
	noteOn.velocity.push_back(100);
	noteOn.velocityWeight.push_back(1);
	noteOn.velocityWeight.push_back(1);

	noteOn.length.push_back(4);
	noteOn.lengthWeight.push_back(1);

	myPattern.Add(noteOn);
	myPattern.Add(rest);

	tracks[0]->AddPattern(myPattern, BAR);

	Pattern myPattern2(1000);
	rest.length.clear();
	rest.lengthWeight.clear();
	rest.length.push_back(0.5);
	rest.lengthWeight.push_back(1);

	noteOn.pitch.clear();
	noteOn.pitchWeights.clear();
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 6, 1));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 6, 4));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 6, 5));
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);

	noteOn.velocity.clear();
	noteOn.velocityWeight.clear();
	noteOn.velocity.push_back(40);
	noteOn.velocity.push_back(60);
	noteOn.velocityWeight.push_back(1);
	noteOn.velocityWeight.push_back(1);

	noteOn.length.clear();
	noteOn.lengthWeight.clear();
	noteOn.length.push_back(1);
	noteOn.lengthWeight.push_back(1);

	myPattern2.Add(noteOn);
	myPattern2.Add(rest);

	tracks[1]->AddPattern(myPattern2, BAR);

	// start the audio after everything has been initialized
	StartAudio();

	// init lua
	lua_State* luaState = luaL_newstate();
	luaL_openlibs(luaState);
	lua_register(luaState, "luma_rand", luma_rand);

	cout << "running script" << endl;
	char* scriptName = "C:\\Documents and Settings\\George\\My Documents\\luma2.1\\input.lua";
	if (luaL_dofile(luaState, scriptName) != 0) {
		cout << "ERROR: " << lua_tostring(luaState, -1) << endl;
	}

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	StopAudio();

	for (int i=0; i<plugins.size(); i++) {
		delete plugins[i];
		delete tracks[i];
	}

    return (int) msg.wParam;
}

void HandleAudioError(PaError err)
{
	// print error info here
	cout << "Audio failed to start. Error code: " << err << endl;
}

bool StartAudio()
{
	PaStreamParameters outputParameters;
    PaError err;
    
	// init buffer used in callback to retrieve data from plugin
	vstOutputBuffer = new float*[VST_MAX_OUTPUT_CHANNELS_SUPPORTED];
	for (int i=0; i<VST_MAX_OUTPUT_CHANNELS_SUPPORTED; i++) {
		vstOutputBuffer[i] = new float[AUDIO_FRAMES_PER_BUFFER];
	}

	err = Pa_Initialize();
	if( err != paNoError ) {
		HandleAudioError(err); 
		return false;
	}
    
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      HandleAudioError(err); return false;
    }

    outputParameters.channelCount = AUDIO_OUTPUT_CHANNELS;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              AUDIO_SAMPLE_RATE,
              AUDIO_FRAMES_PER_BUFFER,
              (paClipOff | paDitherOff),
              portaudioCallback,
              NULL );
    if( err != paNoError ) {
		HandleAudioError(err);
		return false;
	}

    err = Pa_StartStream( stream );
    if( err != paNoError ) {
		HandleAudioError(err); 
		return false;
	}

	audioStarted = true;

    return true;
}

bool StopAudio()
{
	PaError err = Pa_CloseStream( stream );
    if( err != paNoError ) {
		HandleAudioError(err); 
		return false;
	}
    Pa_Terminate();

	audioStarted = false;

	return true;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int portaudioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    (void) inputBuffer;

	float timeElapsedInMs = framesPerBuffer / (float)AUDIO_SAMPLE_RATE * 1000;

	float** vstOut = (float**)vstOutputBuffer;

	float *out = (float*)outputBuffer;
	for (unsigned long i=0; i<framesPerBuffer; i++) {
		*out++ = 0;
		*out++ = 0;
	}

	for (int i=0; i<plugins.size(); i++)
	{
		Plugin* plugin = plugins[i];
		Track* track = tracks[i];

		plugin->Process(vstOut, framesPerBuffer);
		out = (float*)outputBuffer;
		for (unsigned long j=0; j<framesPerBuffer; j++) {
			*out++ += vstOutputBuffer[0][j];
			*out++ += vstOutputBuffer[1][j];
		}
	
		songEvents.clear();
		songOffsets.clear();

		// Process events
		track->Update(0, timeElapsedInMs, songEvents, songOffsets);

		// convert offsets to samples
		for (int j=0; j<songEvents.size(); j++) {
			// first convert offset to samples
			int offsetInSamples = songOffsets[j] / 1000 * AUDIO_SAMPLE_RATE;
			songOffsets[j] = offsetInSamples;
		}

		// check for note-off / note-on pairs at the same pitch and time.
		// some vsts require that the note-on be atleast one sample after the note-off.
		for (int j=0; j<songEvents.size(); j++) {
			if (songEvents[j].type == NOTE_OFF) {
				if (j + 1 < songEvents.size() && songEvents[j+1].type == NOTE_ON && songOffsets[j] == songOffsets[j+1]) 
				{
					if (songEvents[j].pitch == songEvents[j+1].pitch) {
						if (songOffsets[j] > 0) {
							// move the note-off back one sample
							songOffsets[j] -= 1;
						}
						else {
							// move the note-on forward one sample
							songOffsets[j+1] += 1;
						}
					}
				}
			}
		}

		// send events to plugin
		for (int j=0; j<songEvents.size(); j++) {
			Event& e = songEvents[j];
			float offset = songOffsets[j];

			if (e.type == NOTE_OFF) {
				plugin->PlayNoteOff(offset, e.pitch);
			}
			else if (e.type == NOTE_ON) {
				int noteLengthInSamples = int(e.length / 1000 * AUDIO_SAMPLE_RATE);
				plugin->PlayNoteOn(offset, e.pitch, e.velocity, noteLengthInSamples);
			}
		}
	}
	
	// End process events
    return 0;
}