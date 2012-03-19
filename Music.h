#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace Music
{

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

enum Quantization
{
	BEAT,
	BAR
};

unsigned short GetMidiPitch(Scale scale, int octave, int degree);
const char* GetScaleName(Scale scale);
float BeatsToMilliseconds(float beats);

class Note
{
public:
	short pitch;
	short velocity;
	float length;
};

class Rest
{
public:
	float length;
};

typedef boost::variant<std::string, int, float, boost::shared_ptr<Note>, boost::shared_ptr<Rest> > Value;

class Generator
{
public:
	virtual boost::shared_ptr<Value> Generate() = 0;
};

template <typename T>
class SingleValueGenerator : public Generator
{
public:
	SingleValueGenerator(T val) : val_(val) {}

	virtual boost::shared_ptr<Value> Generate()
	{
		return boost::shared_ptr<Value>(new Value(val_));
	}

	T val_;
};

typedef boost::shared_ptr<Generator> GeneratorPtr;

class NoteGenerator : public Generator
{
public:
	NoteGenerator(GeneratorPtr pitchGen, GeneratorPtr velocityGen, GeneratorPtr lengthGen);
	virtual boost::shared_ptr<Value> Generate();

private:
	GeneratorPtr pitchGen_;
	GeneratorPtr velocityGen_;
	GeneratorPtr lengthGen_;
};

class RestGenerator : public Generator
{
public:
	RestGenerator(GeneratorPtr lengthGen) : lengthGen_(lengthGen) {};
	virtual boost::shared_ptr<Value> Generate();

private:
	GeneratorPtr lengthGen_;
};

class PatternGenerator : public Generator
{
public:
	PatternGenerator(std::vector<GeneratorPtr >& values);
	virtual boost::shared_ptr<Value> Generate();

private:
	std::vector<GeneratorPtr > values_;
	unsigned long current_;
};

class WeightedGenerator : public Generator
{
public:
	typedef std::pair<GeneratorPtr, unsigned long> WeightedValue;

	WeightedGenerator(const std::vector<WeightedValue>& values);
	virtual boost::shared_ptr<Value> Generate();

private:
	std::vector<WeightedValue> values_;
};

class Track
{
public:
	Track();

	struct NoteOnEvent
	{
		short pitch;
		short velocity;
	};
	struct NoteOffEvent
	{
		short pitch;
	};
	typedef boost::variant<NoteOnEvent, NoteOffEvent> Event;

	void Add(boost::shared_ptr<Generator> gen, Quantization quantize);
	void Clear();

	void Update(float songTime, float elapsedTime, std::vector<Event>& events, std::vector<float>& offsets);

private:

	struct GeneratorInfo
	{
		float waitTime;
		Quantization quantize;
		boost::shared_ptr<Generator> generator;
	};

	struct ActiveNote
	{
		short pitch;
		float timeLeft;
	};
	std::vector<GeneratorInfo> generators_;
	std::map<short, ActiveNote> activeNotes_;

	boost::mutex mtx_;
};

}

#endif
