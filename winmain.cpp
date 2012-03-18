// GT_HelloWorldWin32.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Music.h"
#include "Plugin.h"
#include "Audio.h"
#include "portaudio.h"

#include "JSFuncs.h"

using namespace std;

static int portaudioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );
bool StartAudio();
bool StopAudio();


//static const int VST_MAX_EVENTS = 512;

PaStream *stream = NULL;
bool audioStarted = false;
static float** vstOutputBuffer = NULL;

vector<Music::Track::Event> songEvents;
vector<float> songOffsets;

HINSTANCE gHinstance;
int gCmdShow;

int ScriptEditBoxId = 1;
int ExecuteButtonId = 2;
HWND scriptBox;

BOOL WINAPI myProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	if(message == WM_CLOSE) {
		PostQuitMessage(0);	
	}

	//check if text in textbox has been changed by user
	if(message==WM_COMMAND && HIWORD(wParam)==EN_CHANGE && LOWORD(wParam) == ScriptEditBoxId)
	{
		//text in the textbox has been modified
		//do your coding here
	}

	if(message==WM_COMMAND && HIWORD(wParam)== BN_CLICKED && LOWORD(wParam) == ExecuteButtonId)
	{
		cout << "Execute Script!" << endl;

		DWORD startSelect;
		DWORD endSelect;
		SendMessage( scriptBox, (UINT) EM_GETSEL, (WPARAM) &startSelect, (LPARAM) &endSelect);
		cout << "w: " << startSelect << " l: " << endSelect << endl;

		int textLength = GetWindowTextLength(scriptBox);
		LPTSTR buff = new char[textLength + 1];
		int ret = GetWindowText(scriptBox, buff, textLength);
		cout << "Parse: " << buff << endl;
		v8::Handle<v8::String> strToParse = v8::String::New(buff);
		v8::Handle<v8::String> filename = v8::String::New("None");
		if (!ExecuteString(strToParse, filename, false, true)) {
			cout << "Failed to parse script: " << buff << endl;
		}
		delete[] buff;
	}

	return false;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	songEvents.reserve(100);
	songOffsets.reserve(100);

	gHinstance = hInstance;
	gCmdShow = nCmdShow;

	// init v8
	v8::HandleScope handle_scope;

	v8::Persistent<v8::Context> context = CreateV8Context();

	if (context.IsEmpty()) {
		printf("Error creating context\n");
		return 1;
	}
	context->Enter();


	//// Create a window for entering script /////////////////////////////

	HWND myDialog = CreateWindowEx(
		0,WC_DIALOG,"My Window",WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, 700, 700, NULL, NULL, hInstance, NULL
	);

	//create the textbox
	scriptBox = CreateWindowEx(
		WS_EX_CLIENTEDGE, //special border style for textbox
		"EDIT","",WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
		0, 0, 700, 630, myDialog, (HMENU)ScriptEditBoxId, NULL, NULL
	);

	SetWindowLong(myDialog, DWL_DLGPROC, (long)myProc);

	HWND button = CreateWindow("button", "Execute", WS_CHILD, 0, 630, 100, 30, myDialog, (HMENU)ExecuteButtonId, NULL, 0);
	ShowWindow(button, SW_SHOW);

	//////////////////////////////////////////////////////////////////////


	// execute script
	const char* str = "C:\\Documents and Settings\\George\\My Documents\\luma2.1\\input.js";
	v8::Handle<v8::String> file_name = v8::String::New(str);
    v8::Handle<v8::String> source = ReadFile(str);
    if (source.IsEmpty()) {
		printf("Error reading '%s'\n", str);
    }
	else {
		v8::Handle<v8::String> filename = v8::String::New("None");
		if (!ExecuteString(source, filename, false, true)) {
			cerr << "Failed to parse script" << endl;
		}
	}

	// start the audio after everything has been initialized
	StartAudio();

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	StopAudio();

	// clean up context
	context->Exit();
	context.Dispose();
	v8::V8::Dispose();

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

	list<SongTrack*> tracks = GetTracks();
	typedef list<SongTrack*>::iterator TrackIter;
	for (TrackIter i=tracks.begin(); i != tracks.end(); i++)
	{
		SongTrack* songTrack = *i;
		Plugin* plugin = songTrack->plugin;
		Music::Track* track = songTrack->track;
		float volume = songTrack->volume;

		plugin->Process(vstOut, framesPerBuffer);
		out = (float*)outputBuffer;
		for (unsigned long j=0; j<framesPerBuffer; j++) {
			*out++ += vstOutputBuffer[0][j] * volume;
			*out++ += vstOutputBuffer[1][j] * volume;
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