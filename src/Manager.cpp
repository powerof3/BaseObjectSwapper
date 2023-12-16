#include "Manager.h"

namespace FormSwap
{
	SwapMap<SwapDataVec>& Manager::get_form_map(const std::string& a_str)
	{
		return a_str == "Forms" ? swapForms : swapRefs;
	}

	void Manager::get_transforms(const std::string& a_path, const std::string& a_str)
	{
		return TransformData::GetTransforms(a_path, a_str, [&](RE::FormID a_baseID, const TransformData& a_swapData) {
			transforms[a_baseID].push_back(a_swapData);
		});
	}

	void Manager::get_transforms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs)
	{
		return TransformData::GetTransforms(a_path, a_str, [&](const RE::FormID a_baseID, const TransformData& a_swapData) {
			for (auto& id : a_conditionalIDs) {
				transformsConditional[a_baseID][id].push_back(a_swapData);
			}
		});
	}

	void Manager::get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map)
	{
		return SwapData::GetForms(a_path, a_str, [&](RE::FormID a_baseID, const SwapData& a_swapData) {
			a_map[a_baseID].push_back(a_swapData);
		});
	}

	void Manager::get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs)
	{
		return SwapData::GetForms(a_path, a_str, [&](const RE::FormID a_baseID, const SwapData& a_swapData) {
			for (auto& id : a_conditionalIDs) {
				swapFormsConditional[a_baseID][id].push_back(a_swapData);
			}
		});
	}

	bool ConditionalInput::IsValid(const FormIDStr& a_data) const
	{
		if (std::holds_alternative<RE::FormID>(a_data)) {
			if (const auto form = RE::TESForm::LookupByID(std::get<RE::FormID>(a_data))) {
				switch (form->GetFormType()) {
				case RE::FormType::Location:
					{
						const auto location = form->As<RE::BGSLocation>();
						return currentLocation && (currentLocation == location || currentLocation->IsParent(location));
					}
				case RE::FormType::Region:
					{
						if (const auto region = form->As<RE::TESRegion>()) {
							if (const auto regionList = currentCell ? currentCell->GetRegionList(false) : nullptr) {
								return std::ranges::any_of(*regionList, [&](const auto& regionInList) {
									return regionInList && regionInList == region;
								});
							}
						}
						return false;
					}
				case RE::FormType::Keyword:
					{
						const auto keyword = form->As<RE::BGSKeyword>();
						return currentLocation && currentLocation->HasKeyword(keyword) || ref->HasKeyword(keyword);
					}
				case RE::FormType::Cell:
					{
						return currentCell == form;
					}
				default:
					break;
				}
			}
		} else {
			const std::string editorID = std::get<std::string>(a_data);
			if (currentCell && string::iequals(currentCell->GetFormEditorID(), editorID)) {
				return true;
			}
			if (currentLocation && currentLocation->HasKeywordString(editorID)) {
				return true;
			}
			if (const auto keywordForm = base->As<RE::BGSKeywordForm>()) {
				return keywordForm->HasKeywordString(editorID);
			}
		}
		return false;
	}

	void Manager::LoadFormsOnce()
	{
		std::call_once(init, [this] {
			LoadForms();
		});
	}

	void Manager::LoadForms()
	{
		logger::info("{:*^30}", "INI");

		std::vector<std::string> configs = dist::get_configs(R"(Data\)", "_SWAP"sv);

		if (configs.empty()) {
			logger::warn("No .ini files with _SWAP suffix were found within the Data folder, aborting...");
			return;
		}

		logger::info("{} matching inis found", configs.size());

		for (auto& path : configs) {
			logger::info("\tINI : {}", path);

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

			constexpr auto push_filter = [](const std::string& a_condition, std::vector<FormIDStr>& a_processedFilters) {
				if (const auto processedID = SwapData::GetFormID(a_condition); processedID != 0) {
					a_processedFilters.emplace_back(processedID);
				} else {
					logger::error("\t\tFilter  [{}] INFO - unable to find form, treating filter as string", a_condition);
					a_processedFilters.emplace_back(a_condition);
				}
			};

			for (auto& [section, comment, keyOrder] : sections) {
				if (string::icontains(section, "|")) {
					auto splitSection = string::split(section, "|");
					auto conditions = string::split(splitSection[1], ",");  //[Forms|EditorID,EditorID2]

					logger::info("\t\treading [{}] : {} conditions", splitSection[0], conditions.size());

					std::vector<FormIDStr> processedConditions;
					processedConditions.reserve(conditions.size());
					for (auto& condition : conditions) {
						push_filter(condition, processedConditions);
					}

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section, values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (splitSection[0] == "Forms") {
							logger::info("\t\t\t{} form swaps found", values.size());
							for (const auto& key : values) {
								get_forms(path, key.pItem, processedConditions);
							}
						} else {
							logger::info("\t\t\t{} transform overrides found", values.size());
							for (const auto& key : values) {
								get_transforms(path, key.pItem, processedConditions);
							}
						}
					}
				} else {
					logger::info("\t\treading [{}]", section);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section, values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (string::iequals(section, "Transforms")) {
							logger::info("\t\t\t{} transform overrides found", values.size());
							for (const auto& key : values) {
								get_transforms(path, key.pItem);
							}
						} else {
							logger::info("\t\t\t{} swaps found", values.size());
							auto& map = get_form_map(section);
							for (const auto& key : values) {
								get_forms(path, key.pItem, map);
							}
						}
					}
				}
			}
		}

		logger::info("{:*^30}", "RESULT");

		logger::info("{} form-form swaps processed", swapForms.size());
		logger::info("{} conditional form swaps processed", swapFormsConditional.size());
		logger::info("{} ref-form swaps processed", swapRefs.size());
		logger::info("{} transform overrides processed", transforms.size());
		logger::info("{} conditional transform overrides processed", transformsConditional.size());

		logger::info("{:*^30}", "CONFLICTS");

		const auto log_conflicts = [&]<typename T>(std::string_view a_type, const SwapMap<T>& a_map) {
			if (a_map.empty()) {
				return;
			}
			logger::info("[{}]", a_type);
			bool conflicts = false;
			for (auto& [baseID, swapDataVec] : a_map) {
				if (swapDataVec.size() > 1) {
					const auto& winningRecord = swapDataVec.back();
					if (winningRecord.traits.chance != 100) {  //ignore if winning record is randomized
						continue;
					}
					conflicts = true;
					auto winningForm = string::split(winningRecord.record, "|");
					logger::warn("\t{}", winningForm[0]);
					logger::warn("\t\twinning record : {} ({})", winningForm[1], swapDataVec.back().path);
					logger::warn("\t\t{} conflicts", swapDataVec.size() - 1);
					for (auto it = swapDataVec.rbegin() + 1; it != swapDataVec.rend(); ++it) {
						auto losingRecord = it->record.substr(it->record.find('|') + 1);
						logger::warn("\t\t\t{} ({})", losingRecord, it->path);
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
		log_conflicts("Transforms"sv, transforms);

		logger::info("{:*^30}", "END");
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
			const ConditionalInput input(a_ref, a_base);
			const auto             result = std::ranges::find_if(it->second, [&](const auto& a_data) {
                return input.IsValid(a_data.first);
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

	TransformResult Manager::GetTransformConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		if (const auto it = transformsConditional.find(a_base->GetFormID()); it != transformsConditional.end()) {
			const ConditionalInput input(a_ref, a_base);
			const auto             result = std::ranges::find_if(it->second, [&](const auto& a_data) {
                return input.IsValid(a_data.first);
            });

			if (result != it->second.end()) {
				for (auto& transformData : result->second | std::ranges::views::reverse) {
					if (transformData.IsTransformValid(a_ref)) {
						return transformData.transform;
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

	SwapResult Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		const auto get_swap_base = [a_ref](const RE::TESForm* a_form, const SwapMap<SwapDataVec>& a_map) -> SwapResult {
			if (const auto it = a_map.find(a_form->GetFormID()); it != a_map.end()) {
				for (auto& swapData : it->second | std::ranges::views::reverse) {
					if (auto swapObject = swapData.GetSwapBase(a_ref)) {
						return { swapObject, swapData.transform };
					}
				}
			}
			return { nullptr, std::nullopt };
		};

		const auto get_transform = [&](const RE::TESForm* a_form) -> TransformResult {
			if (const auto it = transforms.find(a_form->GetFormID()); it != transforms.end()) {
				for (auto& transformData : it->second | std::ranges::views::reverse) {
					if (transformData.IsTransformValid(a_ref)) {
						return transformData.transform;
					}
				}
			}
			return std::nullopt;
		};

		constexpr auto has_transform = [](const TransformResult& a_result) {
			return a_result && a_result->IsValid();
		};

		SwapResult swapData{ nullptr, std::nullopt };

		// get base
		if (!a_ref->IsDynamicForm()) {
			swapData = get_swap_base(a_ref, swapRefs);
		}

		if (!swapData.first) {
			swapData = GetSwapConditionalBase(a_ref, a_base);
		}

		if (!swapData.first) {
			swapData = get_swap_base(a_base, swapForms);
		}

		if (const auto swapLvlBase = swapData.first ? swapData.first->As<RE::TESLevItem>() : nullptr) {
			RE::BSScrapArray<RE::CALCED_OBJECT> calcedObjects{};
			swapLvlBase->CalculateCurrentFormList(a_ref->GetCalcLevel(false), 1, calcedObjects, 0, true);
			if (!calcedObjects.empty()) {
				swapData.first = static_cast<RE::TESBoundObject*>(calcedObjects.front().form);
			}
		}

		// get transforms
		if (!has_transform(swapData.second) && !a_ref->IsDynamicForm()) {
			swapData.second = get_transform(a_ref);
		}

		if (!has_transform(swapData.second)) {
			swapData.second = GetTransformConditional(a_ref, a_base);
		}

		if (!has_transform(swapData.second)) {
			swapData.second = get_transform(a_base);
		}

		return swapData;
	}
}
