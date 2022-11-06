#include "Hooks.h"
#include "Manager.h"

namespace BaseObjectSwapper
{
	void detail::swap_base(RE::TESObjectREFR* a_ref)
	{
		if (const auto base = a_ref->GetBaseObject(); base) {
			FormSwap::Manager::GetSingleton()->LoadFormsOnce();

			const auto& [swapBase, transformData] = FormSwap::Manager::GetSingleton()->GetSwapData(a_ref, base);
			if (swapBase && swapBase != base) {
				a_ref->SetObjectReference(swapBase);
			}
			if (transformData) {
				transformData->SetTransform(a_ref);
			}
		}
	}

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		InitItemImpl<RE::TESObjectREFR>::Install();
		InitItemImpl<RE::Hazard>::Install();
		InitItemImpl<RE::ArrowProjectile>::Install();
	}
}
