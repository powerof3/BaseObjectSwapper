#pragma once

#include "SwapData.h"

namespace FormSwap
{
	struct ConditionalInput
	{
		ConditionalInput(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_form) :
			ref(a_ref),
			base(a_form),
			currentCell(a_ref->GetSaveParentCell()),
			currentLocation(a_ref->GetCurrentLocation())
		{}

		[[nodiscard]] bool IsValid(const FormIDStr& a_data) const;

		// members
		const RE::TESObjectREFR* ref;
		const RE::TESForm*       base;
		RE::TESObjectCELL*       currentCell;
		RE::BGSLocation*         currentLocation;
	};

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
		static void           get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map);
		void                  get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);
		void                  get_transforms(const std::string& a_path, const std::string& a_str);
		void                  get_transforms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);

		// members
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
