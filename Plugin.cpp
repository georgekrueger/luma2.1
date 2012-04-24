//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:28 $
//
// Category     : VST 2.x SDK Samples
// Filename     : minihost.cpp
// Created by   : Steinberg
// Description  : VST Mini Host
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include "Plugin.h"
#include "pluginterfaces/vst2.x/aeffectx.h"

#if _WIN32
#include <tchar.h>
#elif TARGET_API_MAC_CARBON
#include <CoreFoundation/CoreFoundation.h>
#endif

#if _WIN32
// The main window class name.
static TCHAR szWindowClass[] = _T("pluginWindow");
// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Plugin Window");
#endif

static std::map<HWND, Plugin*> windowPluginMap;
static WNDCLASSEX windowClass;
static bool registeredWindowClass = false;

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);
static VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
static void checkEffectProperties (AEffect* effect);
static void checkEffectProcessing (AEffect* effect);
static bool checkPlatform ();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//-------------------------------------------------------------------------------------------------------
// PluginLoader
//-------------------------------------------------------------------------------------------------------
struct PluginLoader
{
//-------------------------------------------------------------------------------------------------------
	void* module;

	PluginLoader ()
	: module (0)
	{}

	~PluginLoader ()
	{
		if (module)
		{
		#if _WIN32
			FreeLibrary ((HMODULE)module);
		#elif TARGET_API_MAC_CARBON
			CFBundleUnloadExecutable ((CFBundleRef)module);
			CFRelease ((CFBundleRef)module);
		#endif
		}
	}

	bool loadLibrary (const char* fileName)
	{
	#if _WIN32
		module = LoadLibrary (fileName);
	#elif TARGET_API_MAC_CARBON
		CFStringRef fileNameString = CFStringCreateWithCString (NULL, fileName, kCFStringEncodingUTF8);
		if (fileNameString == 0)
			return false;
		CFURLRef url = CFURLCreateWithFileSystemPath (NULL, fileNameString, kCFURLPOSIXPathStyle, false);
		CFRelease (fileNameString);
		if (url == 0)
			return false;
		module = CFBundleCreate (NULL, url);
		CFRelease (url);
		if (module && CFBundleLoadExecutable ((CFBundleRef)module) == false)
			return false;
	#endif
		return module != 0;
	}

	PluginEntryProc getMainEntry ()
	{
		PluginEntryProc mainProc = 0;
	#if _WIN32
		mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "VSTPluginMain");
		if (!mainProc)
			mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "main");
	#elif TARGET_API_MAC_CARBON
		mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName ((CFBundleRef)module, CFSTR("VSTPluginMain"));
		if (!mainProc)
			mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName ((CFBundleRef)module, CFSTR("main_macho"));
	#endif
		return mainProc;
	}
//-------------------------------------------------------------------------------------------------------
};

Plugin::Plugin(unsigned long sampleRate, unsigned long blockSize) : mBlockSize(blockSize), mSampleRate(sampleRate)
{
	pluginLoader = new PluginLoader();

	int nHdrLen = sizeof(VstEvents) + (100 * sizeof(VstMidiEvent *));
	BYTE * effEvData = new BYTE[nHdrLen];
	vstEvents = (VstEvents *) effEvData;
	vstEvents->numEvents = 0;

	globalEvent = new VstMidiEvent;
}

Plugin::~Plugin()
{
	Hide();
	Unload();

	delete pluginLoader;
	delete vstEvents;
	delete globalEvent;
}

bool Plugin::SetProgram(string program)
{
	// Iterate programs...
	for (VstInt32 i = 0; i < effect->numPrograms; i++) {
		string programName;
		if (GetProgramName(i, programName)) {
			if (programName == program) {
				effect->dispatcher (effect, effSetProgram, 0, i, 0, 0.0f);
				return true;
			}
		}
	}
	return false;
}

bool Plugin::GetProgramName (int programNumber, string& programName)
{
    char strProgramName [256];

    if (!effect->dispatcher (effect, effGetProgramNameIndexed, programNumber, 0, strProgramName, 0.0f))
    {
       // the hard way - have to set each program to find it's name
       int tempPreset = effect->dispatcher (effect, effGetProgram, 0, 0, 0, 0.0f);
       effect->dispatcher (effect, effSetProgram, 0, programNumber, 0, 0.0f);
       effect->dispatcher (effect, effGetProgramName, 0, 0, strProgramName, 0.0f);
       effect->dispatcher (effect, effSetProgram, 0, tempPreset, 0, 0.0f);
    }

	if (strlen(strProgramName) > 0) {
		programName = strProgramName;
		return true;
	}

    return false;
}

