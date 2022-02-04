#include "MergeMapper.h"

namespace MergeMapper
{
	bool GetMerges()
	{
		using json = nlohmann::json;
		logger::info("Searching for merges within the Data folder");
		auto constexpr folder = R"(Data\)";
		json json_data;
		auto total = 0;
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
						auto espkey = esp;
						std::transform(espkey.begin(), espkey.end(), espkey.begin(), ::tolower);
						if (idmap.size()) {
							logger::info(" Found {} maps to {} with {} mappings", esp, merged, idmap.size());
							total += idmap.size();
						}
						if (mergeMap.contains(espkey))
							logger::warn(" Duplicate {} found in {}", esp, merged);
						mergeMap[espkey]["name"] = merged;
						if (!idmap.empty()) {
							for (auto& [key, value] : idmap.items()) {
								auto storedKey = std::to_string(std::stoi(key, 0, 16));
								std::transform(storedKey.begin(), storedKey.end(), storedKey.begin(), ::tolower);
								auto storedValue = std::to_string(std::stoi(value.get<std::string>(), 0, 16));
								std::transform(storedValue.begin(), storedValue.end(), storedValue.begin(), ::tolower);
								mergeMap[espkey]["map"][storedKey] = storedValue;
							}
						}
					}
				}
			}
		}
		if (mergeMap.empty()) {
			logger::info("	No merges were found within the Data folder");
			return false;
		}
		logger::info("	{} merges found with {} mappings", mergeMap.size(), total);
		return true;
	}

	std::pair<std::string, RE::FormID> GetNewFormID(std::string oldName, std::string oldFormID)
	{
		auto modName = oldName;
		auto espkey = oldName;
		std::transform(espkey.begin(), espkey.end(), espkey.begin(), ::tolower);
		RE::FormID formID = std::stoi(oldFormID, 0, 16);
		//check for merged esps
		if (mergeMap.contains(espkey)) {
			modName = mergeMap[espkey]["name"];
			auto storedKey = std::to_string(formID);
			if (!mergeMap[espkey]["map"].empty()) {
				std::transform(storedKey.begin(), storedKey.end(), storedKey.begin(), ::tolower);
				if (mergeMap[espkey]["map"].contains(storedKey)) {
					formID = std::stoi(mergeMap[espkey]["map"][storedKey].get<std::string>());
				}
			}
		}
		return std::make_pair(modName, formID);
	}
}
