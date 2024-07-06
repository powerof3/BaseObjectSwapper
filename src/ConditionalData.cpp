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
		if (const auto processedID = util::GetFormID(condition); processedID != 0) {
			negate ? NOT.emplace_back(processedID) : MATCH.emplace_back(processedID);
		} else {
			logger::error("\t\tFilter [{}] INFO - unable to find form, treating filter as FF keyword or cell editorID", condition);
			negate ? NOT.emplace_back(condition) : MATCH.emplace_back(condition);
		}
	}
}

bool ConditionalInput::IsValid(RE::FormID a_formID) const
{
	if (const auto form = RE::TESForm::LookupByID(a_formID)) {
		switch (form->GetFormType()) {
		case RE::FormType::Location:
			{
				const auto location = form->As<RE::BGSLocation>();
				return currentLocation && (currentLocation == location || currentLocation->IsParent(location));
			}
		case RE::FormType::Region:
			{
				if (const auto region = form->As<RE::TESRegion>()) {
					if (currentRegionList) {
						return std::ranges::any_of(*currentRegionList, [&](const auto& regionInList) {
							return regionInList && regionInList == region;
						});
					}
				}
				return false;
			}
		case RE::FormType::Keyword:
			{
				const auto keyword = form->As<RE::BGSKeyword>();
				return currentLocation && currentLocation->HasKeyword(keyword) || ref->HasKeyword(keyword);
			}
		case RE::FormType::Cell:
			return currentCell == form;
		default:
			break;
		}
	}

	return false;
}

bool ConditionalInput::IsValid(const std::string& a_edid) const
{
	if (currentCell && string::iequals(editorID::get_editorID(currentCell), a_edid)) {
		return true;
	}

	if (currentLocation && currentLocation->HasKeywordString(a_edid)) {
		return true;
	}

	if (const auto keywordForm = base->As<RE::BGSKeywordForm>()) {
		return keywordForm->HasKeywordString(a_edid);
	}

	return false;
}

bool ConditionalInput::IsValid(const FormIDStr& a_data) const
{
	bool result = false;

	std::visit(overload{
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
