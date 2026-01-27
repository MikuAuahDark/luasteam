#include "inventory.hpp"
#include "const.hpp"
#include <cassert>
#include <sstream>
#include <vector>

class CallbackListener {
private:
    STEAM_CALLBACK(CallbackListener, OnSteamInventoryDefinitionUpdate, SteamInventoryDefinitionUpdate_t);
    STEAM_CALLBACK(CallbackListener, OnSteamInventoryResultReady, SteamInventoryResultReady_t);
} *inventory_listener = nullptr;
int inventory_ref = LUA_NOREF;

void CallbackListener::OnSteamInventoryDefinitionUpdate(SteamInventoryDefinitionUpdate_t *data) {
    if (data == nullptr) {
        return;
    }
    lua_State *L = luasteam::global_lua_state;

    lua_rawgeti(L, LUA_REGISTRYINDEX, inventory_ref);
    lua_getfield(L, -1, "onSteamInventoryDefinitionUpdate");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
    } else {
        lua_call(L, 0, 0);
        lua_pop(L, 1);
    }
}

void CallbackListener::OnSteamInventoryResultReady(SteamInventoryResultReady_t *data) {
    if (data == nullptr) {
        return;
    }
    lua_State *L = luasteam::global_lua_state;

    lua_rawgeti(L, LUA_REGISTRYINDEX, inventory_ref);
    lua_getfield(L, -1, "onSteamInventoryResultReady");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
    } else {
        lua_createtable(L, 0, 1);
        lua_pushinteger(L, data->m_handle);
        lua_setfield(L, -2, "handle");
        lua_pushstring(L, steam_result_code[data->m_result]);
        lua_setfield(L, -2, "result");
        lua_call(L, 1, 0);
        lua_pop(L, 1);
    }
}



// void DestroyResult( SteamInventoryResult_t resultHandle );
EXTERN int luasteam_destroyResult(lua_State *L) {
    SteamInventoryResult_t handle = (SteamInventoryResult_t) luaL_checkinteger(L, 1);
    SteamInventory()->DestroyResult(handle);
    return 0;
}