bool Plugin::Load(string fileName, std::string program)
{
	if (!checkPlatform ()) {
		cout << "Platform verification failed! Please check your Compiler Settings!" << endl;
		return false;
	}

	// load library
	if (!pluginLoader->loadLibrary(fileName.c_str())) {
		printf ("Failed to load VST Plugin library!\n");
		return false;
	}

	// get main entry point of plugin
	PluginEntryProc mainEntry = pluginLoader->getMainEntry();
	if (!mainEntry)	{
		printf ("VST Plugin main entry not found!\n");
		return false;
	}

	// call main entry point of plugin
	effect = mainEntry (HostCallback);
	if (!effect) {
		printf ("Failed to create effect instance!\n");
		return false;
	}

	if (effect->numOutputs > VST_MAX_OUTPUT_CHANNELS_SUPPORTED) {
		printf("Plugin has more outputs than are supported by this host. Max outputs support is: %d\n", effect->numOutputs);
		return false;
	}
	
	// initialize plugin
	effect->dispatcher (effect, effOpen, 0, 0, 0, 0);
	effect->dispatcher (effect, effSetSampleRate, 0, 0, 0, (float)mSampleRate);
	effect->dispatcher (effect, effSetBlockSize, 0, mBlockSize, 0, 0);
	effect->dispatcher (effect, effMainsChanged, 0, 1, 0, 0);

	// sanity check
	checkEffectProperties (effect);

	if (program.length() > 0) {
		SetProgram(program);
	}

	bool usesChunks = (effect->flags & effFlagsProgramChunks);
	cout << "usesChunks: " << usesChunks << endl;

	bool supportsProcessReplacing = (effect->flags & effFlagsCanReplacing);
	cout << "supportsProcessReplacing: " << supportsProcessReplacing << endl;

	//void* chunkData;
	//static const VstInt32 PRESET_CHUNK =  1;
	//static const VstInt32 BANK_CHUNK =  0;
	//VstInt32 chunkSize = effect->dispatcher (effect, effGetChunk, BANK_CHUNK, 0, &chunkData, 0);
	//cout << chunkSize << endl;
	//VstIntPtr ret = effect->dispatcher (effect, effSetChunk, BANK_CHUNK, chunkSize, &chunkData, 0);
	//cout << ret << endl;

	mLoaded = true;

	return true;
}

bool Plugin::Unload()
{
	if (!mLoaded) {
		return true;
	}

	// close effect
	effect->dispatcher (effect, effMainsChanged, 0, 0, 0, 0);
	effect->dispatcher (effect, effClose, 0, 0, 0, 0);

	mLoaded = false;

	return true;
}

