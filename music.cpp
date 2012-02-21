
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
using namespace std;
#include "music.h"

float BPM = 120;
float BEAT_LENGTH = 1 / BPM * 60000;

const unsigned short NumEventTypes = NONE;

const char* ScaleStrings[NumScales] = 
{
	"CMAJ",
	"CMIN"
};

const char* EventTypeNames[NumEventTypes] =
{
	"NOTE_ON",
	"NOTE_OFF",
	"REST",
};

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

float BeatsToMilliseconds(float beats)
{
	return (BEAT_LENGTH * beats);
}


void WeightedEvent::Print(ostream &stream)
{
	stream << "Type: " << EventTypeNames[type] << " ";
	/*if (type == NOTE_ON) {
		stream << "Pitch " << noteOnEvent.pitch << " Vel " << noteOnEvent.velocity << " Length " << noteOnEvent.length;
	}
	else if (type == NOTE_OFF) {
		stream << "Pitch " << noteOffEvent.pitch;
	}
	else if (type == REST) {
		stream << "Length " << restEvent.length;
	}*/
}

short WeightedEvent::GetPitch() { 
	unsigned long pick = WeightedChoose(pitchWeights);
	return pitch[pick];
}

short WeightedEvent::GetVelocity() { 
	unsigned long pick = WeightedChoose(velocityWeight);
	return velocity[pick];
}

float WeightedEvent::GetLength() { 
	unsigned long pick = WeightedChoose(lengthWeight);
	return length[pick];
}

unsigned long WeightedEvent::WeightedChoose(vector<unsigned long> weights)
{
	unsigned long total = 0;
	for (unsigned long i=0; i<weights.size(); i++) {
		total += weights[i];
	}
	unsigned long num = rand() % total;
	total = 0;
	for (unsigned long i=0; i<weights.size(); i++) {
		total += weights[i];
		if (total > num) {
			return i;
		}
	}
	return 0;
}
	

Pattern::Pattern() : repeatCount_(1) 
{
	init();
}
Pattern::Pattern(unsigned long repeatCount) : repeatCount_(repeatCount)
{
	init();
}

Pattern::~Pattern() 
{
}

void Pattern::Add(const WeightedEvent& e)
{
	events_.push_back(e);
}

void Pattern::Clear()
{
	events_.clear();
}

size_t Pattern::GetNumEvents() { 
	return events_.size(); 
}

WeightedEvent* Pattern::GetEvent(unsigned long i) {
	if (i > events_.size()) return NULL;
	return &events_[i];
}
	
void Pattern::SetRepeatCount(int count) {
	repeatCount_ = count;
}

int Pattern::GetRepeatCount() {
	return repeatCount_;
}

void Pattern::Print()
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

void Pattern::init()
{
	events_.reserve(100);
}

Track::Track() {}

void Track::AddPattern(const Pattern& p, PatternQuantize quantize)
{
	PatternInfo sp(p, quantize);
	patterns_.push_back(sp);
}

void Track::Update(float songTime, float elapsedTime, vector<Event>& events, vector<float>& offsets)
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
			off.pitch = activeNote.pitch;
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
	for (size_t i=0; i<numPatterns; i++) {
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
						float timeLeftInFrame = elapsedTime - timeUsed;
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
					WeightedEvent* e = pat.pattern.GetEvent(pat.pos);
						
					if (e->type == REST)
					{
						// rest event
						float length = e->GetLength();
						float noteLength = BeatsToMilliseconds(length);
						if (timeUsed + noteLength > elapsedTime) {
							float timeLeftInFrame = elapsedTime - timeUsed;
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
						short pitch = e->GetPitch();
						short velocity = e->GetVelocity();
						float length = e->GetLength();
						map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(pitch);
						if (activeNoteIter != activeNotes_.end()) {
							// if note is already on, turn it off
							ActiveNote& activeNote = activeNoteIter->second;
							Event off;
							off.type = NOTE_OFF;
							off.pitch = pitch;
							events.push_back(off);
							offsets.push_back(timeUsed); // make sure the note off event is before the note on for the same pitch

							// active note at this pitch already exists, so replace 
							// that active note with this one
							activeNote.pitch = pitch;
							activeNote.timeLeft = BeatsToMilliseconds(length) + timeUsed;
						}
						else {
							// create a new entry in the active note list
							ActiveNote active;
							active.pitch = pitch;
							active.timeLeft = BeatsToMilliseconds(length) + timeUsed;
							activeNotes_[pitch] = active;
						}
						Event noteOn;
						noteOn.type = NOTE_ON;
						noteOn.pitch = pitch;
						noteOn.velocity = velocity;
						noteOn.length = length;
						events.push_back(noteOn);
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
			off.pitch = activeNote.pitch;
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


