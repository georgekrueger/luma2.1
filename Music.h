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
	MAJ,
	MIN,
	PENTAMIN,
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

typedef boost::shared_ptr<Note> NoteSharedPtr;
typedef boost::shared_ptr<Rest> RestSharedPtr;

typedef boost::variant<std::string, int, float, NoteSharedPtr, RestSharedPtr> Value;
typedef boost::shared_ptr<Value> ValueSharedPtr;

class Generator;
typedef boost::shared_ptr<Generator> GeneratorSharedPtr;
typedef boost::variant<std::string, int, float, GeneratorSharedPtr> GeneratorInput;

class GenResult
{
public:
	GenResult(ValueSharedPtr value, bool done) : value_(value), done_(done) {}
	ValueSharedPtr GetValue() { return value_; }
	bool IsDone() { return done_; }
	void SetDone(bool b) { done_ = b; }
private:
	ValueSharedPtr value_;
	bool done_;
};

///////////////////////////
// Generator base class
///////////////////////////
class Generator
{
public:
	Generator(std::vector<GeneratorInput> inputs) : inputs_(inputs), canStep_(true) {}
	GenResult Generate();

protected:
	ValueSharedPtr GenerateInput(unsigned long i);

	// Step the generator forward and return the current ge
	virtual void DoStep() = 0;
	virtual GenResult DoGenerate() = 0;

private:
	std::vector<GeneratorInput> inputs_;
	std::vector<GenResult> activeGenResults_;
	bool canStep_;
	bool isDone_;
};

///////////////////////////
// Note generator
///////////////////////////
class NoteGenerator : public Generator
{
public:
	NoteGenerator(std::vector<GeneratorInput> inputs) : Generator(inputs) {}
	virtual void DoStep() {}
	virtual GenResult DoGenerate();
};
typedef boost::shared_ptr<NoteGenerator> NoteGenSharedPtr;

///////////////////////////
// Rest generator
///////////////////////////
class RestGenerator : public Generator
{
public:
	RestGenerator(std::vector<GeneratorInput> inputs) : Generator(inputs) {}
	virtual void DoStep() {}
	virtual GenResult DoGenerate();
};
typedef boost::shared_ptr<RestGenerator> RestGenSharedPtr;

///////////////////////////
// Pattern generator
///////////////////////////
class PatternGenerator : public Generator
{
public:
	PatternGenerator(std::vector<GeneratorInput> inputs , unsigned long repeat);
	virtual void DoStep();
	virtual GenResult DoGenerate();
	//boost::shared_ptr<PatternGenerator> MakeStatic();

private:
	static const unsigned long CURRENT_NONE = ULONG_MAX;
	unsigned long current_;
	unsigned long size_;
	unsigned long repeat_;
	unsigned long numIter_;

	void Reset();
};
typedef boost::shared_ptr<PatternGenerator> PatternGenSharedPtr;
/*
class WeightedGenerator : public Generator
{
public:
	typedef std::pair<GeneratorSharedPtr, unsigned long> WeightedValue;

	WeightedGenerator(const std::vector<WeightedValue>& values);
	virtual boost::shared_ptr<Value> Generate();

private:
	std::vector<WeightedValue> values_;
	GeneratorSharedPtr currentGen_;
};
typedef boost::shared_ptr<WeightedGenerator> WeightedGenPtr;

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
*/
}

#endif
