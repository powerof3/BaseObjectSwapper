#include "Manager.h"

namespace FormSwap
{
	SwapMap<SwapDataVec>& Manager::get_form_map(const std::string& a_str)
	{
		return a_str == "Forms" ? swapForms : swapRefs;
	}

	void Manager::get_forms_impl(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, SwapData&)> a_func)
	{
		const auto formPair = string::split(a_str, "|");

		if (const auto baseFormID = SwapData::GetFormID(formPair[0]); baseFormID != 0) {
			if (const auto swapFormID = SwapData::GetSwapFormID(formPair[1]); !swap_empty(swapFormID)) {
				const SwapData::Input input(
					formPair.size() > 2 ? formPair[2] : std::string{},  // transform
					formPair.size() > 3 ? formPair[3] : std::string{},  // traits
					a_str,
					a_path);
				SwapData swapData(swapFormID, input);
				a_func(baseFormID, swapData);
			} else {
				logger::error("			failed to process {} (SWAP formID not found)", a_str);
			}
		} else {
			logger::error("			failed to process {} (BASE formID not found)", a_str);
		}
	}

	void Manager::get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map)
	{
		return get_forms_impl(a_path, a_str, [&](RE::FormID a_baseID, const SwapData& a_swapData) {
			a_map[a_baseID].push_back(a_swapData);
		});
	}

	void Manager::get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs, SwapMap<SwapDataConditional>& a_map)
	{
		return get_forms_impl(a_path, a_str, [&](const RE::FormID a_baseID, const SwapData& a_swapData) {
			for (auto& id : a_conditionalIDs) {
				a_map[a_baseID][id].push_back(a_swapData);
			}
		});
	}

	bool Manager::LoadFormsOnce()
	{
		if (init) {
			return true;
		}

		init = true;

		logger::info("{:*^30}", "INI");

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

		std::ranges::sort(configs);

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

			constexpr auto push_filter = [](const std::string& a_condition, std::vector<FormIDStr>& a_processedFilters) {
				if (const auto processedID = SwapData::GetFormID(a_condition); processedID != 0) {
					a_processedFilters.emplace_back(processedID);
				} else {
					logger::error("		Filter  [{}] INFO - unable to find form, treating filter as cell editorID", a_condition);
					a_processedFilters.emplace_back(a_condition);
				}
			};

			for (auto& [section, comment, keyOrder] : sections) {
				if (string::icontains(section, "|")) {
					auto conditions = string::split(string::split(section, "|")[1], ",");  //[Forms|EditorID,EditorID2]

					logger::info("		reading [Forms] : {} conditions", conditions.size());

					std::vector<FormIDStr> processedConditions;
					processedConditions.reserve(conditions.size());
					for (auto& condition : conditions) {
						push_filter(condition, processedConditions);
					}

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section, values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						logger::info("				{} swaps found", values.size());
						for (const auto& key : values) {
							get_forms(path, key.pItem, processedConditions, swapFormsConditional);
						}
					}
				} else {
					logger::info("		reading [{}]", section);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section, values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						logger::info("			{} swaps found", values.size());
						auto& map = get_form_map(section);
						for (const auto& key : values) {
							get_forms(path, key.pItem, map);
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

		const auto log_conflicts = [&](std::string_view a_type, const SwapMap<SwapDataVec>& a_map) {
			if (a_map.empty()) {
				return;
			}
		    logger::info("[{}]", a_type);
			bool conflicts = false;
			for (auto& [baseID, swapDataVec] : a_map) {
				if (swapDataVec.size() > 1) {
					auto winningForm = string::split(swapDataVec.back().record, "|");
					if (winningForm.size() > 3 && winningForm[3].contains("chance")) {  //ignore if winning record is randomized
						continue;
					}
					conflicts = true;
					logger::warn("	{}", winningForm[0]);
					logger::warn("		winning record : {} [{}]", winningForm[1], swapDataVec.back().path);
					logger::warn("		{} conflicts", swapDataVec.size() - 1);
					for (auto it = swapDataVec.rbegin() + 1; it != swapDataVec.rend(); ++it) {
						auto losingRecord = it->record.substr(it->record.find('|') + 1);
						logger::warn("			{} [{}]", losingRecord, it->path);
					}
				}
			}
			if (!conflicts) {
				logger::info("	No conflicts found");
			} else {
				hasConflicts = true;
			}
		};

		log_conflicts("Forms"sv, swapForms);
		log_conflicts("References"sv, swapRefs);

		logger::info("{:*^30}", "END");

		return !swapForms.empty() || !swapFormsConditional.empty() || !swapRefs.empty();
	}

	void Manager::PrintConflicts() const
	{
		if (const auto console = RE::ConsoleLog::GetSingleton(); hasConflicts) {
			console->Print("[BOS] Conflicts found, check po3_BaseObjectSwapper.log in %s for more info\n", logger::log_directory()->string().c_str());
		}
	}

	SwapResult Manager::GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		if (const auto it = swapFormsConditional.find(a_base->GetFormID()); it != swapFormsConditional.end()) {
			auto cell = a_ref->GetParentCell();
			if (!cell) {
				cell = a_ref->GetSaveParentCell();
			}
			const auto currentLocation = a_ref->GetCurrentLocation();

			const auto result = std::ranges::find_if(it->second, [&](const auto& formData) {
				if (std::holds_alternative<RE::FormID>(formData.first)) {
					if (auto form = RE::TESForm::LookupByID(std::get<RE::FormID>(formData.first)); form) {
						switch (form->GetFormType()) {
						case RE::FormType::Location:
							{
								auto location = form->As<RE::BGSLocation>();
								return currentLocation && (currentLocation == location || currentLocation->IsParent(location));
							}
						case RE::FormType::Cell:
							return cell && cell == form;
						case RE::FormType::Keyword:
							{
								auto keyword = form->As<RE::BGSKeyword>();
								return (currentLocation && currentLocation->HasKeyword(keyword)) || a_ref->HasKeyword(keyword);
							}
						default:
							break;
						}
					}
				} else {
					const std::string editorID = std::get<std::string>(formData.first);
					return cell && cell->GetFormEditorID() == editorID;
				}
				return false;
			});

			if (result != it->second.end()) {
				for (auto& swapData : result->second | std::ranges::views::reverse) {
					if (auto swapObject = swapData.GetSwapBase(a_ref)) {
						return { swapObject, swapData.transform };
					}
				}
			}
		}

		return { nullptr, std::nullopt };
	}

	SwapResult Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		const auto get_swap_base = [a_ref](const RE::TESForm* a_form, const SwapMap<SwapDataVec>& a_map) {
			if (const auto it = a_map.find(a_form->GetFormID()); it != a_map.end()) {
				for (auto& swapData : it->second | std::ranges::views::reverse) {
					if (auto swapObject = swapData.GetSwapBase(a_ref)) {
						return SwapResult{ swapObject, swapData.transform };
					}
				}
			}
			return SwapResult{ nullptr, std::nullopt };
		};

		SwapResult swapData{ nullptr, std::nullopt };

		if (!a_ref->IsDynamicForm()) {
			swapData = get_swap_base(a_ref, swapRefs);
		}

		if (!std::get<0>(swapData)) {
			swapData = GetSwapConditionalBase(a_ref, a_base);
		}

		if (!std::get<0>(swapData)) {
			swapData = get_swap_base(a_base, swapForms);
		}

		if (const auto swapLvlBase = std::get<0>(swapData) ? std::get<0>(swapData)->As<RE::TESLevItem>() : nullptr) {
			RE::BSScrapArray<RE::CALCED_OBJECT> calcedObjects{};
			swapLvlBase->CalculateCurrentFormList(a_ref->GetCalcLevel(false), 1, calcedObjects, 0, true);
			if (!calcedObjects.empty()) {
				std::get<0>(swapData) = static_cast<RE::TESBoundObject*>(calcedObjects.front().form);
			}
		}

		return swapData;
	}
}
