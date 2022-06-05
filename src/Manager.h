#pragma once

#include "SwapData.h"

namespace FormSwap
{
	class Manager
	{
	public:
		using SwapResult = std::pair<RE::TESBoundObject*, Transform>;

		[[nodiscard]] static Manager* GetSingleton()
		{
			static Manager singleton;
			return std::addressof(singleton);
		}

		bool LoadFormsOnce();

		void PrintConflicts() const;

		SwapResult GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		SwapResult GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

	protected:
		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		~Manager() = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

	private:
		template <class K, class D>
		using Map = robin_hood::unordered_flat_map<K, D>;

		template <class T>
		using SwapMap = Map<RE::FormID, T>;
		using SwapDataConditional = Map<std::variant<RE::FormID, std::string>, SwapData>;

		using ConflictMap = Map<RE::FormID, std::vector<std::pair<std::string, std::string>>>;  //record, path

		using Lock = std::mutex;
		using Locker = std::scoped_lock<Lock>;

		SwapMap<SwapData>& get_form_map(const std::string& a_str);
        ConflictMap& get_conflict_map(const std::string& a_str);

        static std::pair<bool, RE::FormID> get_forms_impl(const std::string& a_str, std::function<void(RE::FormID a_baseID, SwapData& a_swapData)> a_func);

		static std::pair<bool, RE::FormID> get_forms(const std::string& a_str, SwapMap<SwapData>& a_map);
		static std::pair<bool, RE::FormID> get_forms(const std::string& a_str, const std::vector<std::variant<RE::FormID, std::string>>& a_conditionalIDs, SwapMap<SwapDataConditional>& a_map);

		ConflictMap conflictForms{};
		ConflictMap conflictRefs{};
		ConflictMap conflictFormsConditional{};
		bool hasConflicts{ false };

		SwapMap<SwapData> swapForms{};
		SwapMap<SwapData> swapRefs{};
		SwapMap<SwapDataConditional> swapFormsConditional{};

		std::atomic_bool init{ false };
	};
}
