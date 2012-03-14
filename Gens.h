#ifdef GENS_H
#define GENS_H

namespace MusicGen
{

enum ValueType
{
	STRING,
	INTEGER,
	NOTE,
	REST
};

class Value
{
public:
	ValueType type_;

	std::string str_;
	int int_;
	Note* note_;
	Rest* rest_;
};

class Generator
{
public:
	virtual Value* Generate() = 0;
};

class NoteGenerator : public Generator
{
public:
	NoteGenerator(Generator* pitchGen, Generator* velocityGen, Generator* lengthGen);
	virtual Value* Generate();

private:
	Generator* pitchGen_;
	Generator* velocityGen_;
	Generator* lengthGen_;
};

}

#endif