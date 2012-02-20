#ifndef MUSIC_H
#define MUSIC_H

#include <vector>
#include <iostream>
#include <fstream>
#include <map>
using namespace std;

///////////////////////////
// Scales
///////////////////////////
enum Scale
{
	CMAJ,
	CMIN,
	NO_SCALE,
};

enum PatternQuantize
{
	BEAT,
	BAR
};

enum EventType
{
	NOTE_ON,
	NOTE_OFF,
	REST,
	NONE
};

unsigned short GetMidiPitch(Scale scale, int octave, int degree);
const char* GetScaleName(Scale scale);
float BeatsToMilliseconds(float beats);

class WeightedEvent
{
public:
	void Print(ostream &stream);

	EventType     type;
	vector<short> pitch;
	vector<unsigned long> pitchWeights;
	vector<short> velocity;
	vector<unsigned long> velocityWeight;
	vector<float> length;
	vector<unsigned long> lengthWeight;

	short offPitch;

	short GetPitch();
	short GetVelocity();
	float GetLength();
	
private:
	unsigned long WeightedChoose(vector<unsigned long> weights);
};

struct Event
{
	EventType type;
	short     pitch;
	short     velocity;
	float     length;
};

class Pattern
{
public:
	Pattern();
	Pattern(unsigned long repeatCount);
	~Pattern();

public:
	void Add(const WeightedEvent& e);
	void Clear();
	size_t GetNumEvents();
	WeightedEvent* GetEvent(unsigned long i);
	void SetRepeatCount(int count);

	int GetRepeatCount();

	void Print();

private:
	void init();

private:
	vector<WeightedEvent> events_;
	unsigned long repeatCount_;
};

class Track
{
public:
	Track();

	class EventList;

	void AddPattern(const Pattern& p, PatternQuantize quantize);

	void Update(float songTime, float elapsedTime, vector<Event>& events, vector<float>& offsets);

private:

	class PatternInfo
	{
	public:
		PatternInfo(const Pattern& pPattern, PatternQuantize pQuantize) 
			: started(false), pos(0), leftover(0), pattern(pPattern), quantize(pQuantize) {}

		bool started;
		unsigned int pos;
		float leftover;
		PatternQuantize quantize;
		Pattern pattern;
	};

	struct ActiveNote
	{
		int pitch;
		float timeLeft;
	};
	vector<PatternInfo> patterns_;
	map<short, ActiveNote> activeNotes_;
};

#endif