#ifndef MUSIC_H
#define MUSIC_H

#include <vector>
#include <iostream>
#include <fstream>
#include <map>
using namespace std;

float BPM = 200;
float BEAT_LENGTH = 1 / BPM * 60000;

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
const unsigned short NumEventTypes = NONE;

const char* EventTypeNames[NumEventTypes] =
{
	"NOTE_ON",
	"NOTE_OFF",
	"REST",
};

const int NumScales = NO_SCALE;

struct ScaleInfo
{
	char const * const Name;
	short startMidiPitch;
	short intervals[12];
	short numIntervals;
};

const ScaleInfo scaleInfo[NumScales] = 
{
	{ "cmaj", 0, { 0, 2, 4, 5, 7, 9, 11 }, 7 },
	{ "cmin", 0, { 0, 2, 3, 5, 7, 9, 10 }, 7 },
};

const char* GetScaleName(Scale scale)
{
	return scaleInfo[scale].Name;
}

unsigned short GetMidiPitch(Scale scale, int octave, int degree)
{
	const ScaleInfo* info = &scaleInfo[scale];
	short startMidiPitch = info->startMidiPitch;
	short midiPitch = 12 * octave + startMidiPitch;
	if (degree >= 1 && degree <= info->numIntervals) {
		midiPitch += info->intervals[degree-1];
	}
	return midiPitch;
}

float BeatsToMilliseconds(unsigned long beats)
{
	return (BEAT_LENGTH * beats);
}

struct Event
{
	void Print(ostream &stream)
	{
		stream << "Type: " << EventTypeNames[type] << " ";
		if (type == NOTE_ON) {
			stream << "Pitch " << noteOnEvent.pitch << " Vel " << noteOnEvent.velocity << " Length " << noteOnEvent.length;
		}
		else if (type == NOTE_OFF) {
			stream << "Pitch " << noteOffEvent.pitch;
		}
		else if (type == REST) {
			stream << "Length " << restEvent.length;
		}
	}

	EventType     type;

	union 
	{
		struct
		{
			short         pitch;
			short         velocity;
			unsigned long length;

		} noteOnEvent;

		struct
		{
			short         pitch;

		} noteOffEvent;

		struct
		{
			unsigned long length;

		} restEvent;
	};
	
};

class Pattern
{
public:
	Pattern() : repeatCount_(1) 
	{
		init();
	}
	Pattern(unsigned long repeatCount) : repeatCount_(repeatCount)
	{
		init();
	}

	~Pattern() 
	{
	}

public:
	void Add(const Event& e)
	{
		events_.push_back(e);
	}

	void Clear()
	{
		events_.clear();
	}

	size_t GetNumEvents() { 
		return events_.size(); 
	}

	Event* GetEvent(int i) {
		if (i > events_.size()) return NULL;
		return &events_[i];
	}
	
	void SetRepeatCount(int count) {
		repeatCount_ = count;
	}

	int GetRepeatCount() {
		return repeatCount_;
	}

	void Print()
	{
		cout << "[";
		for (unsigned long i=0; i<events_.size(); i++) {
			if (i != 0) {
				cout << ", ";
			}
			events_[i].Print(cout);
		}
		cout << "]";
		if (repeatCount_ != 1) {
			cout << " RepeatCount: " << repeatCount_;
		}
	}

private:
	void init()
	{
		events_.reserve(100);
	}

private:
	vector<Event> events_;
	unsigned long repeatCount_;
};

class Track
{
public:
	Track() {}

	class EventList;

	void AddPattern(const Pattern& p, PatternQuantize quantize)
	{
		PatternInfo sp(p, quantize);
		patterns_.push_back(sp);
	}

