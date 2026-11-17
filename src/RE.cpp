#include "RE.h"

namespace RE
{
	std::pair<RE::FormID, RE::TESForm*> GetFormWithID(const std::string& a_str, bool a_resolveForm)
	{
		if (const auto splitID = REX::STR::SPLIT(a_str, "~"); splitID.size() == 2) {
			RE::FormID resolvedID;

			const auto  formID = REX::STR::TO_NUM<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			if (g_mergeMapperInterface) {
				const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(modName.c_str(), formID);
				resolvedID = RE::TESDataHandler::GetSingleton()->LookupFormID(mergedFormID, mergedModName);
			} else {
				resolvedID = RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
			}

			return { resolvedID, (a_resolveForm && resolvedID != 0) ? RE::TESForm::LookupByID(resolvedID) : nullptr };
		}
		if (REX::STR::IS_ONLY_HEX(a_str, true)) {
			const auto formID = REX::STR::TO_NUM<RE::FormID>(a_str, true);
			const auto form = RE::TESForm::LookupByID(formID);
			if (!form) {
				REX::ERROR("\t\tFilter [{}] INFO - unable to find form, treating filter as cell formID", a_str);
			}
			return { formID, form };
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str)) {
			return { form->GetFormID(), form };
		}
		return {};
	}

	RE::FormID GetFormID(const std::string& a_str)
	{
		return GetFormWithID(a_str, false).first;
	}

	FormIDOrSet GetSwapFormID(const std::string& a_str)
	{
		if (a_str.contains(",")) {
			FormIDSet  set;
			const auto IDStrs = REX::STR::SPLIT(a_str, ",");
			set.reserve(IDStrs.size());
			for (auto& IDStr : IDStrs) {
				if (auto formID = GetFormID(IDStr); formID != 0) {
					set.emplace_back(formID);
				} else {
					REX::ERROR("\t\t\tfailed to process {} (SWAP formID not found)", IDStr);
				}
			}
			std::ranges::sort(set);
			const auto dupes = std::ranges::unique(set);
			set.erase(dupes.begin(), dupes.end());
			return set;
		} else {
			return GetFormID(a_str);
		}
	}

	FormIDOrderedSet GetFormIDOrderedSet(const std::string& a_str)
	{
		FormIDOrderedSet set;
		if (a_str.contains(",")) {
			const auto IDStrs = REX::STR::SPLIT(a_str, ",");
			for (auto& IDStr : IDStrs) {
				if (auto formID = GetFormID(IDStr); formID != 0) {
					set.emplace(formID);
				} else {
					REX::ERROR("\t\t\tfailed to process {} (formID not found)", IDStr);
				}
			}
			return set;
		} else if (auto formID = GetFormID(a_str); formID != 0) {
			set.emplace(formID);
		}
		return set;
	}

	RE::TESForm* GetLocationOrCell(const RE::TESObjectREFR* a_ref)
	{
		if (const auto location = a_ref->GetEditorLocation()) {
			return location;
		}

		return a_ref->GetSaveParentCell();
	}

	RE::TESObjectCELL* GetParentCell(const RE::TESObjectREFR* a_ref)
	{
		/*auto parentCell = a_ref->GetParentCell();
		if (!parentCell) {
			parentCell = a_ref->GetSaveParentCell();
		}
		return parentCell;*/

		return a_ref->GetSaveParentCell();
	}

	bool IsFixedPersistentForm(const RE::TESBoundObject* a_base)
	{
		if (!a_base) {
			return false;
		}
		
		switch (a_base->GetFormType()) {
		case RE::FormType::Sound:
		case RE::FormType::Activator:
		case RE::FormType::Container:
		case RE::FormType::Door:
		case RE::FormType::Static:
		case RE::FormType::StaticCollection:
		case RE::FormType::Tree:
		case RE::FormType::Flora:
		case RE::FormType::Furniture:
		case RE::FormType::LeveledNPC:
			return true;
		default:
			return false;
		}
	}

	RE::TESWorldSpace* GetPersistentRefWorldSpace(const RE::TESObjectREFR* a_ref)
	{
		if (const auto xPersistentCell = a_ref->extraList.GetByType<RE::ExtraPersistentCell>()) {
			if (const auto persistentCell = xPersistentCell->persistentCell; persistentCell && persistentCell->IsExteriorCell()) {
				return persistentCell->worldSpace;
			}
		}

		return nullptr;
	}
}
