#pragma once

#include "MergeMapper.h"

namespace FormSwap
{
	struct FormData
	{
		enum class FLAGS
		{
			kNone = 0
		};

		[[nodiscard]] static RE::FormID get_formID(const std::string& a_str)
		{
			if (a_str.find('~') != std::string::npos) {
				const auto formPair = string::split(a_str, "~");

				const auto processedFormPair = MergeMapper::GetNewFormID(formPair[1], formPair[0]);

				return RE::TESDataHandler::GetSingleton()->LookupFormID(processedFormPair.second, processedFormPair.first);
			}
			if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
				return form->GetFormID();
			}
			return static_cast<RE::FormID>(0);
		}

		[[nodiscard]] static FLAGS get_flags(const std::string& a_str)
		{
			std::uint32_t flags = 0;
			const auto flagStr = string::split(a_str, ",");

			static std::map<std::string, FLAGS> flagsEnum{
				{ "none", FLAGS::kNone }
			};

			for (auto& flag : flagStr) {
				if (const auto it = flagsEnum.find(flag); it != flagsEnum.end()) {
					flags += stl::to_underlying(it->second);
				}
			}
			return static_cast<FLAGS>(flags);
		}

		RE::FormID formID{};
		stl::enumeration<FLAGS, std::uint32_t> flags{};
	};

	class Manager
	{
	public:
		using SwapData = std::pair<RE::TESBoundObject*, stl::enumeration<FormData::FLAGS, std::uint32_t>>;

		[[nodiscard]] static Manager* GetSingleton()
		{
			static Manager singleton;
			return std::addressof(singleton);
		}

		bool LoadFormsOnce();

		void PrintConflicts() const;

		SwapData GetSwapBase(const RE::TESForm* a_base);
		SwapData GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		SwapData GetSwapRef(const RE::TESObjectREFR* a_ref);
		SwapData GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

	protected:
		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		~Manager() = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

	private:
		template <class K, class D>
	    using Map = robin_hood::unordered_flat_map<K,D>;

	    template <class T>
		using FormMap = Map<RE::FormID, T>;
		using FormDataConditional = Map<RE::FormID, FormData>;

		using ConflictMap = Map<RE::FormID, std::vector<std::pair<std::string, std::string>>>;  //record, path

	    using Lock = std::mutex;
		using Locker = std::scoped_lock<Lock>;

		FormMap<FormData>& get_form_map(const std::string& a_str)
		{
			return a_str == "Forms" ? swapForms : swapRefs;
		}
		ConflictMap& get_conflict_map(const std::string& a_str)
		{
			return a_str == "Forms" ? conflictForms : conflictRefs;
		}

	    static std::pair<bool, RE::FormID> get_forms(const std::string& a_str, FormMap<FormData>& a_map);
		static std::pair<bool, RE::FormID> get_forms(const std::string& a_str, const std::vector<std::string>& a_conditionalIDs, FormMap<FormDataConditional>& a_map);

		static void map_conflicts(const std::string& a_str, const std::string& a_path, RE::FormID a_baseID, ConflictMap& a_conflictMap)
		{
			a_conflictMap[a_baseID].emplace_back(std::make_pair(a_str, a_path));
		}

		ConflictMap conflictForms{};
		ConflictMap conflictRefs{};
		bool hasConflicts{ false };

		FormMap<FormData> swapForms{};
		FormMap<FormDataConditional> swapFormsConditional{};
		FormMap<FormData> swapRefs{};

		std::atomic_bool init{ false };
	};
}
