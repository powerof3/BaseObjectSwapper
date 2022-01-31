#include "MergeMapper.h"

namespace MergeMapper
{
	bool GetMerges()
	{
		using json = nlohmann::json;
		logger::info("Searching for merges within the Data folder");
		auto constexpr folder = R"(Data\)";
		json json_data;
		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			// zMerge folders have name "merge - 018auri"
			auto constexpr mergePrefix = R"(Data\merge - )";
			if (entry.exists() && entry.is_directory() && entry.path().string().starts_with(mergePrefix)) {
				const auto path = entry.path().string();

				auto file = path + "/map.json";
				auto merged = path.substr(13) + ".esp";
				try {
					std::ifstream json_file(file);
					json_file >> json_data;
					json_file.close();
				} catch (std::exception& e) {
					logger::warn("	Unable to open {}:{}", file, e.what());
				}
				if (!json_data.empty()) {
					for (auto& [esp, idmap] : json_data.items()) {
						logger::debug(" Found {} maps to {} with {} mappings", esp, merged, idmap.size());
						if (mergeMap.contains(esp))
							logger::warn(" Duplicate {} found in {}", esp, merged);
						mergeMap[esp]["name"] = merged;
						mergeMap[esp]["map"] = idmap;
					}
				}
			}
		}
		if (mergeMap.empty()) {
			logger::info("	No merges were found within the Data folder");
			return false;
		}
		logger::info("	{} merges found", mergeMap.size());
		return true;
	}

	std::pair<std::string, RE::FormID> GetNewFormID(std::string oldName, std::string oldFormID)
	{
		std::string modName = oldName;
		RE::FormID formID = std::stoi(oldFormID, 0, 16);
		//check for merged esps
		if (mergeMap.contains(oldName)) {
			modName = mergeMap[oldName]["name"];
			if (!mergeMap[oldName]["map"].empty()) {
				if (mergeMap[oldName]["map"].contains(oldFormID)) {
					formID = std::stoi(mergeMap[oldName]["map"][oldFormID].get<std::string>());
				}
			}
		}
		return std::make_pair(modName, formID);
	}
}
