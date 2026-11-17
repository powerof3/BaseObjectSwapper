#include "ConditionalData.h"

ConditionFilters::ConditionFilters(std::string a_conditionID, std::vector<std::string>& a_conditions) :
	conditionID(std::move(a_conditionID))
{
	NOT.reserve(a_conditions.size());
	MATCH.reserve(a_conditions.size());

	for (auto& condition : a_conditions) {
		bool negate = false;
		if (!condition.empty() && condition[0] == '-') {
			condition.erase(0, 1);
			negate = true;
		}
		if (const auto [processedID, form] = RE::GetFormWithID(condition, true); processedID != 0) {
			if (form && !form->IsDynamicForm()) {
				negate ? NOT.emplace_back(form) : MATCH.emplace_back(form);
			} else {
				negate ? NOT.emplace_back(processedID) : MATCH.emplace_back(processedID);
			}
		} else {
			REX::ERROR("\t\tFilter [{}] INFO - unable to find form, treating filter as FF keyword or cell editorID", condition);
			negate ? NOT.emplace_back(condition) : MATCH.emplace_back(condition);
		}
	}
}

RE::BGSLocation* ConditionalInput::GetCurrentLocation() const
{
	if (!lazyLocation) {
		lazyLocation = ref->GetCurrentLocation();
	}
	return *lazyLocation;
}

RE::TESRegionList* ConditionalInput::GetCurrentRegionList() const

{
	if (!lazyRegionList) {
		lazyRegionList = currentCell ? currentCell->GetRegionList(false) : nullptr;
	}
	return *lazyRegionList;
}

bool ConditionalInput::IsValid(RE::FormID a_formID) const
{
	return IsValid(RE::TESForm::LookupByID(a_formID));
}

bool ConditionalInput::IsValid(RE::TESForm* a_form) const
{
	if (a_form) {
		switch (a_form->GetFormType()) {
		case RE::FormType::Location:
			{
				const auto location = a_form->As<RE::BGSLocation>();
				const auto currentLocation = GetCurrentLocation();
				return currentLocation && (currentLocation == location || currentLocation->IsParent(location));
			}
		case RE::FormType::Region:
			{
				if (const auto region = a_form->As<RE::TESRegion>()) {
					if (const auto currentRegionList = GetCurrentRegionList()) {
						return std::ranges::any_of(*currentRegionList, [&](const auto& regionInList) {
							return regionInList && regionInList == region;
						});
					}
				}
				return false;
			}
		case RE::FormType::Keyword:
			{
				const auto keyword = a_form->As<RE::BGSKeyword>();
				if (ref->HasKeyword(keyword)) {
					return true;
				}
				const auto currentLocation = GetCurrentLocation();
				return currentLocation && currentLocation->HasKeyword(keyword);
			}
		case RE::FormType::Cell:
			return currentCell == a_form;
		case RE::FormType::WorldSpace:
			{
				const auto worldspace = a_form->As<RE::TESWorldSpace>();
				return currentWorldspace && (currentWorldspace == worldspace || currentWorldspace->parentWorld == worldspace);
			}
		default:
			break;
		}
	}

	return false;
}

bool ConditionalInput::IsValid(const std::string& a_edid) const
{
	if (currentCell && REX::STR::IEQUALS(currentCellEDID, a_edid)) {
		return true;
	}

	if (auto currentLocation = GetCurrentLocation(); currentLocation && currentLocation->HasKeywordString(a_edid)) {
		return true;
	}

	if (const auto keywordForm = base->As<RE::BGSKeywordForm>()) {
		return keywordForm->HasKeywordString(a_edid);
	}

	return false;
}

bool ConditionalInput::IsValid(const ConditionData& a_data) const
{
	bool result = false;

	std::visit(overload{
				   [&](RE::TESForm* a_form) {
					   result = IsValid(a_form);
				   },
				   [&](RE::FormID a_formID) {
					   result = IsValid(a_formID);
				   },
				   [&](const std::string& a_edid) {
					   result = IsValid(a_edid);
				   } },
		a_data);

	return result;
}

bool ConditionalInput::IsValid(const ConditionFilters& a_filters) const
{
	if (!a_filters.NOT.empty()) {
		if (std::ranges::any_of(a_filters.NOT, [this](const auto& data) { return IsValid(data); })) {
			return false;
		}
	}

	if (!a_filters.MATCH.empty()) {
		if (std::ranges::none_of(a_filters.MATCH, [this](const auto& data) { return IsValid(data); })) {
			return false;
		}
	}

	return true;
}
