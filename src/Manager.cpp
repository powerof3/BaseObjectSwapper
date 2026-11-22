#include "Manager.h"

namespace FormSwap
{
	struct detail
	{
		template <class T>
		static void sort_entries(std::vector<T>& a_vec)
		{
			std::ranges::stable_sort(a_vec, std::ranges::greater{}, &ObjectData::fileIndex);
			a_vec.shrink_to_fit();
		}

		template <class T>
		static void sort_entries(ConditionalData<T>& a_map)
		{
			a_map.sort([](const auto& a_lhs, const auto& a_rhs) {
				return a_lhs.first->fileIndex > a_rhs.first->fileIndex;
			});
			a_map.shrink_to_fit();
		}


	};

	void FormEntries::Sort()
	{
		detail::sort_entries(conditionalSwaps);
		detail::sort_entries(formSwaps);
		detail::sort_entries(properties);
		detail::sort_entries(conditionalProperties);
	}

	void RefEntries::Sort()
	{
		detail::sort_entries(refSwaps);
		detail::sort_entries(properties);
	}

	void Manager::Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(this);
	}

	void Manager::LoadFormsOnce()
	{
		std::call_once(init, [this] {
			REX::FTimer timer;
			timer.Start();

			LoadForms();

			timer.Stop();
			REX::INFO("Parsing form swaps took {}", timer.GetDurationStringMin());
		});
	}

	void Manager::LoadForms()
	{
		REX::INFO("{:*^30}", "INI");

		std::vector<std::string> configs = distribution::get_configs(R"(Data\)", "_SWAP"sv);

		if (configs.empty()) {
			REX::WARN("No .ini files with _SWAP suffix were found within the Data folder, aborting...");
			return;
		}

		REX::INFO("{} matching inis found...", configs.size());

		std::uint32_t            fileIndex = 0;
		std::vector<std::string> records{}; // store non conditional swaps for conflict logging

		for (auto& path : configs) {
			fileIndex++;

			REX::INFO("INI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();
			ini.SetAllowKeyOnly();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				REX::ERROR("\tcouldn't read INI");
				continue;
			}

			CSimpleIniA::TNamesDepend sections;
			ini.GetAllSections(sections);
			sections.sort(CSimpleIniA::Entry::LoadOrder());

			for (auto& [_section, comment, keyOrder] : sections) {
				std::string section = _section;
				if (section.contains('|')) {
					auto splitSection = REX::STR::SPLIT(section, "|");
					auto conditions = REX::STR::SPLIT(splitSection[1], ",");  //[Forms|EditorID,EditorID2]

					REX::INFO("\treading [{}] : {} conditions", splitSection[0], conditions.size());

					const auto processedConditions = std::make_shared<const ConditionFilters>(conditions, fileIndex);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section.c_str(), values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (splitSection[0] == "Forms") {
							REX::INFO("\t\t\t{} form swaps found", values.size());
							for (const auto& key : values) {
								SwapFormData::GetForms(key.pItem, indexedProperties, [&](const RE::FormID a_baseID, SwapFormData&& a_swapData) {
									a_swapData.fileIndex = fileIndex;
									baseEntryMap[a_baseID].conditionalSwaps[processedConditions].push_back(std::move(a_swapData));
								});
							}
						} else {
							REX::INFO("\t\t\t{} ref property overrides found", values.size());
							for (const auto& key : values) {
								ObjectData::GetProperties(key.pItem, indexedProperties, false, [&](const RE::FormID a_baseID, const RE::TESForm*, ObjectData&& a_objectData) {
									a_objectData.fileIndex = fileIndex;
									baseEntryMap[a_baseID].conditionalProperties[processedConditions].push_back(std::move(a_objectData));
								});
							}
						}
					}
				} else {
					REX::INFO("\treading [{}]", section);

					CSimpleIniA::TNamesDepend values;
					ini.GetAllKeys(section.c_str(), values);
					values.sort(CSimpleIniA::Entry::LoadOrder());

					if (!values.empty()) {
						if (section == "Transforms" || section == "Properties") {
							REX::INFO("\t\t\t{} ref property overrides found", values.size());
							for (const auto& key : values) {
								const auto  recordIndex = static_cast<std::uint32_t>(records.size());
								const auto& record = records.emplace_back(key.pItem);
								ObjectData::GetProperties(record, indexedProperties, true, [&](RE::FormID a_baseID, const RE::TESForm* a_form, ObjectData&& a_objectData) {
									a_objectData.fileIndex = fileIndex;
									a_objectData.recordIndex = recordIndex;

									if (RE::IsBaseObject(a_form)) {
										baseEntryMap[a_baseID].properties.push_back(std::move(a_objectData));
									} else {
										refEntryMap[a_baseID].properties.push_back(std::move(a_objectData));
									}
								});
							}
						} else {
							REX::INFO("\t\t\t{} swaps found", values.size());
							bool useFormVec = (section == "Forms");
							for (const auto& key : values) {
								const auto  recordIndex = static_cast<std::uint32_t>(records.size());
								const auto& record = records.emplace_back(key.pItem);
								SwapFormData::GetForms(record, indexedProperties, [&](RE::FormID a_baseID, SwapFormData&& a_swapData) {
									a_swapData.fileIndex = fileIndex;
									a_swapData.recordIndex = recordIndex;

									if (useFormVec) {
										baseEntryMap[a_baseID].formSwaps.push_back(std::move(a_swapData));
									} else {
										refEntryMap[a_baseID].refSwaps.push_back(std::move(a_swapData));
									}
								});
							}
						}
					}
				}
			}
		}

		for (auto& formEntries : baseEntryMap | std::views::values) {
			formEntries.Sort();
		}
		for (auto& refEntries : refEntryMap | std::views::values) {
			refEntries.Sort();
		}

		REX::INFO("{:*^30}", "RESULT");
		LogFormSwaps();

		REX::INFO("{:*^30}", "CONFLICTS");
		LogConflicts(configs, records);

		REX::INFO("{:*^30}", "END");

		indexedProperties.ReleaseIndex();
	}

	void Manager::LogFormSwaps() const
	{
		const auto count_forms = [](const auto& a_map, auto a_member) {
			return std::ranges::count_if(a_map | std::views::values, [&](const auto& a_entries) {
				return !(a_entries.*a_member).empty();
			});
		};

		REX::INFO("{} form-form swaps", count_forms(baseEntryMap, &FormEntries::formSwaps));
		REX::INFO("{} conditional form swaps", count_forms(baseEntryMap, &FormEntries::conditionalSwaps));
		REX::INFO("{} ref-form swaps", count_forms(refEntryMap, &RefEntries::refSwaps));
		REX::INFO("{} ref property overrides", count_forms(refEntryMap, &RefEntries::properties) + count_forms(baseEntryMap, &FormEntries::properties));
		REX::INFO("{} conditional ref property overrides", count_forms(baseEntryMap, &FormEntries::conditionalProperties));
	}

	void Manager::LogConflicts(const std::vector<std::string>& a_configs, const std::vector<std::string>& a_records)
	{
		struct State
		{
			bool hasEntries{ false };
			bool conflicts{ false };
		};

		const auto log_map = [&](std::string_view a_type, State& a_state, const auto& a_map, auto a_member) {
			for (const auto& entries : a_map | std::views::values) {
				const auto& dataVec = entries.*a_member;
				if (dataVec.empty()) {
					continue;
				}
				if (!a_state.hasEntries) {
					REX::INFO("[{}]", a_type);
					a_state.hasEntries = true;
				}
				if (dataVec.size() > 1) {
					const auto& winningRecord = dataVec.front();
					if (winningRecord.chance != 100) {  //ignore if winning record is randomized
						continue;
					}
					a_state.conflicts = true;
					const auto winningForm = REX::STR::SPLIT(a_records[winningRecord.recordIndex], "|");
					REX::WARN("\t{}", winningForm[0]);
					REX::WARN("\t\twinning swap : {} ({})", winningForm[1], a_configs[winningRecord.fileIndex - 1]);
					REX::WARN("\t\t{} conflicts", dataVec.size() - 1);
					for (auto it = dataVec.begin() + 1; it != dataVec.end(); ++it) {
						const auto& losingLine = a_records[it->recordIndex];
						REX::WARN("\t\t\t{} ({})", losingLine.substr(losingLine.find('|') + 1), a_configs[it->fileIndex - 1]);
					}
				}
			}
		};

		const auto finish = [&](const State& a_state) {
			if (!a_state.hasEntries) {
				return;
			}
			if (!a_state.conflicts) {
				REX::INFO("\tNo conflicts found");
			} else {
				hasConflicts = true;
			}
		};

		State forms;
		log_map("Forms"sv, forms, baseEntryMap, &FormEntries::formSwaps);
		finish(forms);

		State refs;
		log_map("References"sv, refs, refEntryMap, &RefEntries::refSwaps);
		finish(refs);

		State props;
		log_map("Properties"sv, props, refEntryMap, &RefEntries::properties);
		log_map("Properties"sv, props, baseEntryMap, &FormEntries::properties);
		finish(props);
	}

	void Manager::PrintConflicts() const
	{
		if (const auto console = RE::ConsoleLog::GetSingleton(); hasConflicts) {
			console->Print("[BOS] Conflicts found, check po3_BaseObjectSwapper.log in %s for more info\n", SKSE::log::log_directory()->string().c_str());
		}
	}

	const FormEntries* Manager::GetBaseEntries(RE::FormID a_formID) const
	{
		const auto it = baseEntryMap.find(a_formID);
		return it != baseEntryMap.end() ? &it->second : nullptr;
	}

	const RefEntries* Manager::GetRefEntries(RE::FormID a_formID) const
	{
		const auto it = refEntryMap.find(a_formID);
		return it != refEntryMap.end() ? &it->second : nullptr;
	}

	SwapFormResult Manager::GetSwapFormConditional(const FormEntries& a_baseEntries, const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, std::optional<ConditionalInput>& a_input)
	{
		if (a_baseEntries.conditionalSwaps.empty()) {
			return {};
		}

		if (!a_input) {
			a_input.emplace(a_ref, a_base);
		}

		for (auto& [filters, swapDataVec] : a_baseEntries.conditionalSwaps) {
			if (a_input->IsValid(*filters)) {
				for (auto& swapData : swapDataVec) {
					if (auto swapObject = swapData.GetSwapBase(a_ref)) {
						return { swapObject, &swapData };
					}
				}
			}
		}

		return {};
	}

	const ObjectData* Manager::GetObjectPropertiesConditional(const FormEntries& a_baseEntries, const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, std::optional<ConditionalInput>& a_input)
	{
		if (a_baseEntries.conditionalProperties.empty()) {
			return nullptr;
		}

		if (!a_input) {
			a_input.emplace(a_ref, a_base);
		}

		for (auto& [filters, objectDataVec] : a_baseEntries.conditionalProperties) {
			if (a_input->IsValid(*filters)) {
				for (auto& objectData : objectDataVec) {
					if (objectData.HasValidProperties(a_ref)) {
						return &objectData;
					}
				}
			}
		}

		return nullptr;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
	{
		if (!a_event || !hasLeveledItemRefs.load(std::memory_order_relaxed) || a_event->formID == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		swappedLeveledItemRefs.erase(a_event->formID);

		return RE::BSEventNotifyControl::kContinue;
	}

	void Manager::InsertLeveledItemRef(const RE::TESObjectREFR* a_refr)
	{
		hasLeveledItemRefs.store(true, std::memory_order_relaxed);
		swappedLeveledItemRefs.insert(a_refr->GetFormID());
	}

	bool Manager::IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const
	{
		return hasLeveledItemRefs.load(std::memory_order_relaxed) && swappedLeveledItemRefs.contains(a_refr->GetFormID());
	}

	SwapFormResult Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		const RefEntries*  refEntries = a_ref->IsDynamicForm() ? nullptr : GetRefEntries(a_ref->GetFormID());
		const FormEntries* baseEntries = GetBaseEntries(a_base->GetFormID());

		if (!refEntries && !baseEntries) {
			return {};
		}

		SwapFormResult                  swapData{};
		std::optional<ConditionalInput> input;

		// get base
		const auto get_swap_base = [a_ref](const SwapFormDataVec& a_swaps) -> SwapFormResult {
			for (auto& swapData : a_swaps) {
				if (auto swapObject = swapData.GetSwapBase(a_ref)) {
					return { swapObject, &swapData };
				}
			}
			return {};
		};

		// references -> conditional forms -> forms
		if (refEntries) {
			swapData = get_swap_base(refEntries->refSwaps);
		}
		if (!swapData.swapBase && baseEntries) {
			swapData = GetSwapFormConditional(*baseEntries, a_ref, a_base, input);
			if (!swapData.swapBase) {
				swapData = get_swap_base(baseEntries->formSwaps);
			}
		}

		// process leveled swaps. do not swap if leveled item has encounter zone
		if (const auto swapLvlBase = swapData.swapBase ? swapData.swapBase->As<RE::TESLevItem>() : nullptr) {
			if (a_ref->GetEncounterZone() == nullptr) {
				RE::BSScrapArray<RE::CALCED_OBJECT> calcedObjects{};
				swapLvlBase->CalculateCurrentFormList(a_ref->GetCalcLevel(false), 1, calcedObjects, 0, true);
				if (!calcedObjects.empty()) {
					swapData.swapBase = static_cast<RE::TESBoundObject*>(calcedObjects.front().form);
				}
			} else {
				swapData.swapBase = nullptr;
			}
		}

		// get object properties
		const auto get_properties = [a_ref](const ObjectDataVec& a_vec) -> const ObjectData* {
			for (auto& objectData : a_vec) {
				if (objectData.HasValidProperties(a_ref)) {
					return &objectData;
				}
			}
			return nullptr;
		};
		constexpr auto has_properties = [](const ObjectData* a_src) {
			return a_src && a_src->properties;
		};

		if (!has_properties(swapData.propertySource) && refEntries) {
			swapData.propertySource = get_properties(refEntries->properties);
		}
		if (!has_properties(swapData.propertySource) && baseEntries) {
			swapData.propertySource = GetObjectPropertiesConditional(*baseEntries, a_ref, a_base, input);
			if (!has_properties(swapData.propertySource)) {
				swapData.propertySource = get_properties(baseEntries->properties);
			}
		}
		if (!has_properties(swapData.propertySource)) {
			swapData.propertySource = nullptr;
		}
		return swapData;
	}
}
