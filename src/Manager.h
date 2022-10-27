#pragma once

#include "SwapData.h"

namespace FormSwap
{
	template <class T>
	using SwapMap = Map<RE::FormID, T>;

	using SwapDataVec = std::vector<SwapData>;
	using SwapDataConditional = Map<FormIDStr, SwapDataVec>;
	using SwapResult = std::pair<RE::TESBoundObject*, Transform>;

	class Manager
	{
	public:
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
		SwapMap<SwapDataVec>& get_form_map(const std::string& a_str);

		static void get_forms_impl(const std::string& a_path,const std::string& a_str, std::function<void(RE::FormID, SwapData&)> a_func);

		static void get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map);
		static void get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs, SwapMap<SwapDataConditional>& a_map);

		SwapMap<SwapDataVec> swapForms{};
		SwapMap<SwapDataVec> swapRefs{};
		SwapMap<SwapDataConditional> swapFormsConditional{};

		bool hasConflicts{ false };
		std::atomic_bool init{ false };
	};
}
