#include "Hooks.h"
#include "Manager.h"

namespace BaseObjectSwapper
{
	namespace TESObjectREFR
	{
		struct InitItemImpl
		{
			static void thunk(RE::TESObjectREFR* a_ref)
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

				func(a_ref);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t size = 0x13;
		};

		void Install()
		{
			stl::write_vfunc<RE::TESObjectREFR, InitItemImpl>();
			logger::info("Installed reference form swap"sv);
		}
	}

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		TESObjectREFR::Install();
	}
}
