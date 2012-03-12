
#include "Conductor.h"

void Conductor::PushCommand(const ConductorCommand& command)
{
	commands_.Produce(command);
}

bool Conductor::PopCommand(ConductorCommand& command)
{
	return commands_.Consume(command);
	return true;
}
