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

class Event
{
public:
	Event(EventType type) : type_(type){}
	virtual ~Event() {}
	virtual void Print(ostream &stream)
	{
		stream << "Type: " << EventTypeNames[type_];
	}

	EventType GetType() { return type_; }

	virtual Event* Copy() const = 0;

private:
	EventType type_;
};

class NoteOnEvent : public Event
{
public:
	NoteOnEvent(Scale scale, int octave, int degree, short velocity, int length) 
		: Event(NOTE_ON), scale_(scale), octave_(octave), degree_(degree), velocity_(velocity), 
		length_(length)	{}

	~NoteOnEvent() {}

	short GetLength() { return length_; }
	short GetLengthInMs() { return BeatsToMilliseconds(length_); }
	short GetPitch() { return GetMidiPitch(scale_, octave_, degree_); }
	short GetVelocity() { return velocity_; }
	Scale GetScale() { return scale_; }
	int   GetOctave() { return octave_; }
	int   GetDegree() { return degree_; }

	virtual void Print(ostream &stream)
	{
		stream << "NoteOn: ";
		stream << "Scale " << GetScaleName(scale_) << " Octave " << octave_ << " Degree " << degree_
				<< " Vel " << velocity_ << " Length " << length_;
	}

	virtual Event* Copy() const
	{
		return new NoteOnEvent(*this);
	}

private:
	Scale scale_;
	int octave_;
	int degree_;
	short velocity_;
	unsigned long length_;
};

class NoteOffEvent : public Event
{
public:
	NoteOffEvent(short pitch) : Event(NOTE_OFF), pitch_(pitch) {}
	~NoteOffEvent() {}

	short GetPitch() { return pitch_; }

	virtual void Print(ostream &stream)
	{
		stream << "NoteOff: ";
		stream << "Pitch " << pitch_;
	}

	virtual Event* Copy() const
	{
		return new NoteOffEvent(*this);
	}

private:
	short pitch_;
};

class RestEvent : public Event
{
public:
	RestEvent(unsigned long length) : Event(REST), length_(length) {}
	~RestEvent() {}
	short GetLengthInMs() { return BeatsToMilliseconds(length_); }

	virtual void Print(ostream &stream)
	{
		stream << "Rest: ";
		stream << "Length " << length_;
	}

	virtual Event* Copy() const
	{
		return new RestEvent(*this);
	}

private:
	unsigned long length_;
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
		for (unsigned long i=0; i<events_.size(); i++) {
			delete events_[i];
		}
	}

public:
	void Add(const Event* e)
	{
		events_.push_back(e->Copy());
	}

	size_t GetNumEvents() { 
		return events_.size(); 
	}

	Event* GetEvent(int i) {
		if (i > events_.size()) return NULL;
		return events_[i];
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
			events_[i]->Print(cout);
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
	vector<Event*> events_;
	unsigned long repeatCount_;
};

class Track
{
public:
	Track() {}

	void AddPattern(const Pattern& p, PatternQuantize quantize)
	{
		PatternInfo sp(p, quantize);
		patterns_.push_back(sp);
	}

	void Update(float songTime, float elapsedTime, vector<Event*>& events, vector<float>& offsets)
	{
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
						
						if (e->GetType() == REST)
						{
							RestEvent* restEvent = static_cast<RestEvent*>(e);

							// rest event
							float noteLength = restEvent->GetLengthInMs();
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
						else if (e->GetType() == NOTE_ON) 
						{
							NoteOnEvent* noteOnEvent = static_cast<NoteOnEvent*>(e);

							// note on event
							map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(noteOnEvent->GetPitch());
							if (activeNoteIter != activeNotes_.end()) {
								// if note is already on, turn it off
								ActiveNote& activeNote = activeNoteIter->second;
								NoteOffEvent noteOffEvent(activeNote.pitch);
								events.push_back(noteOffEvent);
								offsets.push_back(timeUsed); // make sure the note off event is before the note on for the same pitch

								// active note at this pitch already exists, so replace 
								// that active note with this one
								activeNote.pitch = noteOnEvent->GetPitch();
								activeNote.timeLeft = noteOnEvent->GetLengthInMs();
							}
							else {
								// create a new entry in the active note list
								ActiveNote active;
								active.pitch = noteOnEvent->GetPitch();
								active.timeLeft = noteOnEvent->GetLengthInMs();
								activeNotes_[noteOnEvent->GetPitch()] = active;
							}
							events.push_back(*e);
							offsets.push_back(timeUsed);
							pat.pos++;
						}
					}
				}
				else {
					timeUsed = elapsedTime;
					timeUsedThisIteration = elapsedTime;
				}

				// update active notes
				map<short, ActiveNote>::iterator it;
				for (it = activeNotes_.begin(); it != activeNotes_.end(); ) {
					ActiveNote* activeNote = &it->second;
					if (timeUsedThisIteration > activeNote->timeLeft) {
						// generate note off event
						NoteOffEvent noteOffEvent(activeNote->pitch);
						events.push_back(noteOffEvent);
						offsets.push_back(activeNote->timeLeft);

						// remove active note
						map<short, ActiveNote>::iterator removeIt = it;
						it++;
						activeNotes_.erase(removeIt);
					}
					else {
						activeNote->timeLeft -= timeUsedThisIteration;
						it++;
					}
				}
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

	class EventList
	{
	public:
		EventList()
		{
			events.reserve(1000);
			for (int i=0; i<events.size(); i++) {
				//events[i] = new 
			}
		}

		~EventList()
		{
			for (int i=0; i<events.size(); i++) {
				delete events[i];
			}
		}

		vector<Event*> events;
		vector<float>  offsets;
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