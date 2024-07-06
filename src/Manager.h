#pragma once

#include "SwapData.h"

namespace FormSwap
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadFormsOnce();

		void PrintConflicts() const;

		SwapFormResult                  GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		SwapFormResult                  GetSwapFormConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		std::optional<ObjectProperties> GetObjectPropertiesConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

		void InsertLeveledItemRef(const RE::TESObjectREFR* a_refr);
		bool IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const;

	private:
		void LoadForms();

		// members
		FormIDMap<SwapFormDataVec>         swapRefs{};
		FormIDMap<SwapFormDataConditional> swapFormsConditional{};
		FormIDMap<SwapFormDataVec>         swapForms{};

		FormIDMap<ObjectDataVec>         refProperties{};
		FormIDMap<ObjectDataConditional> refPropertiesConditional{};

		Set<RE::FormID> swappedLeveledItemRefs{};

		bool           hasConflicts{ false };
		std::once_flag init{};
	};
}
