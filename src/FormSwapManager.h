#pragma once

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
	using SwapData = std::pair<RE::TESBoundObject*, stl::enumeration<SWAP_FLAGS, std::uint32_t>>;

	[[nodiscard]] static FormSwapManager* GetSingleton()
	{
		static FormSwapManager singleton;
		return std::addressof(singleton);
	}

	bool LoadFormsOnce();

	void PrintConflicts() const;

	SwapData GetSwapBase(const RE::TESForm* a_base);
	SwapData GetSwapRef(const RE::TESObjectREFR* a_ref);
	SwapData GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

	void SetOriginalBase(const RE::TESObjectREFR* a_ref, const FormData& a_originalBaseData);
	SwapData GetOriginalBase(const RE::TESObjectREFR* a_ref);

protected:
	FormSwapManager() = default;
	FormSwapManager(const FormSwapManager&) = delete;
	FormSwapManager(FormSwapManager&&) = delete;
	~FormSwapManager() = default;

	FormSwapManager& operator=(const FormSwapManager&) = delete;
	FormSwapManager& operator=(FormSwapManager&&) = delete;

private:
	using FormMap = robin_hood::unordered_flat_map<RE::FormID, FormData>;
	using ConflictMap = robin_hood::unordered_flat_map<RE::FormID, std::vector<std::pair<std::string, std::string>>>;  //record, path

	using Lock = std::mutex;
	using Locker = std::scoped_lock<Lock>;

	ConflictMap conflictForms{};
	ConflictMap conflictRefs{};
	bool hasConflicts{ false };

	FormMap swapForms{};
	FormMap swapRefs{};

	mutable Lock origBaseLock;
	FormMap origBases{};

	std::atomic_bool init{ false };

	[[nodiscard]] RE::FormID get_formID(const std::string& a_str) const
	{
		if (a_str.find('~') != std::string::npos) {
			const auto formPair = string::split(a_str, "~");

			const auto processedFormPair = std::make_pair(
				string::lexical_cast<RE::FormID>(formPair[0], true), formPair[1]);

			return RE::TESDataHandler::GetSingleton()->LookupFormID(processedFormPair.first, processedFormPair.second);
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}

	[[nodiscard]] SWAP_FLAGS get_flags(const std::string& a_str)
	{
		std::uint32_t flags = 0;
		const auto flagStr = string::split(a_str, ",");

		static std::map<std::string, SWAP_FLAGS> flagsEnum{
			{ "none", SWAP_FLAGS::kNone },
			{ "apply_material_shader", SWAP_FLAGS::kApplyMaterialShader }
		};

		for (auto& flag : flagStr) {
			if (const auto it = flagsEnum.find(flag); it != flagsEnum.end()) {
				flags += stl::to_underlying(it->second);
			}
		}
		return static_cast<SWAP_FLAGS>(flags);
	}
};
