#ifndef GENS_H
#define GENS_H

#include <string>
#include <vector>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

namespace MusicGen
{

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
	virtual boost::shared_ptr<Value> Generate()
	{
		return shared_ptr(new Value(val));
	}

	T val;
};

class NoteGenerator : public Generator
{
public:
	NoteGenerator(Generator* pitchGen, Generator* velocityGen, Generator* lengthGen);
	virtual boost::shared_ptr<Value> Generate();

private:
	Generator* pitchGen_;
	Generator* velocityGen_;
	Generator* lengthGen_;
};

class WeightedGenerator : public Generator
{
public:
	typedef std::pair<boost::shared_ptr<Generator>, float> WeightedValue;

	WeightedGenerator(std::vector<const WeightedValue>& values);
	virtual boost::shared_ptr<Value> Generate();

private:
	std::vector<WeightedValue> values_;
};

}

#endif