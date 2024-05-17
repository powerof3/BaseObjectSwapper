#include "Hooks.h"

namespace BaseObjectSwapper
{
	void detail::swap_base(RE::TESObjectREFR* a_ref)
	{
		if (const auto base = a_ref->GetBaseObject()) {
			FormSwap::Manager::GetSingleton()->LoadFormsOnce();

			const auto& [swapBase, objectProperties] = FormSwap::Manager::GetSingleton()->GetSwapData(a_ref, base);

			if (swapBase && swapBase != base) {
				a_ref->SetObjectReference(swapBase);

				if (a_ref->extraList.HasType<RE::ExtraLeveledItemBase>()) {
					FormSwap::Manager::GetSingleton()->InsertLeveledItemRef(a_ref);
				}
			}

			if (objectProperties) {
				objectProperties->SetTransform(a_ref);
				objectProperties->SetRecordFlags(a_ref);
			}
		}
	}

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		InitItemImpl<RE::TESObjectREFR>::Install();
		InitItemImpl<RE::Hazard>::Install();
		InitItemImpl<RE::ArrowProjectile>::Install();

		SetObjectReference<RE::TESObjectREFR>::Install();
		SetObjectReference<RE::Hazard>::Install();
		SetObjectReference<RE::ArrowProjectile>::Install();
	}
}