	void Update(float songTime, float elapsedTime, vector<Event>& events, vector<float>& offsets)
	{
		// update active notes
		map<short, ActiveNote>::iterator it;
		for (it = activeNotes_.begin(); it != activeNotes_.end(); ) {
			ActiveNote& activeNote = it->second;
			if (elapsedTime >= activeNote.timeLeft) {
				// if at the end of the buffer, wait until next update to generate note off
				if (elapsedTime == activeNote.timeLeft) {
					activeNote.timeLeft = 0;
					it++;
					continue;
				}
				// generate note off event
				Event off;
				off.type = NOTE_OFF;
				off.noteOffEvent.pitch = activeNote.pitch;
				events.push_back(off);
				offsets.push_back(activeNote.timeLeft);

				// remove active note
				map<short, ActiveNote>::iterator removeIt = it;
				it++;
				activeNotes_.erase(removeIt);
			}
			else {
				it++;
			}
		}

		// iterate the patterns
		size_t numPatterns = patterns_.size();
		for (int i=0; i<numPatterns; i++) {
			PatternInfo& pat = patterns_[i];
			float timeUsed = 0;
			while (timeUsed < elapsedTime) {

				float timeUsedThisIteration = 0;

				if (pat.pattern.GetRepeatCount() > 0)
				{
					if (pat.pos >= pat.pattern.GetNumEvents()) {
						pat.pattern.SetRepeatCount(pat.pattern.GetRepeatCount() - 1);
						pat.pos = 0;
						continue;
					}

					// if there is left over time from a previously encountered rest, then consume it.
					if (pat.leftover > 0) 
					{
						if (timeUsed + pat.leftover > elapsedTime) {
							unsigned long timeLeftInFrame = elapsedTime - timeUsed;
							pat.leftover -= timeLeftInFrame;
							timeUsed = elapsedTime;
							timeUsedThisIteration = timeLeftInFrame;
						}
						else {
							timeUsed += pat.leftover;
							timeUsedThisIteration = pat.leftover;
							pat.leftover = 0;
							pat.pos++;
						}
					}
					else
					{
						Event* e = pat.pattern.GetEvent(pat.pos);
						
						if (e->type == REST)
						{
							// rest event
							float noteLength = BeatsToMilliseconds(e->restEvent.length);
							if (timeUsed + noteLength > elapsedTime) {
								unsigned long timeLeftInFrame = elapsedTime - timeUsed;
								pat.leftover = noteLength - timeLeftInFrame;
								timeUsed = elapsedTime;
								timeUsedThisIteration = timeLeftInFrame;
							}
							else {
								timeUsed += noteLength;
								timeUsedThisIteration = noteLength;
								pat.pos++;
							}
						}
						else if (e->type == NOTE_ON) 
						{
							// note on event
							map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(e->noteOnEvent.pitch);
							if (activeNoteIter != activeNotes_.end()) {
								// if note is already on, turn it off
								ActiveNote& activeNote = activeNoteIter->second;
								Event off;
								off.type = NOTE_OFF;
								off.noteOffEvent.pitch = activeNote.pitch;
								events.push_back(off);
								offsets.push_back(timeUsed); // make sure the note off event is before the note on for the same pitch

								// active note at this pitch already exists, so replace 
								// that active note with this one
								activeNote.pitch = e->noteOnEvent.pitch;
								activeNote.timeLeft = BeatsToMilliseconds(e->noteOnEvent.length) + timeUsed;
							}
							else {
								// create a new entry in the active note list
								ActiveNote active;
								active.pitch = e->noteOnEvent.pitch;
								active.timeLeft = BeatsToMilliseconds(e->noteOnEvent.length) + timeUsed;
								activeNotes_[e->noteOnEvent.pitch] = active;
							}
							events.push_back(*e);
							offsets.push_back(timeUsed);
							pat.pos++;
						}
					}
				}
				else {
					timeUsedThisIteration = elapsedTime - timeUsed;
					timeUsed = elapsedTime;
				}
			}
		}

		// update active notes
		for (it = activeNotes_.begin(); it != activeNotes_.end(); ) {
			ActiveNote& activeNote = it->second;
			if (elapsedTime >= activeNote.timeLeft) {
				// if at the end of the buffer, wait until next update to generate note off
				if (elapsedTime == activeNote.timeLeft) {
					activeNote.timeLeft = 0;
					it++;
					continue;
				}
				// generate note off event
				Event off;
				off.type = NOTE_OFF;
				off.noteOffEvent.pitch = activeNote.pitch;
				events.push_back(off);
				offsets.push_back(activeNote.timeLeft);

				// remove active note
				map<short, ActiveNote>::iterator removeIt = it;
				it++;
				activeNotes_.erase(removeIt);
			}
			else {
				activeNote.timeLeft -= elapsedTime;
				it++;
			}
		}
	}

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