#pragma once
#include <nlohmann/json.hpp>

static nlohmann::json mergeMap;

namespace MergeMapper
{
	/// @brief Search the data directory for any zmerge merges. This searches for map.json files to build a mapping table.
	/// @return true if any merge mappings found
	bool GetMerges();

	/// @brief Get the new modName and formID
	/// @param oldName The original modName string e.g., Dragonborn.esp
	/// @param oldFormID The original formID in hex format as a string e.g., 0x134ab
	/// @return a pair with string modName and uint32 FormID. If no merge is found, it will return oldName and oldFormID.
	std::pair<std::string, RE::FormID> GetNewFormID(std::string oldName, std::string oldFormID);
}
