#ifndef LUASTEAM_INVENTORY
#define LUASTEAM_INVENTORY

#include "common.hpp"

namespace luasteam
{

// Adds functions from ISteamInventory
void add_inventory(lua_State *L);

void init_inventory(lua_State *L);
void shutdown_inventory(lua_State *L);

}

#endif
