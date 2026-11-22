#pragma once

namespace regex
{
	inline boost::regex generic{ R"(\((.*?)\))" };                // pos(0,0,100) -> "0,0,100"
	inline boost::regex transform{ R"(\((.*?),(.*?),(.*?)\))" };  // pos(0,0,100) -> 0, 0, 100
	inline boost::regex string{ R"(,\s*(?![^()]*\)))" };          // pos(0, 0, 100), rot(0, 0, 100) -> "pos(0, 0, 100)","rot(0, 0, 100)"
}

namespace RE
{
	RE::FormID                          LookupFormID(RE::FormID a_rawFormID, std::string_view a_modName);
	std::pair<RE::FormID, RE::TESForm*> GetFormWithID(const std::string& a_str, bool a_resolveForm);
	RE::FormID                          GetFormID(const std::string& a_str);

	FormIDOrSet      GetSwapFormID(const std::string& a_str);
	FormIDOrderedSet GetFormIDOrderedSet(const std::string& a_str);

	RE::TESForm*       GetLocationOrCell(const RE::TESObjectREFR* a_ref);
	RE::TESObjectCELL* GetParentCell(const RE::TESObjectREFR* a_ref);
	bool               IsFixedPersistentForm(const RE::TESBoundObject* a_base);
	RE::TESWorldSpace* GetPersistentRefWorldSpace(const RE::TESObjectREFR* a_ref);
	bool               IsBaseObject(const RE::TESForm* a_form);
}
