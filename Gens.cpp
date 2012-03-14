#include "Gens.h"

namespace MusicGen
{

NoteGenerator::NoteGenerator(Generator* pitchGen, Generator* velocityGen, Generator* lengthGen) :
	pitchGen_(pitchGen), velocityGen_(velocityGen), lengthGen_(lengthGen)
{
}

Value* NoteGenerator::Generate()
{
	Value* pitchValue = pitchGen_->Generate();
	Value* velocityValue = velocityGen_->Generate();
	Value* lengthValue = lengthGen_->Generate();

	// Create and return a Note value
}

}