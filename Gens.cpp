#include "Gens.h"
#include "music.h"
#include <boost/shared_ptr.hpp>

using namespace boost;
using namespace std;

namespace MusicGen
{

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

NoteGenerator::NoteGenerator(Generator* pitchGen, Generator* velocityGen, Generator* lengthGen) :
	pitchGen_(pitchGen), velocityGen_(velocityGen), lengthGen_(lengthGen)
{
}

shared_ptr<Value> NoteGenerator::Generate()
{
	Value* pitchValue = pitchGen_->Generate();
	Value* velocityValue = velocityGen_->Generate();
	Value* lengthValue = lengthGen_->Generate();

	// Create and return a Note value
	std::string* pitchStr = boost::get<std::string>(pitchValue);
	if (!pitchStr) {
		return boost::shared_ptr();
	}

	int* velocityPtr = boost::get<int>(velocityValue);
	int velocity = 127;
	if (velocityPtr != NULL) {
		velocity = *velocityPtr;
	}

	float* lengthPtr = boost::get<float>(lengthValue);
	float length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	Scale scale;
	short octave;
	short degree;
	ParsePitchString(*pitchStr, scale, octave, degree);

	shared_ptr<Note> note = shared_ptr(new Note);
	note->pitch = GetMidiPitch(scale, octave, degree);
	note->velocity = velocity;
	note->length = length;

	return shared_ptr(new Value(note));
}

WeightedGenerator::WeightedGenerator(vector<const WeightedValue>& values)
{
}

shared_ptr<Value> WeightedGenerator::Generate()
{
	return shared_ptr;
}

}
