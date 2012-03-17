#include "Music.h"

using namespace std;

namespace Music
{

float BPM = 120;
float BEAT_LENGTH = 1 / BPM * 60000;

const string ScaleStrings[NumScales] = 
{
	"CMAJ",
	"CMIN"
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

void ParsePitchString(const std::string& str, Scale& scale, short& octave, short& degree)
{
	scale = NO_SCALE;
	octave = 1;
	degree = 1;

	size_t firstSplit = str.find_first_of('_');
	size_t secondSplit = str.find_last_of('_');
	string scaleStr = str.substr(0, firstSplit);
	string octaveStr = str.substr(firstSplit+1, 1);
	string degreeStr = str.substr(secondSplit+1, 1);
	//cout << scaleStr << " " << octaveStr << " " << degreeStr << endl;
	for (int i=0; i<NumScales; i++) {
		if (scaleStr.compare(ScaleStrings[i]) == 0) {
			scale = (Scale)i;
			break;
		}
	}
	if (scale == NO_SCALE) {
		cout << "Invalid scale: " << scaleStr << endl;
		scale = CMAJ;
	}
	stringstream octaveStream(octaveStr);
	octaveStream >> octave;
	stringstream degreeStream(degreeStr);
	degreeStream >> degree;
}

NoteGenerator::NoteGenerator(GeneratorPtr pitchGen, GeneratorPtr velocityGen, GeneratorPtr lengthGen) :
	pitchGen_(pitchGen), velocityGen_(velocityGen), lengthGen_(lengthGen)
{
}

boost::shared_ptr<Value> NoteGenerator::Generate()
{
	boost::shared_ptr<Value> pitchValue = pitchGen_->Generate();
	boost::shared_ptr<Value> velocityValue = velocityGen_->Generate();
	boost::shared_ptr<Value> lengthValue = lengthGen_->Generate();

	// Create and return a Note value
	std::string* pitchStr = boost::get<std::string>(pitchValue.get());
	if (!pitchStr) {
		return boost::shared_ptr<Value>();
	}

	int* velocityPtr = boost::get<int>(velocityValue.get());
	int velocity = 127;
	if (velocityPtr != NULL) {
		velocity = *velocityPtr;
	}

	float* lengthPtr = boost::get<float>(lengthValue.get());
	float length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	Scale scale;
	short octave;
	short degree;
	ParsePitchString(*pitchStr, scale, octave, degree);

	boost::shared_ptr<Note> note = boost::shared_ptr<Note>(new Note);
	note->pitch = GetMidiPitch(scale, octave, degree);
	note->velocity = velocity;
	note->length = length;

	return boost::shared_ptr<Value>(new Value(note));
}

WeightedGenerator::WeightedGenerator(const vector<WeightedValue>& values) : values_(values)
{
}

boost::shared_ptr<Value> WeightedGenerator::Generate()
{
	unsigned long total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
	}
	// TODO: make uniform distribution
	unsigned long num = rand() % total;
	total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
		if (total > num) {
			// found the item we are choosing, return it
			boost::shared_ptr<Value> value = values_[i].first->Generate();
			return value;
		}
	}

	return boost::shared_ptr<Value>();
}

PatternGenerator::PatternGenerator(std::vector<GeneratorPtr>& values) : values_(values), current_(0)
{
}

boost::shared_ptr<Value> PatternGenerator::Generate()
{
	unsigned long size = values_.size();
	if (current_ >= size) current_ = 0;
	boost::shared_ptr<Value> value = values_[current_]->Generate();
	current_++;
	return value;
}


Track::Track() {}

void Track::Add(boost::shared_ptr<Generator> gen, Quantization quantize)
{
	boost::mutex::scoped_lock lock(mtx_);

	GeneratorInfo genInfo = {0, BAR, gen};
	generators_.push_back(genInfo);
}

void Track::Clear()
{
	boost::mutex::scoped_lock lock(mtx_);
	generators_.clear();
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

	for (vector<GeneratorInfo>::iterator i = generators_.begin(); i != generators_.end(); i++)
	{
		GeneratorInfo& genInfo = *i;

		float timeUsed = 0;
		while (timeUsed < elapsedTime) 
		{
			float timeUsedThisIteration = 0;

			// if there is left over time from a previously encountered rest, then consume it.
			if (genInfo.waitTime > 0) 
			{
				if (timeUsed + genInfo.waitTime > elapsedTime) {
					// wait time is bigger than time left in window. use up as much wait time as we can.
					float timeLeftInFrame = elapsedTime - timeUsed;
					genInfo.waitTime -= timeLeftInFrame;
					timeUsed = elapsedTime;
					timeUsedThisIteration = timeLeftInFrame;
				}
				else {
					// wait time is smaller than window. use up remaining wait time.
					timeUsed += genInfo.waitTime;
					timeUsedThisIteration = genInfo.waitTime;
					genInfo.waitTime = 0;
				}
			}
			else
			{
				boost::shared_ptr<Value> value = genInfo.generator->Generate();
				if (Note* note = boost::get<Note>(value.get())) {

					ActiveNote newActiveNote;
					newActiveNote.pitch = note->pitch;
					// timeUsed is added to active note length because we subtract entire 
					// window size when udpating active notes
					newActiveNote.timeLeft = BeatsToMilliseconds(note->length) + timeUsed;

					map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(note->pitch);
					if (activeNoteIter != activeNotes_.end()) {
						// note is already on, turn it off
						NoteOffEvent noteOffEvent;
						noteOffEvent.pitch = note->pitch;
						events.push_back(noteOffEvent);
						offsets.push_back(timeUsed);

						// replace currently active note at this pitch
						ActiveNote& activeNote = activeNoteIter->second;
						activeNote = newActiveNote;
					}
					else {
						// new active note
						activeNotes_[note->pitch] = newActiveNote;
					}
					NoteOnEvent noteOnEvent;
					noteOnEvent.pitch = note->pitch;
					noteOnEvent.velocity = note->velocity;
					events.push_back(noteOnEvent);
					offsets.push_back(timeUsed);
				}
				else if (Rest* rest = boost::get<Rest>(value.get())) {
					genInfo.waitTime += rest->length;
				}
			}
		}
	}

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
			activeNote.timeLeft -= elapsedTime;
			it++;
		}
	}
}

}
