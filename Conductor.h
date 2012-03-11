#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <boost/variant.hpp>
#include "SafeQueue.h"
#include "music.h"

enum ConductorCommandType
{
	PLAY_PATTERN,
	UPDATE_PATTERN
};

typedef boost::variant<Pattern> ConductorCommandData;

struct ConductorCommand
{
	ConductorCommandType type;
	ConductorCommandData data;
};

class Conductor
{
public:
	void PushCommand(const ConductorCommand& command);
	bool PopCommand(ConductorCommand& command);

private:
	SafeQueue<ConductorCommand> commands_;
};

#endif