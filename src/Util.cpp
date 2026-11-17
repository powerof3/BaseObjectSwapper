#include "Util.h"

namespace util
{
	std::vector<std::string> split_with_regex(const std::string& a_str, const boost::regex& a_regex)
	{
		boost::sregex_token_iterator iter(a_str.begin(),
			a_str.end(),
			a_regex,
			-1);
		boost::sregex_token_iterator end{};
		return { iter, end };
	}

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
}
