#include "Hooks.h"

namespace BaseObjectSwapper
{
	// add persistent ref to correct worldspace list after swap or xy change
	class PersistentRefHelper
	{
	public:
		explicit PersistentRefHelper(RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_base, const RE::TESBoundObject* a_swapBase, const ObjectProperties* a_properties) :
			ref(a_ref),
			worldspace(nullptr)
		{
			if (ShouldReindexRef(a_base, a_swapBase, a_properties)) {
				worldspace = RE::GetPersistentRefWorldSpace(a_ref);
				if (worldspace) {
					worldspace->RemovePersistentRef(ref);
				}
			}
		}

		~PersistentRefHelper()
		{
			if (worldspace) {
				worldspace->AddPersistentRef(ref);
			}
		}

	private:
		static bool ShouldReindexRef(const RE::TESBoundObject* a_base, const RE::TESBoundObject* a_swapBase, const ObjectProperties* a_properties)
		{
			if (a_swapBase && RE::IsFixedPersistentForm(a_swapBase) != RE::IsFixedPersistentForm(a_base)) {
				return true;
			}
			return a_properties && (a_properties->ChangesXYLocation() || a_properties->ChangesRecordFlag(RE::TESForm::RecordFlags::kTemporary));
		}
		
		RE::TESObjectREFR* ref;
		RE::TESWorldSpace* worldspace;
	};

	void detail::swap_base(RE::TESObjectREFR* a_ref)
	{
		const auto base = a_ref->GetBaseObject();
		if (!base) {
			return;
		}

		FormSwap::Manager::GetSingleton()->LoadFormsOnce();

		const auto& [swapBase, objectProperties] = FormSwap::Manager::GetSingleton()->GetSwapData(a_ref, base);

		const bool shouldSwap = swapBase && swapBase != base && RE::GetParentCell(a_ref);
		if (!shouldSwap && !objectProperties) {
			return;
		}

		PersistentRefHelper persistentRefHelper(a_ref, base, shouldSwap ? swapBase : nullptr, objectProperties);

		if (shouldSwap) {
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

	void Install()
	{
		REX::INFO("{:*^30}", "HOOKS");

		InitItemImpl<RE::TESObjectREFR>::Install();
		InitItemImpl<RE::Hazard>::Install();
		InitItemImpl<RE::ArrowProjectile>::Install();

		SetObjectReference<RE::TESObjectREFR>::Install();
		SetObjectReference<RE::Hazard>::Install();
		SetObjectReference<RE::ArrowProjectile>::Install();
	}
}
