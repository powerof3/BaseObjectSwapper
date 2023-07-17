#pragma once

#include "SwapData.h"

namespace FormSwap
{
	template <class T>
	using SwapMap = Map<RE::FormID, T>;

	using SwapDataVec = std::vector<SwapData>;
	using TransformDataVec = std::vector<TransformData>;

	using SwapDataConditional = Map<FormIDStr, SwapDataVec>;
	using TransformDataConditional = Map<FormIDStr, TransformDataVec>;

	using ConditionalInput = std::tuple<const RE::TESObjectREFR*, const RE::TESForm*, RE::TESObjectCELL*, RE::BGSLocation*>;

	using TransformResult = std::optional<Transform>;
	using SwapResult = std::pair<RE::TESBoundObject*, TransformResult>;

	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadFormsOnce();

		void            PrintConflicts() const;
		SwapResult      GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		SwapResult      GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		TransformResult GetTransformConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

		void InsertLeveledItemRef(const RE::TESObjectREFR* a_refr);
		bool IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const;

	private:
		void LoadForms();

		SwapMap<SwapDataVec>& get_form_map(const std::string& a_str);

		static void get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map);
		void        get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);

		void get_transforms(const std::string& a_path, const std::string& a_str);
		void get_transforms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);

		[[nodiscard]] bool get_conditional_result(const FormIDStr& a_data, const ConditionalInput& a_input) const;

		SwapMap<SwapDataVec>         swapForms{};
		SwapMap<SwapDataVec>         swapRefs{};
		SwapMap<SwapDataConditional> swapFormsConditional{};

		SwapMap<TransformDataVec>         transforms{};
		SwapMap<TransformDataConditional> transformsConditional{};

		Set<RE::FormID> swappedLeveledItemRefs{};

		bool           hasConflicts{ false };
		std::once_flag init{};
	};
}
