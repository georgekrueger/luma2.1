#ifndef __LUAFUNC_H__
#define __LUAFUNC_H__

void luma_error(lua_State* luaState, char* errorMsg)
{
	lua_pushstring(luaState, errorMsg);
	lua_error(luaState);
}

int luma_rand(lua_State *luaState)
{
	int r = (rand() % 1000) + 1;
	// push key to top of stack
	lua_pushnumber(luaState, r);

	return 1;
}

#endif // __LUAFUNC_H__