#include "FormSwapManager.h"

bool FormSwapManager::LoadFormsOnce()
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

	const auto get_forms = [&](const std::string& a_str, const std::string& a_path, FormMap& a_map, ConflictMap& a_conflictMap) {
		const auto formPair = string::split(a_str, "|");

		auto baseFormID = get_formID(formPair[0]);
		auto swapFormID = get_formID(formPair[1]);

		auto flags = formPair.size() > 2 ? get_flags(formPair[2]) : SWAP_FLAGS::kNone;

		if (swapFormID != 0 && baseFormID != 0) {
			FormData data = { swapFormID, flags };

			a_map.insert_or_assign(baseFormID, data);
			a_conflictMap[baseFormID].emplace_back(std::make_pair(a_str, a_path));
		} else {
			logger::error("failed to process [{}|{}|{}] (formID not found)", baseFormID, swapFormID, static_cast<int>(flags));
		}
	};

	for (auto& path : configs) {
		logger::info("	INI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();
		ini.SetAllowEmptyValues();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
			continue;
		}

		std::vector<std::string> types{ "Forms", "References" };

		for (auto& type : types) {
			if (const auto values = ini.GetSection(type.c_str()); values && !values->empty()) {
				logger::info("		reading [{}]", type);
				logger::info("		{} values found", values->size());
				auto& map = type == types[0] ? swapForms : swapRefs;
				auto& conflictMap = type == types[0] ? conflictForms : conflictRefs;
				for (auto& [key, entry] : *values) {
					get_forms(key.pItem, path, map, conflictMap);
				}
			} else {
				logger::info("		[{}] not found", type);
			}
		}
	}

	logger::info("{:*^30}", "RESULT");

	logger::info("{} form-form swaps done", swapForms.size());
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

	return !swapForms.empty() || !swapRefs.empty();
}

void FormSwapManager::PrintConflicts() const
{
	if (hasConflicts) {
        if (const auto console = RE::ConsoleLog::GetSingleton()) {
			console->Print("~BASE OBJECT SWAPPER~");
			console->Print("Conflicts detected, check po3_BaseObjectSwapper.log for more details");
		}
	}
}

FormSwapManager::SwapData FormSwapManager::GetSwapBase(const RE::TESForm* a_base)
{
	if (const auto it = swapForms.find(a_base->GetFormID()); it != swapForms.end()) {
		return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
	}
	return { nullptr, SWAP_FLAGS::kNone };
}

FormSwapManager::SwapData FormSwapManager::GetSwapRef(const RE::TESObjectREFR* a_ref)
{
	if (const auto it = swapRefs.find(a_ref->GetFormID()); it != swapRefs.end()) {
		return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
	}
	return { nullptr, SWAP_FLAGS::kNone };
}

FormSwapManager::SwapData FormSwapManager::GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base)
{
	auto nullSwap = std::make_pair(nullptr, SWAP_FLAGS::kNone);

    auto swapData = !a_ref->IsDynamicForm() ? GetSwapRef(a_ref) : nullSwap;
	if (!swapData.first) {
		swapData = !a_base->IsDynamicForm() ? GetSwapBase(a_base) : nullSwap;
	}
	return swapData;
}

void FormSwapManager::SetOriginalBase(const RE::TESObjectREFR* a_ref, const FormData& a_originalBaseData)
{
	Locker locker(origBaseLock);

	origBases.emplace(a_ref->GetFormID(), a_originalBaseData);
}

FormSwapManager::SwapData FormSwapManager::GetOriginalBase(const RE::TESObjectREFR* a_ref)
{
	Locker locker(origBaseLock);

	if (const auto it = origBases.find(a_ref->GetFormID()); it != origBases.end()) {
		return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
	}
	return { nullptr, SWAP_FLAGS::kNone };
}
