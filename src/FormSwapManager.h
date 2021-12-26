#pragma once

#include "Cache.h"

enum class SWAP_FLAGS
{
	kNone = 0,
	kApplyMaterialShader
};

struct FormData
{
	RE::FormID formID;
	stl::enumeration<SWAP_FLAGS, std::uint32_t> flags;
};

class FormSwapManager
{
public:
	[[nodiscard]] static FormSwapManager* GetSingleton()
	{
		static FormSwapManager singleton;
		return std::addressof(singleton);
	}

	void LoadINI()
	{
		std::vector<std::string> configs;

		auto constexpr folder = R"(Data\\)";
		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
				if (const auto path = entry.path().string(); path.rfind("_SWAP") != std::string::npos) {
					configs.push_back(path);
				}
			}
		}

		if (configs.empty()) {
			logger::warn("	No .ini files with _SWAP suffix were found within the Data folder, aborting...");
			return;
		}

		logger::info("	{} matching inis found", configs.size());

		const auto get_raw_forms = [](const std::string& a_str, robin_hood::unordered_flat_map<std::string, std::pair<std::string, std::string>>& a_map) {
			const auto formPair = string::split(a_str, "|");
			const auto it = a_map.emplace(
				formPair[0],
				std::make_pair(formPair[1], formPair.size() > 2 ? formPair[2] : "none"));
			if (!it.second) {
				logger::error("			SKIP : cannot replace base [{}] with [{}] because the form pair [{}|{}] already exists", formPair[0], formPair[1], formPair[0], it.first->second.first);
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

			logger::info("		reading Forms");
			if (const auto values = ini.GetSection("Forms"); values && !values->empty()) {
				for (auto& [key, entry] : *values) {
					get_raw_forms(key.pItem, rawSwapForms);
				}
			}
			logger::info("		reading References");
			if (const auto values = ini.GetSection("References"); values && !values->empty()) {
				for (auto& [key, entry] : *values) {
					get_raw_forms(key.pItem, rawSwapRefs);
				}
			}
		}
	}

	bool LoadForms()
	{
		logger::info("{:*^30}", "PROCESSING");

		constexpr auto get_form = [](const std::string& a_str) {
			if (a_str.find('~') != std::string::npos) {
				const auto formPair = string::split(a_str, "~");

				const auto processedFormPair = std::make_pair(
					string::lexical_cast<RE::FormID>(formPair[0], true), formPair[1]);

				return RE::TESDataHandler::GetSingleton()->LookupFormID(processedFormPair.first, processedFormPair.second);
			} else {
				if (const auto form = RE::TESForm::LookupByID(Cache::EditorID::GetSingleton()->GetFormID(a_str)); form) {
					return form->GetFormID();
				}
			}
			return static_cast<RE::FormID>(0);
		};

		constexpr auto get_flags = [](const std::string& a_str) {
			std::uint32_t flags = 0;
		    const auto flagStr = string::split(a_str, ",");
			for (auto& flag : flagStr) {
                if (const auto it = flagsEnum.find(flag); it != flagsEnum.end()) {
					flags += stl::to_underlying(it->second);
				}
			}
			return static_cast<SWAP_FLAGS>(flags);
		};

		constexpr auto load_form = [](const std::string& a_base, const std::string& a_replacement, const std::string& a_flags, robin_hood::unordered_flat_map<RE::FormID, FormData>& a_map) {
			auto baseFormID = get_form(a_base);
			auto replaceFormID = get_form(a_replacement);
            auto flags = get_flags(a_flags);

			if (replaceFormID != 0 && baseFormID != 0) {
				FormData data = { replaceFormID, flags };
				a_map.emplace(baseFormID, data);
			}
		};

		for (auto& [base, replacement] : rawSwapForms) {
			auto& [replace, flags] = replacement;
			load_form(base, replace, flags, swapForms);
		}
		for (auto& [base, replacement] : rawSwapRefs) {
			auto& [replace, flags] = replacement;
			load_form(base, replace, flags, swapRefs);
		}

		logger::info("{:*^30}", "RESULT");

		logger::info("{}/{} form-form swaps done", rawSwapForms.size(), swapForms.size());
		logger::info("{}/{} ref-form swaps done", rawSwapRefs.size(), swapRefs.size());

		return !swapForms.empty() || !swapRefs.empty();
	}

	std::pair<RE::TESBoundObject*, stl::enumeration<SWAP_FLAGS, std::uint32_t>> GetSwapForm(const RE::TESForm* a_form)
	{
        if (const auto it = swapForms.find(a_form->GetFormID()); it != swapForms.end()) {
			return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
		}
		return { nullptr, SWAP_FLAGS::kNone };
	}

	std::pair<RE::TESBoundObject*, stl::enumeration<SWAP_FLAGS, std::uint32_t>> GetSwapRef(const RE::TESObjectREFR* a_ref)
	{
		if (const auto it = swapForms.find(a_ref->GetFormID()); it != swapForms.end()) {
			return { RE::TESForm::LookupByID<RE::TESBoundObject>(it->second.formID), it->second.flags };
		}
		return { nullptr, SWAP_FLAGS::kNone };
	}

protected:
	FormSwapManager() = default;
	FormSwapManager(const FormSwapManager&) = delete;
	FormSwapManager(FormSwapManager&&) = delete;
	~FormSwapManager() = default;

	FormSwapManager& operator=(const FormSwapManager&) = delete;
	FormSwapManager& operator=(FormSwapManager&&) = delete;

private:
    robin_hood::unordered_flat_map<std::string, std::pair<std::string, std::string>> rawSwapForms;
	robin_hood::unordered_flat_map<std::string, std::pair<std::string, std::string>> rawSwapRefs;

	robin_hood::unordered_flat_map<RE::FormID, FormData> swapForms;
	robin_hood::unordered_flat_map<RE::FormID, FormData> swapRefs;

	inline static std::map<std::string, SWAP_FLAGS> flagsEnum = {
		{ "none", SWAP_FLAGS::kNone },
		{ "apply_material_shader", SWAP_FLAGS::kApplyMaterialShader }
	};
};
