#include "FormSwap.h"

namespace FormSwap
{
	std::pair<bool, RE::FormID> Manager::get_forms(const std::string& a_str, FormMap<FormData>& a_map)
	{
		const auto formPair = string::split(a_str, "|");

		auto baseFormID = FormData::get_formID(formPair[0]);
		auto swapFormID = FormData::get_formID(formPair[1]);

		const auto flags = formPair.size() > 2 ? FormData::get_flags(formPair[2]) : FormData::FLAGS::kNone;

		if (swapFormID != 0 && baseFormID != 0) {
			FormData data = { swapFormID, flags };

			a_map.insert_or_assign(baseFormID, data);

			return { true, baseFormID };
		}

		logger::error("			failed to process {} [{:x}|{:x}|{}] (formID not found)", a_str, baseFormID, swapFormID, std::to_underlying(flags));

		return { false, 0 };
	}

	std::pair<bool, RE::FormID> Manager::get_forms(const std::string& a_str, const std::vector<std::string>& a_conditionalIDs, FormMap<FormDataConditional>& a_map)
	{
		const auto formPair = string::split(a_str, "|");

		auto baseFormID = FormData::get_formID(formPair[0]);
		auto swapFormID = FormData::get_formID(formPair[1]);

		const auto flags = formPair.size() > 2 ? FormData::get_flags(formPair[2]) : FormData::FLAGS::kNone;

		if (swapFormID != 0 && baseFormID != 0) {
			const FormData data = { swapFormID, flags };

			for (auto& id : a_conditionalIDs) {
				if (const auto processedID = FormData::get_formID(id); processedID != 0) {
					a_map[baseFormID].insert_or_assign(processedID, data);
				}
			}

			return { true, baseFormID };
		}

		logger::error("			failed to process {} [{:x}|{:x}|{}] (formID not found)", a_str, baseFormID, swapFormID, std::to_underlying(flags));

		return { false, 0 };
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
		if (hasConflicts) {
			if (const auto console = RE::ConsoleLog::GetSingleton()) {
				console->Print("~BASE OBJECT SWAPPER~");
				console->Print("Conflicts detected, check po3_BaseObjectSwapper.log for more details");
			}
		}
	}

	Manager::SwapData Manager::GetSwapBase(const RE::TESForm* a_base)
	{
		if (const auto it = swapForms.find(a_base->GetFormID()); it != swapForms.end()) {
			return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
		}

		return { nullptr, FormData::FLAGS::kNone };
	}

	Manager::SwapData Manager::GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
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
				return { RE::TESForm::LookupByID<RE::TESBoundObject>(result->second.formID), result->second.flags };
			}
		}

		return { nullptr, FormData::FLAGS::kNone };
	}

	Manager::SwapData Manager::GetSwapRef(const RE::TESObjectREFR* a_ref)
	{
		if (const auto it = swapRefs.find(a_ref->GetFormID()); it != swapRefs.end()) {
			return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
		}

		return { nullptr, FormData::FLAGS::kNone };
	}

	Manager::SwapData Manager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
	{
		auto nullSwap = std::make_pair(nullptr, FormData::FLAGS::kNone);

		auto swapData = !a_ref->IsDynamicForm() ? GetSwapRef(a_ref) : nullSwap;
		if (!swapData.first) {
			swapData = !a_base->IsDynamicForm() ? GetSwapConditionalBase(a_ref, a_base) : nullSwap;
		}
		if (!swapData.first) {
			swapData = !a_base->IsDynamicForm() ? GetSwapBase(a_base) : nullSwap;
		}
		return swapData;
	}
}
