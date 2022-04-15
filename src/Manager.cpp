#include "Manager.h"

namespace FormSwap
{
	Manager::SwapMap<SwapData>& Manager::get_form_map(const std::string& a_str)
	{
		return a_str == "Forms" ? swapForms : swapRefs;
	}

	Manager::ConflictMap& Manager::get_conflict_map(const std::string& a_str)
	{
		return a_str == "Forms" ? conflictForms : conflictRefs;
	}

	std::pair<bool, RE::FormID> Manager::get_forms_impl(const std::string& a_str, std::function<void(RE::FormID a_baseID, SwapData& a_swapData)> a_func)
	{
		const auto formPair = string::split(a_str, "|");

		auto baseFormID = SwapData::GetFormID(formPair[0]);
		auto swapFormID = SwapData::GetFormID(formPair[1]);

		const auto data = formPair.size() > 2 ? formPair[2] : std::string{};

		if (swapFormID != 0 && baseFormID != 0) {
			SwapData swapData{
				swapFormID,
				Transform{ data }
			};

			a_func(baseFormID, swapData);

			return { true, baseFormID };
		}

		logger::error("			failed to process {} [{:x}|{:x}|{}] (formID not found)", a_str, baseFormID, swapFormID, data);

		return { false, 0 };
	}

	std::pair<bool, RE::FormID> Manager::get_forms(const std::string& a_str, SwapMap<SwapData>& a_map)
	{
		return get_forms_impl(a_str, [&](RE::FormID a_baseID, SwapData& a_swapData) { a_map.insert_or_assign(a_baseID, a_swapData); });
	}

	std::pair<bool, RE::FormID> Manager::get_forms(const std::string& a_str, const std::vector<std::string>& a_conditionalIDs, SwapMap<SwapDataConditional>& a_map)
	{
		return get_forms_impl(a_str, [&](RE::FormID a_baseID, SwapData& a_swapData) {
			for (auto& id : a_conditionalIDs) {
				if (const auto processedID = SwapData::GetFormID(id); processedID != 0) {
					a_map[a_baseID].insert_or_assign(processedID, a_swapData);
				}
			}
		});
	}

	bool Manager::LoadFormsOnce()
	{
		if (init) {
			return true;
		}

		init = true;

		std::vector<std::string> configs;

		auto constexpr folder = R"(Data\)";
		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
				if (const auto path = entry.path().string(); path.rfind("_SWAP") != std::string::npos) {
					configs.push_back(path);
				}
			}
		}

		if (configs.empty()) {
			logger::warn("	No .ini files with _SWAP suffix were found within the Data folder, aborting...");
			return false;
		}

		logger::info("	{} matching inis found", configs.size());

		for (auto& path : configs) {
			logger::info("	INI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();
			ini.SetAllowKeyOnly();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				logger::error("	couldn't read INI");
				continue;
			}

			CSimpleIniA::TNamesDepend sections;
			ini.GetAllSections(sections);
			sections.sort(CSimpleIniA::Entry::LoadOrder());

			constexpr auto map_conflicts = [](const std::string& a_str, const std::string& a_path, RE::FormID a_baseID, ConflictMap& a_conflictMap) {
				a_conflictMap[a_baseID].emplace_back(std::make_pair(a_str, a_path));
			};

			for (auto& [section, comment, keyOrder] : sections) {
				logger::info("		reading [{}]", section);
				if (const auto values = ini.GetSection(section); values && !values->empty()) {
					logger::info("			{} values found", values->size());
					if (string::icontains(section, "|")) {
						auto locations = string::split(string::split(section, "|")[1], ",");  //[Forms|EditorID,EditorID2]
						for (const auto& key : *values | std::views::keys) {
							std::string value(key.pItem);
							if (auto [result, baseID] = get_forms(value, locations, swapFormsConditional); result) {
								map_conflicts(value, path, baseID, conflictForms);
							}
						}
					} else {
						auto& map = get_form_map(section);
						auto& conflictMap = get_conflict_map(section);
						for (const auto& key : *values | std::views::keys) {
							std::string value(key.pItem);
							if (auto [result, baseID] = get_forms(value, map); result) {
								map_conflicts(value, path, baseID, conflictMap);
							}
						}
					}
				}
			}
		}

		logger::info("{:*^30}", "RESULT");

		logger::info("{} form-form swaps done", swapForms.size());
		logger::info("{} conditional form swaps done", swapFormsConditional.size());
		logger::info("{} ref-form swaps done", swapRefs.size());

		logger::info("{:*^30}", "CONFLICTS");

		const auto log_conflicts = [&](std::string_view a_type, const ConflictMap& a_conflictMap) {
			logger::info("[{}]", a_type);
			for (auto& [baseID, conflicts] : a_conflictMap) {
				if (conflicts.size() > 1) {
					hasConflicts = true;

					auto winningForm = string::split(conflicts.back().first, "|");
					logger::warn("	{}", winningForm[0]);
					logger::warn("		winning record : {} [{}]", winningForm[1], conflicts.back().second);
					logger::warn("		{} conflicts", conflicts.size() - 1);
					for (auto it = conflicts.begin(); it != std::prev(conflicts.end()); ++it) {
						auto& [record, path] = *it;
						auto conflictForm = string::split(record, "|");
						logger::warn("			{} [{}]", conflictForm[1], path);
					}
				}
			}
		};

		log_conflicts("Forms"sv, conflictForms);
		log_conflicts("References"sv, conflictRefs);

		logger::info("{:*^30}", "END");

		return !swapForms.empty() || !swapFormsConditional.empty() || !swapRefs.empty();
	}

	void Manager::PrintConflicts() const
	{
		if (const auto console = RE::ConsoleLog::GetSingleton(); hasConflicts) {
			console->Print("~BASE OBJECT SWAPPER~");
			console->Print("Conflicts detected, check po3_BaseObjectSwapper.log in %s for more details\n", logger::log_directory()->string().c_str());
		}
	}

	Manager::SwapResult Manager::GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		if (const auto it = swapFormsConditional.find(a_base->GetFormID()); it != swapFormsConditional.end()) {
			const auto result = std::ranges::find_if(it->second, [&](const auto& formData) {
				if (auto form = RE::TESForm::LookupByID(formData.first)) {
					switch (form->GetFormType()) {
					case RE::FormType::Location:
						{
							const auto cell = a_ref->GetParentCell();
							const auto location = cell ? cell->GetLocation() : nullptr;

							return location == form;
						}
					case RE::FormType::Cell:
						{
							const auto cell = a_ref->GetParentCell();
							return cell && cell == form;
						}
					default:
						break;
					}
				}
				return false;
			});

			if (result != it->second.end()) {
				return { RE::TESForm::LookupByID<RE::TESBoundObject>(result->second.formID), result->second.transform };
			}
		}

		return { nullptr, Transform() };
	}

	Manager::SwapResult Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		constexpr auto get_swap_base = [](const RE::TESForm* a_form, const SwapMap<SwapData>& a_map) {
			if (const auto it = a_map.find(a_form->GetFormID()); it != a_map.end()) {
				return SwapResult{ RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.transform };
			}
			return SwapResult{};
		};

		SwapResult swapData{ std::make_pair(nullptr, Transform()) };

		if (!a_ref->IsDynamicForm()) {
			swapData = get_swap_base(a_ref, swapRefs);
		}

		if (!swapData.first) {
			swapData = GetSwapConditionalBase(a_ref, a_base);
		}

		if (!swapData.first) {
			swapData = get_swap_base(a_base, swapForms);
		}

		return swapData;
	}
}
