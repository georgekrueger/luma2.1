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
const int NumScales = NO_SCALE;

extern const string ScaleStrings[NumScales];

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
	vector<unsigned long> pitchWeight;
	vector<short> velocity;
	vector<unsigned long> velocityWeight;
	vector<float> length;
	vector<unsigned long> lengthWeight;

	short offPitch;

	short GetPitch() const;
	short GetVelocity() const;
	float GetLength() const;
	
private:
	unsigned long WeightedChoose(vector<unsigned long> weights) const;
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
	void Add(const Pattern& p);
	void Clear();
	size_t GetNumEvents() const;
	const WeightedEvent* GetEvent(unsigned long i) const;
	void SetRepeatCount(int count);

	int GetRepeatCount() const;

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