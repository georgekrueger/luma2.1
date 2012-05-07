#include "Music.h"

using namespace std;

namespace Music
{

float BPM = 120;
float BEAT_LENGTH = 1 / BPM * 60000;

static bool randSeeded = false;

const string ScaleStrings[NumScales] = 
{
	"MAJ",
	"MIN",
	"PENTAMIN"
};

const string NamedPitches[12] = 
{
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

struct ScaleInfo
{
	short intervals[12];
	short numIntervals;
};

const ScaleInfo scaleInfo[NumScales] = 
{
	{ { 0, 2, 4, 5, 7, 9, 11 }, 7 },
	{ { 0, 2, 3, 5, 7, 9, 10 }, 7 },
	{ { 0, 3, 5, 7, 10 }, 5 },
};

const char* GetScaleName(Scale scale)
{
	return ScaleStrings[scale].c_str();
}

unsigned short GetPitchNumberFromName(string PitchName)
{
	for (int i=0; i<12; i++) {
		if (PitchName == NamedPitches[i]) {
			return i;
		}
	}
	return 0;
}

unsigned short GetMidiPitch(Scale scale, int root, int octave, int degree)
{
	const ScaleInfo* info = &scaleInfo[scale];
	short midiPitch = 12 * octave + root;
	if (degree >= 1 && degree <= info->numIntervals) {
		midiPitch += info->intervals[degree-1];
	}
	return midiPitch;
}

float BeatsToMilliseconds(float beats)
{
	return (BEAT_LENGTH * beats);
}

void ParsePitchString(const std::string& str, Scale& scale, short& root, short& octave, short& degree)
{
	scale = NO_SCALE;
	root = 60;
	octave = 1;
	degree = 1;

	size_t firstSplit = str.find('_', 0);
	if (firstSplit == string::npos)
		return;
	size_t secondSplit = str.find('_', firstSplit + 1);
	if (secondSplit == string::npos)
		return;
	size_t thirdSplit = str.find('_', secondSplit + 1);
	if (thirdSplit == string::npos)
		return;
	string rootStr = str.substr(0, firstSplit);
	string scaleStr = str.substr(firstSplit+1, secondSplit-firstSplit-1);
	string octaveStr = str.substr(secondSplit+1, 1);
	string degreeStr = str.substr(thirdSplit+1, 1);
	//cout << scaleStr << " " << octaveStr << " " << degreeStr << endl;

	root = GetPitchNumberFromName(rootStr);

	for (int i=0; i<NumScales; i++) {
		if (scaleStr.compare(ScaleStrings[i]) == 0) {
			scale = (Scale)i;
			break;
		}
	}
	if (scale == NO_SCALE) {
		cout << "Invalid scale: " << scaleStr << endl;
		scale = MAJ;
	}
	stringstream octaveStream(octaveStr);
	octaveStream >> octave;
	stringstream degreeStream(degreeStr);
	degreeStream >> degree;
}

ValueListSharedPtr NoteGenerator::Generate()
{
	ValueListSharedPtr pitchResult = pitchGen_->Generate();
	ValueListSharedPtr velocityResult = velocityGen_->Generate();
	ValueListSharedPtr lengthResult = lengthGen_->Generate();

	short pitch;
	if (std::string* pitchStr = boost::get<std::string>(pitchResult->at(0).get())) {
		// parse pitch as string
		Scale scale;
		short root;
		short octave;
		short degree;
		ParsePitchString(*pitchStr, scale, root, octave, degree);
		pitch = GetMidiPitch(scale, root, octave, degree);
	}
	else if (int* pitchInt = boost::get<int>(pitchResult->at(0).get())) {
		// parse pitch as int
		pitch = *pitchInt;
	}

	int* velocityPtr = boost::get<int>(velocityResult->at(0).get());
	int velocity = 127;
	if (velocityPtr != NULL) {
		velocity = *velocityPtr;
	}

	float* lengthPtr = boost::get<float>(lengthResult->at(0).get());
	float length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	// Create and return a note value
	NoteSharedPtr note = NoteSharedPtr(new Note);
	note->pitch = pitch;
	note->velocity = velocity;
	note->length = length;

	ValueListSharedPtr resultPtr;
	resultPtr->push_back(ValueSharedPtr(new Value(note)));
	return resultPtr;
}


ValueListSharedPtr RestGenerator::Generate()
{
	ValueListSharedPtr lengthResult = lengthGen_->Generate();

	float* lengthPtr = boost::get<float>(lengthResult->at(0).get());
	float length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	// Create and return a rest value
	RestSharedPtr rest = RestSharedPtr(new Rest);
	rest->length = length;

	ValueListSharedPtr resultPtr;
	resultPtr->push_back(ValueSharedPtr(new Value(rest)));
	return resultPtr;
}

ValueListSharedPtr PatternGenerator::Generate()
{
	ValueListSharedPtr outResult;
	for (unsigned long j=0; j<repeat_; j++)
	{
		for (unsigned long i=0; i<items_.size(); i++)
		{
			ValueListSharedPtr res = items_[i]->Generate();
			outResult->insert(outResult->end(), res->begin(), res->end());
		}
	}
	return outResult;
}

ValueListSharedPtr WeightedGenerator::Generate()
{
	if (!randSeeded) {
		srand ( static_cast<unsigned int>(time(NULL)) );
		randSeeded = true;
	}

	unsigned long total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
	}
	// TODO: make uniform distribution
	int r = rand();
	unsigned long num = r % total;
	total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
		if (total > num) {
			// found the item we are choosing
			return values_[i].first->Generate();
		}
	}