bool Plugin::Show(HINSTANCE win, int nCmdShow)
{
	if (!mLoaded) {
		cout << "Plugin is not loaded! It can't be shown!" << endl;
		return false;
	}

	mWindowInstance = win;

	// register window class
	if (!registeredWindowClass)
	{
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style          = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc    = WndProc;
		windowClass.cbClsExtra     = 0;
		windowClass.cbWndExtra     = 0;
		windowClass.hInstance      = win;
		windowClass.hIcon          = LoadIcon(win, MAKEINTRESOURCE(IDI_APPLICATION));
		windowClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		windowClass.lpszMenuName   = NULL;
		windowClass.lpszClassName  = szWindowClass;
		windowClass.hIconSm        = LoadIcon(windowClass.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

		if (!RegisterClassEx(&windowClass))
		{
			MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Plugin::show"), NULL);
			return false;
		}
		registeredWindowClass = true;
	}

    // create window
	// The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    mWindow = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
        NULL,
        NULL,
        mWindowInstance,
        this
    );

    if (!mWindow){
        MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Win32 Guided Tour"), NULL);
        return false;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(mWindow, nCmdShow);
    UpdateWindow(mWindow);

	mShown = true;

	return true;
}

bool Plugin::Hide()
{
	if (!mShown) {
		return true;
	}

	mShown = false;

	return true;
}

void Plugin::PlayNoteOn(float deltaFrames, short pitch, short velocity, short length)
{
	globalEvent->type = kVstMidiType;
	globalEvent->byteSize = sizeof(VstMidiEvent);
	globalEvent->deltaFrames = deltaFrames;	///< sample frames related to the current block start sample position
	globalEvent->flags = 0;			///< @see VstMidiEventFlags
	globalEvent->noteLength = length;	///< (in sample frames) of entire note, if available, else 0
	globalEvent->noteOffset = 0;	///< offset (in sample frames) into note from note start if available, else 0
	globalEvent->midiData[0] = (char)0x90;
	globalEvent->midiData[1] = (char)pitch;
	globalEvent->midiData[2] = (char)velocity;
	globalEvent->detune = 0;			///< -64 to +63 cents; for scales other than 'well-tempered' ('microtuning')
	globalEvent->noteOffVelocity = 0;	///< Note Off Velocity [0, 127]
	
	vstEvents->numEvents = 1;
	vstEvents->events[0] = (VstEvent*)globalEvent;
	
	effect->dispatcher( effect, effProcessEvents, 0, 0, vstEvents, 0);
}

void Plugin::PlayNoteOff(float deltaFrames, short pitch)
{
	globalEvent->type = kVstMidiType;
	globalEvent->byteSize = sizeof(VstMidiEvent);
	globalEvent->deltaFrames = deltaFrames;	///< sample frames related to the current block start sample position
	globalEvent->flags = 0;			///< @see VstMidiEventFlags
	globalEvent->noteLength = 0;	///< (in sample frames) of entire note, if available, else 0
	globalEvent->noteOffset = 0;	///< offset (in sample frames) into note from note start if available, else 0
	globalEvent->midiData[0] = (char)0x80;
	globalEvent->midiData[1] = (char)pitch;
	globalEvent->midiData[2] = (char)0;
	globalEvent->detune = 0;			///< -64 to +63 cents; for scales other than 'well-tempered' ('microtuning')
	globalEvent->noteOffVelocity = 0;	///< Note Off Velocity [0, 127]
	
	vstEvents->numEvents = 1;
	vstEvents->events[0] = (VstEvent*)globalEvent;
	
	effect->dispatcher( effect, effProcessEvents, 0, 0, vstEvents, 0);
}

void Plugin::ProgramChange(float deltaFrames, char programNumber)
{
	globalEvent->type = kVstMidiType;
	globalEvent->byteSize = sizeof(VstMidiEvent);
	globalEvent->deltaFrames = deltaFrames;	///< sample frames related to the current block start sample position
	globalEvent->flags = 0;			///< @see VstMidiEventFlags
	globalEvent->noteLength = 0;	///< (in sample frames) of entire note, if available, else 0
	globalEvent->noteOffset = 0;	///< offset (in sample frames) into note from note start if available, else 0
	globalEvent->midiData[0] = (char)0xC0;
	globalEvent->midiData[1] = programNumber;
	globalEvent->midiData[2] = 0;
	globalEvent->detune = 0;			///< -64 to +63 cents; for scales other than 'well-tempered' ('microtuning')
	globalEvent->noteOffVelocity = 0;	///< Note Off Velocity [0, 127]
	
	vstEvents->numEvents = 1;
	vstEvents->events[0] = (VstEvent*)globalEvent;
	
	effect->dispatcher( effect, effProcessEvents, 0, 0, vstEvents, 0);
}

void Plugin::Process(float** buffer, unsigned long numFrames)
{
	effect->processReplacing(effect, NULL, buffer, numFrames);
}

unsigned short Plugin::GetNumOutputs()
{
	return effect->numOutputs;
}

bool Plugin::SetPreset(std::string)
{
	effect->dispatcher( effect, effSetProgram, 0, 0, 0, 0);
	return true;
}

//-------------------------------------------------------------------------------------------------------
static bool checkPlatform ()
{
#if VST_64BIT_PLATFORM
	printf ("*** This is a 64 Bit Build! ***\n");
#else
	printf ("*** This is a 32 Bit Build! ***\n");
#endif

	int sizeOfVstIntPtr = sizeof (VstIntPtr);
	int sizeOfVstInt32 = sizeof (VstInt32);
	int sizeOfPointer = sizeof (void*);
	int sizeOfAEffect = sizeof (AEffect);
	
	printf ("VstIntPtr = %d Bytes, VstInt32 = %d Bytes, Pointer = %d Bytes, AEffect = %d Bytes\n\n",
			sizeOfVstIntPtr, sizeOfVstInt32, sizeOfPointer, sizeOfAEffect);

	return sizeOfVstIntPtr == sizeOfPointer;
}

//-------------------------------------------------------------------------------------------------------
void checkEffectProperties (AEffect* effect)
{
	printf ("HOST> Gathering properties...\n");

	char effectName[256] = {0};
	char vendorString[256] = {0};
	char productString[256] = {0};

	effect->dispatcher (effect, effGetEffectName, 0, 0, effectName, 0);
	effect->dispatcher (effect, effGetVendorString, 0, 0, vendorString, 0);
	effect->dispatcher (effect, effGetProductString, 0, 0, productString, 0);

	printf ("Name = %s\nVendor = %s\nProduct = %s\n\n", effectName, vendorString, productString);

	printf ("numPrograms = %d\nnumParams = %d\nnumInputs = %d\nnumOutputs = %d\n\n", 
			effect->numPrograms, effect->numParams, effect->numInputs, effect->numOutputs);

	printf ("\n");

	// Iterate parameters...
	for (VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++)
	{
		char paramName[256] = {0};
		char paramLabel[256] = {0};
		char paramDisplay[256] = {0};

		effect->dispatcher (effect, effGetParamName, paramIndex, 0, paramName, 0);
		effect->dispatcher (effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
		effect->dispatcher (effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
		float value = effect->getParameter (effect, paramIndex);

		printf ("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName, paramDisplay, paramLabel, value);
	}

	printf ("\n");

	// Can-do nonsense...
	static const char* canDos[] =
	{
		"receiveVstEvents",
		"receiveVstMidiEvent",
		"midiProgramNames"
	};

	for (VstInt32 canDoIndex = 0; canDoIndex < sizeof (canDos) / sizeof (canDos[0]); canDoIndex++)
	{
		printf ("Can do %s... ", canDos[canDoIndex]);
		VstInt32 result = (VstInt32)effect->dispatcher (effect, effCanDo, 0, 0, (void*)canDos[canDoIndex], 0);
		switch (result)
		{
			case 0  : printf ("don't know"); break;
			case 1  : printf ("yes"); break;
			case -1 : printf ("definitely not!"); break;
			default : printf ("?????");
		}
		printf ("\n");
	}

	printf ("\n");
}

//-------------------------------------------------------------------------------------------------------
VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	VstIntPtr result = 0;

	// Filter idle calls...
	bool filtered = false;
	if (opcode == audioMasterIdle)
	{
		static bool wasIdle = false;
		if (wasIdle)
			filtered = true;
		else
		{
			printf ("(Future idle calls will not be displayed!)\n");
			wasIdle = true;
		}
	}

	//if (!filtered)
	//	printf ("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void> (value), ptr, opt);

	switch (opcode)
	{
		case audioMasterVersion :
			result = kVstVersion;
			break;
	}

	return result;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	//-----------------------
	case WM_CREATE :
	{
		SetWindowText (hWnd, "VST Editor");

		CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
		Plugin* plugin = static_cast<Plugin*>(createStruct->lpCreateParams);
		AEffect* effect = plugin->GetVstEffect();

		if (effect)
		{
			windowPluginMap[hWnd] = plugin;

			printf ("HOST> Open editor...\n");
			effect->dispatcher (effect, effEditOpen, 0, 0, hWnd, 0);

			printf ("HOST> Get editor rect..\n");
			ERect* eRect = 0;
			effect->dispatcher (effect, effEditGetRect, 0, 0, &eRect, 0);
			if (eRect)
			{
				int width = eRect->right - eRect->left;
				int height = eRect->bottom - eRect->top;
				if (width < 100)
					width = 100;
				if (height < 100)
					height = 100;

				RECT wRect;
				SetRect (&wRect, 0, 0, width, height);
				AdjustWindowRectEx (&wRect, GetWindowLong (hWnd, GWL_STYLE), FALSE, GetWindowLong (hWnd, GWL_EXSTYLE));
				width = wRect.right - wRect.left;
				height = wRect.bottom - wRect.top;

				SetWindowPos (hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
			}
		}
	}	break;

	//-----------------------
	/*case WM_TIMER :
		if (effect)
			effect->dispatcher (effect, effEditIdle, 0, 0, 0, 0);
		break;*/

	//-----------------------
	case WM_CLOSE :
	{
		Plugin* plugin = windowPluginMap[hWnd];
		AEffect* effect = plugin->GetVstEffect();

		printf ("HOST> Close editor..\n");
		if (effect)
			effect->dispatcher (effect, effEditClose, 0, 0, 0, 0);

		DestroyWindow(hWnd);
	}	break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}
