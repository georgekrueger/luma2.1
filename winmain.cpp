// GT_HelloWorldWin32.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "luainc.h"
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

Track gTrack;
Plugin gPlugin(AUDIO_SAMPLE_RATE, AUDIO_FRAMES_PER_BUFFER);
Plugin gPlugin2(AUDIO_SAMPLE_RATE, AUDIO_FRAMES_PER_BUFFER);
vector<Event> songEvents;
vector<float> songOffsets;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
	
	gPlugin.Load("C:\\Program Files\\VSTPlugins\\Circle.dll");
	gPlugin.Show(hInstance, nCmdShow);
	gPlugin2.Load("C:\\Program Files\\VSTPlugins\\Circle.dll");
	gPlugin2.Show(hInstance, nCmdShow);

	//lua_State* luaState_ = luaL_newstate();
	
	Pattern myPattern(1000);
	WeightedEvent noteOn;
	WeightedEvent rest;
	rest.type = REST;
	rest.length.push_back(0.5);
	rest.length.push_back(0.25);
	rest.length.push_back(1);
	rest.lengthWeight.push_back(5);
	rest.lengthWeight.push_back(5);
	rest.lengthWeight.push_back(1);

	noteOn.type = NOTE_ON;

	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 4, 1));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 4, 5));
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);

	noteOn.velocity.push_back(100);
	noteOn.velocityWeight.push_back(1);

	noteOn.length.push_back(0.5);
	noteOn.lengthWeight.push_back(1);

	myPattern.Add(noteOn);
	myPattern.Add(rest);

	gTrack.AddPattern(myPattern, BAR);

	Pattern myPattern2(1000);
	rest.length.clear();
	rest.lengthWeight.clear();
	rest.length.push_back(1);
	rest.lengthWeight.push_back(1);

	noteOn.pitch.clear();
	noteOn.pitchWeights.clear();
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 6, 3));
	noteOn.pitch.push_back(GetMidiPitch(CMAJ, 6, 5));
	noteOn.pitchWeights.push_back(1);
	noteOn.pitchWeights.push_back(1);

	noteOn.velocity.clear();
	noteOn.velocityWeight.clear();
	noteOn.velocity.push_back(100);
	noteOn.velocityWeight.push_back(1);

	noteOn.length.clear();
	noteOn.lengthWeight.clear();
	noteOn.length.push_back(1.5);
	noteOn.lengthWeight.push_back(1);

	myPattern2.Add(noteOn);
	myPattern2.Add(rest);

	gTrack.AddPattern(myPattern2, BAR);

	// start the audio after everything has been initialized
	StartAudio();

	/*cout << "Test" << endl;

	vector<Event> mySongEvents;
	vector<float> mySongOffsets;
 	for (int i=0; i<1000; i++)
	{
		myTrack.Update(0, 64, mySongEvents, mySongOffsets);
		for (int j=0; j<mySongEvents.size(); j++) {
			cout << "Offset: " << mySongOffsets[j] << " ";
			mySongEvents[j].Print(cout);
			cout << endl;
		}
		mySongEvents.clear();
		mySongOffsets.clear();
	}*/

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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
	songEvents.reserve(100);
	songOffsets.reserve(100);

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

	unsigned short numOutputs = gPlugin.GetNumOutputs();
	
	float** vstOut = (float**)vstOutputBuffer;
	gPlugin.Process(vstOut, framesPerBuffer);


	float *out = (float*)outputBuffer;
	for (unsigned long i=0; i<framesPerBuffer; i++) {
		*out++ = vstOutputBuffer[0][i];
		*out++ = vstOutputBuffer[1][i];
	}
	
	// Process events
	gTrack.Update(0, timeElapsedInMs, songEvents, songOffsets);

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

	for (int j=0; j<songEvents.size(); j++) {
		Event& e = songEvents[j];
		float offset = songOffsets[j];

		if (e.type == NOTE_OFF) {
			gPlugin.PlayNoteOff(offset, e.pitch);
		}
		else if (e.type == NOTE_ON) {
			int noteLengthInSamples = int(e.length / 1000 * AUDIO_SAMPLE_RATE);
			gPlugin.PlayNoteOn(offset, e.pitch, e.velocity, noteLengthInSamples);
		}
	}
	songEvents.clear();
	songOffsets.clear();
	
	// End process events
    return 0;
}