// bool GenerateItems( SteamInventoryResult_t *pResultHandle, const SteamItemDef_t *pArrayItemDefs, const uint32 *punArrayQuantity, uint32 unArrayLength );
EXTERN int luasteam_generateItems(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    std::vector<SteamItemDef_t> itemIds;
    std::vector<uint32> itemCounts;

    lua_pushnil(L);  /* first key */
    while (lua_next(L, 1) != 0) {
        if (lua_type(L, -2) == LUA_TNUMBER && lua_type(L, -1) == LUA_TNUMBER) {
            SteamItemDef_t itemId = lua_tointeger(L, -2);
            lua_Integer count = lua_tointeger(L, -1);

            if (itemId < 1 || itemId > 999999999) {
                return luaL_error(L, "Item ID %d is invalid", itemId);
            }
            if (count < 0) {
                return luaL_error(L, "Item ID %d has invalid count %d", itemId, (int) count);
            }

            itemIds.push_back(itemId);
            itemCounts.push_back((uint32) count);
        }

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }

    assert(itemIds.size() == itemCounts.size());

    SteamInventoryResult_t handle = 0;
    if (SteamInventory()->GenerateItems(&handle, itemIds.data(), itemCounts.data(), itemIds.size())) {
        lua_pushinteger(L, handle);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// bool GetAllItems( SteamInventoryResult_t *pResultHandle );
EXTERN int luasteam_getAllItems(lua_State *L) {
    SteamInventoryResult_t handle = 0;

    if (SteamInventory()->GetAllItems(&handle)) {
        lua_pushinteger(L, handle);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// bool GetItemDefinitionIDs( SteamItemDef_t *pItemDefIDs, uint32 *punItemDefIDsArraySize );
EXTERN int luasteam_getItemDefinitionIDs(lua_State *L) {
    uint32 count = 0;
    bool hasItems = false;
    std::vector<SteamItemDef_t> itemDefs;

    if (SteamInventory()->GetItemDefinitionIDs(nullptr, &count)) {
        itemDefs.resize(count);
        hasItems = SteamInventory()->GetItemDefinitionIDs(itemDefs.data(), &count);
    }

    if (hasItems) {
        lua_createtable(L, count, 0);
        for (uint32 i = 0; i < count; i++) {
            lua_pushinteger(L, itemDefs[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}


static int getItemDefinitionProperty(lua_State *L, SteamItemDef_t definition, const std::string &propertyName) {
    uint32 count = 0;
    bool hasPropval = false;

    if (!SteamInventory()->GetItemDefinitionProperty(definition, propertyName.c_str(), nullptr, &count)) {
        lua_pushnil(L);
        return 1;
    }

    std::vector<char> propertyValue(count);
    if (SteamInventory()->GetItemDefinitionProperty(definition, propertyName.c_str(), propertyValue.data(), &count)) {
        lua_pushlstring(L, propertyValue.data(), propertyValue.size());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int getItemDefinitionProperties(lua_State *L, SteamItemDef_t definition) {
    uint32 count = 0;
    bool hasProplist = false;

    if (!SteamInventory()->GetItemDefinitionProperty(definition, nullptr, nullptr, &count)) {
        lua_pushnil(L);
        return 1;
    }

    std::vector<char> propertyNames(count);
    if (!SteamInventory()->GetItemDefinitionProperty(definition, nullptr, propertyNames.data(), &count)) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    std::stringstream ss(std::string(propertyNames.begin(), propertyNames.end()));
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        if (SteamInventory()->GetItemDefinitionProperty(definition, segment.c_str(), nullptr, &count)) {
            std::vector<char> propertyValue(count);

            if (SteamInventory()->GetItemDefinitionProperty(definition, segment.c_str(), propertyValue.data(), &count)) {
                lua_pushlstring(L, segment.data(), segment.length());
                lua_pushlstring(L, propertyValue.data(), propertyValue.size());
                lua_rawset(L, -3);
            }
        }
    }

    return 1;
}

// bool GetItemDefinitionProperty( SteamItemDef_t iDefinition, const char *pchPropertyName, char *pchValueBuffer, uint32 *punValueBufferSizeOut );
EXTERN int luasteam_GetItemDefinitionProperty(lua_State *L) {
    SteamItemDef_t definition = luaL_checkinteger(L, 1);

    const char *propertyName = nullptr;
    size_t propertyNameSize = 0;
    if (lua_isstring(L, 2)) {
        propertyName = lua_tolstring(L, 2, &propertyNameSize);
    }

    if (propertyName) {
        return getItemDefinitionProperty(L, definition, std::string(propertyName, propertyNameSize));
    } else {
        return getItemDefinitionProperties(L, definition);
    }
}

// bool GetResultItems( SteamInventoryResult_t resultHandle, SteamItemDetails_t *pOutItemsArray, uint32 *punOutItemsArraySize );
EXTERN int luasteam_getResultItems(lua_State *L) {
    SteamInventoryResult_t handle = (SteamInventoryResult_t) luaL_checkinteger(L, 1);

    bool hasResult = false;
    std::vector<SteamItemDetails_t> itemDetails;
    uint32 count = 0;
    if (SteamInventory()->GetResultItems(handle, nullptr, &count))
    {
        itemDetails.resize(count);
        hasResult = SteamInventory()->GetResultItems(handle, itemDetails.data(), &count );
    }

    if (!hasResult) {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, count, 0);
    lua_Integer i = 1;
    for (const SteamItemDetails_t &itemDetail: itemDetails) {
        lua_pushinteger(L, i);
        lua_createtable(L, 0, 4);
        lua_pushstring(L, "id");
        luasteam::pushuint64(L, itemDetail.m_itemId);
        lua_rawset(L, -3);
        lua_pushstring(L, "definition");
        lua_pushinteger(L, itemDetail.m_iDefinition);
        lua_rawset(L, -3);
        lua_pushstring(L, "quantity");
        lua_pushinteger(L, itemDetail.m_unQuantity);
        lua_rawset(L, -3);
        lua_pushstring(L, "flags");
        lua_pushinteger(L, itemDetail.m_unFlags);
        lua_rawset(L, -3);

        lua_rawset(L, -3); // For the array itself
        i++;
    }

    return 1;
}

// EResult GetResultStatus( SteamInventoryResult_t resultHandle );
EXTERN int luasteam_getResultStatus(lua_State *L) {
    SteamInventoryResult_t handle = (SteamInventoryResult_t) luaL_checkinteger(L, 1);
    EResult status = SteamInventory()->GetResultStatus(handle);
    lua_pushstring(L, steam_result_code[status]);
    return 1;
}

// bool LoadItemDefinitions();
EXTERN int luasteam_loadItemDefinitions(lua_State *L) {
    lua_pushboolean(L, SteamInventory()->LoadItemDefinitions());
    return 1;
}



namespace luasteam {

void add_inventory(lua_State *L) {
    lua_createtable(L, 0, 8);
    add_func(L, "destroyResult", luasteam_destroyResult);
    add_func(L, "generateItems", luasteam_generateItems);
    add_func(L, "getAllItems", luasteam_getAllItems);
    add_func(L, "getItemDefinitionIDs", luasteam_getItemDefinitionIDs);
    add_func(L, "getItemDefinitionProperty", luasteam_GetItemDefinitionProperty);
    add_func(L, "getResultItems", luasteam_getResultItems);
    add_func(L, "getResultStatus", luasteam_getResultStatus);
    add_func(L, "loadItemDefinitions", luasteam_loadItemDefinitions);
    lua_pushvalue(L, -1);
    inventory_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_setfield(L, -2, "inventory");
}

void init_inventory(lua_State *L) {
    inventory_listener = new CallbackListener();
}

void shutdown_inventory(lua_State *L) {
    luaL_unref(L, LUA_REGISTRYINDEX, inventory_ref);
    inventory_ref = LUA_NOREF;
    delete inventory_listener;
    inventory_listener = nullptr;
}

}
