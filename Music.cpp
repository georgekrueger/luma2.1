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

class GenerateInputVisitor : public boost::static_visitor<GenResult>
{
public:
	GenResult operator()(int i) const {
        return GenResult(ValueSharedPtr(new Value(i)), true);
    }

	GenResult operator()(float f) const {
		return GenResult(ValueSharedPtr(new Value(f)), true);
    }

	GenResult operator()(std::string s) const {
		return GenResult(ValueSharedPtr(new Value(s)), true);
    }

	GenResult operator()(GeneratorSharedPtr g) const {
		return g->Generate();
    }
};

GenResult Generator::Generate()
{
	if (canStep_) {
		DoStep();
	}

	activeGenResults_.clear();

	GenResult result = DoGenerate();
	
	bool allResultsDone = true;
	for (auto i=activeGenResults_.begin(); i!=activeGenResults_.end(); i++) {
		allResultsDone &= (*i).IsDone();
	}
	canStep_ = allResultsDone;

	result.SetDone(result.IsDone() && allResultsDone);

	return result;
}

ValueSharedPtr Generator::GenerateInput(unsigned long i)
{
	GenResult result = boost::apply_visitor( GenerateInputVisitor(), inputs_[i] );
	activeGenResults_.push_back(result);
	return result.GetValue();
}

GenResult NoteGenerator::DoGenerate()
{
	ValueSharedPtr pitchValue = GenerateInput(0);
	ValueSharedPtr velocityValue = GenerateInput(1);
	ValueSharedPtr lengthValue = GenerateInput(2);

	short pitch;
	if (std::string* pitchStr = boost::get<std::string>(pitchValue.get())) {
		// parse pitch as string
		Scale scale;
		short root;
		short octave;
		short degree;
		ParsePitchString(*pitchStr, scale, root, octave, degree);
		pitch = GetMidiPitch(scale, root, octave, degree);
	}
	else if (int* pitchInt = boost::get<int>(pitchValue.get())) {
		// parse pitch as int
		pitch = *pitchInt;
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

	// Create and return a note value
	NoteSharedPtr note = NoteSharedPtr(new Note);
	note->pitch = pitch;
	note->velocity = velocity;
	note->length = length;

	return GenResult( ValueSharedPtr(new Value(note)), true );
}


GenResult RestGenerator::DoGenerate()
{
	ValueSharedPtr lengthValue = GenerateInput(0);

	float* lengthPtr = boost::get<float>(lengthValue.get());
	float length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	// Create and return a rest value
	RestSharedPtr rest = RestSharedPtr(new Rest);
	rest->length = length;

	return GenResult( ValueSharedPtr(new Value(rest)), true );
}

PatternGenerator::PatternGenerator(std::vector<GeneratorInput> inputs , unsigned long repeat) 
	: Generator(inputs), repeat_(repeat), size_(inputs.size())
{
	Reset();
}

void PatternGenerator::Reset()
{
	numIter_ = 0;
	current_ = CURRENT_NONE;
}

void PatternGenerator::DoStep()
{
	if (current_ == CURRENT_NONE || current_ == size_ - 1) {
		current_ = 0;
	}
	else {
		current_++;
		if (current_ == size_ - 1) numIter_++;
	}
}

GenResult PatternGenerator::DoGenerate()
{
	ValueSharedPtr value = GenerateInput(current_);
	bool done = (numIter_ == repeat_);
	return GenResult( value, done );
}

/*PatternGenSharedPtr PatternGenerator::MakeStatic()
{
	std::vector<GeneratorSharedPtr> gens;

	unsigned long numIter = 0;
	unsigned long current = 0;

	while (!done_) {
		ValueSharedPtr value = Generate();
		if (!value) {
			break;
		}
		else if (NoteSharedPtr* note = boost::get<NoteSharedPtr>(value.get())) 
		{
			GeneratorSharedPtr pitchGen( new SingleValueGenerator<int>((*note)->pitch) );
			GeneratorSharedPtr velGen( new SingleValueGenerator<int>((*note)->velocity) );
			GeneratorSharedPtr lenGen( new SingleValueGenerator<float>((*note)->length) );
			NoteGenSharedPtr noteGen( new NoteGenerator(pitchGen, velGen, lenGen) );
			gens.push_back(noteGen);
		}
		else if (RestSharedPtr* rest = boost::get<RestSharedPtr>(value.get())) {
			GeneratorSharedPtr lenGen( new SingleValueGenerator<float>((*rest)->length) );
			RestGenSharedPtr restGen( new RestGenerator(lenGen) );
			gens.push_back(restGen);
		}
	}

	PatternGenSharedPtr newPatternGen(new PatternGenerator(gens , 1));
	return newPatternGen;
}*/

/*
WeightedGenerator::WeightedGenerator(const vector<WeightedValue>& values) : values_(values)
{
}

boost::shared_ptr<Value> WeightedGenerator::Generate()
{
	if (!randSeeded) {
		srand ( static_cast<unsigned int>(time(NULL)) );
		randSeeded = true;
	}

	boost::shared_ptr<Value> value;

	if (done_)
	{
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
				currentGen_ = values_[i].first;
				break;
			}
		}
	}

	value = currentGen_->Generate();
	done_ = currentGen_->IsDone();

	return value;
}

Track::Track() 
{
}

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
				ValueSharedPtr value = genInfo.generator->Generate();
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
					genInfo.waitTime += BeatsToMilliseconds(r->length);
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
*/
}