	return ValueListSharedPtr();
}

Track::Track() 
{
}

void Track::Add(GeneratorSharedPtr gen, Quantization quantize)
{
	boost::mutex::scoped_lock lock(mtx_);

	Part part = {0, 0, BAR, gen->Generate()};
	parts_.push_back(part);
}

void Track::Clear()
{
	boost::mutex::scoped_lock lock(mtx_);
	parts_.clear();
}

void Track::Update(float songTime, float elapsedTime, vector<Event>& events, vector<float>& offsets)
{
	boost::mutex::scoped_lock lock(mtx_);

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
			NoteOffEvent off;
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

	for (vector<Part>::iterator i = parts_.begin(); i != parts_.end(); i++)
	{
		Part& part = *i;

		float timeUsed = 0;
		while (timeUsed < elapsedTime)
		{
			float timeUsedThisIteration = 0;

			// if there is left over time from a previously encountered rest, then consume it.
			if (part.waitTime > 0) 
			{
				if (timeUsed + part.waitTime > elapsedTime) {
					// wait time is bigger than time left in window. use up as much wait time as we can.
					float timeLeftInFrame = elapsedTime - timeUsed;
					part.waitTime -= timeLeftInFrame;
					timeUsed = elapsedTime;
					timeUsedThisIteration = timeLeftInFrame;
				}
				else {
					// wait time is smaller than window. use up remaining wait time.
					timeUsed += part.waitTime;
					timeUsedThisIteration = part.waitTime;
					part.waitTime = 0;
				}
			}
			else
			{
				// get the next event in the list
				if (part.currentEvent >= part.events->size()) {
					break;
				}
				ValueSharedPtr value = part.events->at(part.currentEvent);
				if (!value) {
					break;
				}
				else if (Music::NoteSharedPtr* note = boost::get<NoteSharedPtr>(value.get())) 
				{
					NoteSharedPtr n = *note;
					ActiveNote newActiveNote;
					newActiveNote.pitch = n->pitch;
					// timeUsed is added to active note length because we subtract entire 
					// window size when udpating active notes
					newActiveNote.timeLeft = BeatsToMilliseconds(n->length) + timeUsed;

					map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(n->pitch);
					if (activeNoteIter != activeNotes_.end()) {
						// note is already on, turn it off
						NoteOffEvent noteOffEvent;
						noteOffEvent.pitch = n->pitch;
						events.push_back(noteOffEvent);
						offsets.push_back(timeUsed);

						// replace currently active note at this pitch
						ActiveNote& activeNote = activeNoteIter->second;
						activeNote = newActiveNote;
					}
					else {
						// new active note
						activeNotes_[n->pitch] = newActiveNote;
					}
					NoteOnEvent noteOnEvent;
					noteOnEvent.pitch = n->pitch;
					noteOnEvent.velocity = n->velocity;
					events.push_back(noteOnEvent);
					offsets.push_back(timeUsed);
				}
				else if (Music::RestSharedPtr* rest = boost::get<RestSharedPtr>(value.get())) {
					RestSharedPtr r = *rest;
					part.waitTime += BeatsToMilliseconds(r->length);
				}
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
			NoteOffEvent off;
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

}
