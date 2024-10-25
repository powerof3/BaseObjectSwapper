#include "Manager.h"

namespace FormSwap
{
	void Manager::LoadFormsOnce()
	{
		std::call_once(init, [this] {
			LoadForms();
		});
	}

	void Manager::LoadForms()
	{
		logger::info("{:*^30}", "INI");

		std::vector<std::string> configs = distribution::get_configs(R"(Data\)", "_SWAP"sv);

		if (configs.empty()) {
			logger::warn("No .ini files with _SWAP suffix were found within the Data folder, aborting...");
			return;
		}

		logger::info("{} matching inis found...", configs.size());

		for (auto& path : configs) {
			logger::info("INI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();
			ini.SetAllowKeyOnly();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				logger::error("\tcouldn't read INI");
				continue;
			}

			CSimpleIniA::TNamesDepend sections;
			ini.GetAllSections(sections);
			sections.sort(CSimpleIniA::Entry::LoadOrder());

			for (auto& [_section, comment, keyOrder] : sections) {
				std::string section = _section;
				if (section.contains('|')) {
					auto splitSection = string::split(section, "|");
					auto conditions = string::split(splitSection[1], ",");  //[Forms|EditorID,EditorID2]

					logger::info("\treading [{}] : {} conditions", splitSection[0], conditions.size());

					ConditionFilters processedConditions(path.substr(5) + "|" + splitSection[1], conditions);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section.c_str(), values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (splitSection[0] == "Forms") {
							logger::info("\t\t\t{} form swaps found", values.size());
							for (const auto& key : values) {
								SwapFormData::GetForms(path, key.pItem, [&](const RE::FormID a_baseID, const SwapFormData& a_swapData) {
									swapFormsConditional[a_baseID][processedConditions].emplace_back(a_swapData);
								});
							}
						} else {
							logger::info("\t\t\t{} ref property overrides found", values.size());
							for (const auto& key : values) {
								ObjectData::GetProperties(path, key.pItem, [&](const RE::FormID a_baseID, const ObjectData& a_objectData) {
									refPropertiesConditional[a_baseID][processedConditions].emplace_back(a_objectData);
								});
							}
						}
					}
				} else {
					logger::info("\treading [{}]", section);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section.c_str(), values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (section == "Transforms" || section == "Properties") {
							logger::info("\t\t\t{} ref property overrides found", values.size());
							for (const auto& key : values) {
								ObjectData::GetProperties(path, key.pItem, [&](RE::FormID a_baseID, const ObjectData& a_objectData) {
									refProperties[a_baseID].push_back(a_objectData);
								});
							}
						} else {
							logger::info("\t\t\t{} swaps found", values.size());
							auto& map = (section == "Forms") ? swapForms : swapRefs;
							for (const auto& key : values) {
								SwapFormData::GetForms(path, key.pItem, [&](RE::FormID a_baseID, const SwapFormData& a_swapData) {
									map[a_baseID].push_back(a_swapData);
								});
							}
						}
					}
				}
			}
		}

		logger::info("{:*^30}", "RESULT");

		logger::info("{} form-form swaps", swapForms.size());
		logger::info("{} conditional form swaps", swapFormsConditional.size());
		logger::info("{} ref-form swaps", swapRefs.size());
		logger::info("{} ref property overrides", refProperties.size());
		logger::info("{} conditional ref property overrides", refPropertiesConditional.size());

		logger::info("{:*^30}", "CONFLICTS");

		const auto log_conflicts = [&]<typename T>(std::string_view a_type, const FormIDMap<T>& a_map) {
			if (a_map.empty()) {
				return;
			}
			logger::info("[{}]", a_type);
			bool conflicts = false;
			for (auto& [baseID, swapDataVec] : a_map) {
				if (swapDataVec.size() > 1) {
					const auto& winningRecord = swapDataVec.back();
					if (winningRecord.chance.chanceValue != 100) {  //ignore if winning record is randomized
						continue;
					}
					conflicts = true;
					auto winningForm = string::split(winningRecord.record, "|");
					logger::warn("\t{}", winningForm[0]);
					logger::warn("\t\twinning swap : {} ({})", winningForm[1], swapDataVec.back().path);
					logger::warn("\t\t{} conflicts", swapDataVec.size() - 1);
					for (auto it = swapDataVec.rbegin() + 1; it != swapDataVec.rend(); ++it) {
						auto losingRecord = it->record.substr(it->record.find('|') + 1);
						logger::warn("\t\t\t{} ({})", losingRecord, it->path);
					}
				}
			}
			if (!conflicts) {
				logger::info("\tNo conflicts found");
			} else {
				hasConflicts = true;
			}
		};

		log_conflicts("Forms"sv, swapForms);
		log_conflicts("References"sv, swapRefs);
		log_conflicts("Properties"sv, refProperties);

		logger::info("{:*^30}", "END");
	}

	void Manager::PrintConflicts() const
	{
		if (const auto console = RE::ConsoleLog::GetSingleton(); hasConflicts) {
			console->Print("[BOS] Conflicts found, check po3_BaseObjectSwapper.log in %s for more info\n", logger::log_directory()->string().c_str());
		}
	}

	SwapFormResult Manager::GetSwapFormConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		if (const auto it = swapFormsConditional.find(a_base->GetFormID()); it != swapFormsConditional.end()) {
			const ConditionalInput input(a_ref, a_base);

			for (auto& [filters, swapDataVec] : it->second | std::ranges::views::reverse) {
				if (input.IsValid(filters)) {
					for (auto& swapData : swapDataVec | std::ranges::views::reverse) {
						if (auto swapObject = swapData.GetSwapBase(a_ref)) {
							return { swapObject, swapData.properties };
						}
					}
				}
			}
		}

		return { nullptr, std::nullopt };
	}

	std::optional<ObjectProperties> Manager::GetObjectPropertiesConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		if (const auto it = refPropertiesConditional.find(a_base->GetFormID()); it != refPropertiesConditional.end()) {
			const ConditionalInput input(a_ref, a_base);

			for (auto& [filters, objectDataVec] : it->second | std::ranges::views::reverse) {
				if (input.IsValid(filters)) {
					for (auto& objectData : objectDataVec | std::ranges::views::reverse) {
						if (objectData.HasValidProperties(a_ref)) {
							return objectData.properties;
						}
					}
				}
			}
		}

		return std::nullopt;
	}

	void Manager::InsertLeveledItemRef(const RE::TESObjectREFR* a_refr)
	{
		swappedLeveledItemRefs.insert(a_refr->GetFormID());
	}

	bool Manager::IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const
	{
		return swappedLeveledItemRefs.contains(a_refr->GetFormID());
	}

	SwapFormResult Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		SwapFormResult swapData{ nullptr, std::nullopt };

		// get base
		const auto get_swap_base = [a_ref](const RE::TESForm* a_form, const FormIDMap<SwapFormDataVec>& a_map) -> SwapFormResult {
			if (const auto it = a_map.find(a_form->GetFormID()); it != a_map.end()) {
				for (auto& swapData : it->second | std::ranges::views::reverse) {
					if (auto swapObject = swapData.GetSwapBase(a_ref)) {
						return { swapObject, swapData.properties };
					}
				}
			}
			return { nullptr, std::nullopt };
		};

		// references -> conditional forms -> forms
		if (!a_ref->IsDynamicForm()) {
			swapData = get_swap_base(a_ref, swapRefs);
		}
		if (!swapData.first) {
			swapData = GetSwapFormConditional(a_ref, a_base);
		}
		if (!swapData.first) {
			swapData = get_swap_base(a_base, swapForms);
		}

		// process leveled swaps. do not swap if leveled item has encounter zone
		if (const auto swapLvlBase = swapData.first ? swapData.first->As<RE::TESLevItem>() : nullptr) {
			if (a_ref->GetEncounterZone() == nullptr) {
				RE::BSScrapArray<RE::CALCED_OBJECT> calcedObjects{};
				swapLvlBase->CalculateCurrentFormList(a_ref->GetCalcLevel(false), 1, calcedObjects, 0, true);
				if (!calcedObjects.empty()) {
					swapData.first = static_cast<RE::TESBoundObject*>(calcedObjects.front().form);
				}
			} else {
				swapData.first = nullptr;
			}
		}

		// get object properties
		const auto get_properties = [&](const RE::TESForm* a_form) -> std::optional<ObjectProperties> {
			if (const auto it = refProperties.find(a_form->GetFormID()); it != refProperties.end()) {
				for (auto& objectData : it->second | std::ranges::views::reverse) {
					if (objectData.HasValidProperties(a_ref)) {
						return objectData.properties;
					}
				}
			}
			return std::nullopt;
		};

		constexpr auto has_properties = [](const std::optional<ObjectProperties>& a_result) {
			return a_result && a_result->IsValid();
		};

		// references -> conditional forms -> forms
		if (!has_properties(swapData.second) && !a_ref->IsDynamicForm()) {
			swapData.second = get_properties(a_ref);
		}
		if (!has_properties(swapData.second)) {
			swapData.second = GetObjectPropertiesConditional(a_ref, a_base);
		}
		if (!has_properties(swapData.second)) {
			swapData.second = get_properties(a_base);
		}

		return swapData;
	}
}
