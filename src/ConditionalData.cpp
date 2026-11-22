#include "ConditionalData.h"

// sort filter according to how expensive it is
std::int32_t FilterRule::GetFilterCost(bool a_allFilter) const
{
	std::int32_t cost = 0;

	std::visit(overload{
				   [&](RE::TESForm* a_form) {
					   switch (a_form->GetFormType()) {
					   case RE::FormType::Cell:
					   case RE::FormType::WorldSpace:
						   cost += 5;
						   break;
					   case RE::FormType::Region:
						   cost += 15;
						   break;
					   case RE::FormType::Keyword:
						   cost += 25;
						   break;
					   case RE::FormType::Location:
						   cost += 30;
						   break;
					   case RE::FormType::FormList:
						   cost += 45;
						   break;
					   default:
						   cost += 20;
						   break;
					   }
				   },
				   [&](RE::FormID) { cost += 20; },
				   [&](const std::string&) { cost += (partialModifier ? 40 : 30); } },
		data);

	if (a_allFilter ? !excludeModifier : excludeModifier) {
		cost -= 1;
	}

	return cost;
}

FilterRule FilterRule::FromEntry(std::string a_entry)
{
	const auto hasExcludeModifier = a_entry[0] == '-';

	if (hasExcludeModifier || a_entry[0] == '+') {  // -*Guard
		a_entry.erase(0, 1);                        // *Guard
	}

	const auto hasPartialModifier = !a_entry.empty() && a_entry[0] == '*';
	if (hasPartialModifier) {
		a_entry.erase(0, 1);  // Guard
	}

	return FilterRule(hasExcludeModifier, hasPartialModifier, a_entry);
}

FilterRule::FilterRule(bool a_excludeModifier, bool a_partialModifier, const std::string& a_value) :
	excludeModifier(a_excludeModifier),
	partialModifier(a_partialModifier)
{
	if (a_partialModifier) {
		data = a_value;
		return;
	}
	if (const auto [processedID, form] = RE::GetFormWithID(a_value, true); processedID != 0) {
		if (form && !form->IsDynamicForm()) {
			data = form;
		} else {
			data = processedID;
		}
	} else {
		REX::ERROR("\t\tFilter [{}] INFO - unable to find form, treating filter as FF keyword or cell editorID", a_value);
		data = a_value;
	}
}

ConditionFilters::ConditionFilters(std::vector<std::string>& a_conditions, std::uint32_t a_fileIndex) :
	fileIndex(a_fileIndex)
{
	for (auto& condition : a_conditions) {
		REX::STR::TRIM(condition);
		if (!distribution::is_valid_entry(condition)) {
			continue;
		}
		if (condition.contains('+')) {
			// A, (X + Y + Z), B
			FilterGroup group;
			for (auto& ALLEntry : REX::STR::SPLIT(condition, "+")) {
				REX::STR::TRIM(ALLEntry);
				if (ALLEntry.empty()) {
					continue;
				}
				group.emplace_back(FilterRule::FromEntry(ALLEntry));
			}
			if (!group.empty()) {
				ALL.emplace_back(std::move(group));
			}
		} else {
			// A or *B or -C or -*D
			ANY.emplace_back(FilterRule::FromEntry(condition));
		}
	}

	std::ranges::stable_sort(ANY, {}, [&](const auto& f) { return f.GetFilterCost(false); });
	for (auto& group : ALL) {
		std::ranges::stable_sort(group, {}, [&](const auto& f) { return f.GetFilterCost(true); });
	}
}

std::string_view ConditionalInput::GetCurrentCellEDID() const
{
	if (!lazyCellEDID) {
		lazyCellEDID = currentCell ? currentCell->GetFormEditorID() : std::string_view{};
	}
	return *lazyCellEDID;
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
		case RE::FormType::FormList:
			{
				bool result = false;

				const auto list = a_form->As<RE::BGSListForm>();
				list->ForEachForm([&](RE::TESForm* a_formInList) {
					if (result = IsValid(a_formInList); result) {
						return RE::BSContainer::ForEachResult::kStop;
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});

				return result;
			}
		default:
			break;
		}
	}

	return false;
}

bool ConditionalInput::IsValid(const std::string& a_string) const
{
	if (currentCell && REX::STR::IEQUALS(GetCurrentCellEDID(), a_string)) {
		return true;
	}

	if (const auto currentLocation = GetCurrentLocation(); currentLocation && currentLocation->HasKeywordString(a_string)) {
		return true;
	}

	const auto keywordForm = base->As<RE::BGSKeywordForm>();
	return keywordForm && keywordForm->HasKeywordString(a_string);
}

bool ConditionalInput::IsAnyValid(const std::string& a_string) const
{
	if (const auto currentLocation = GetCurrentLocation(); currentLocation && currentLocation->ContainsKeywordString(a_string)) {
		return true;
	}

	const auto keywordForm = base->As<RE::BGSKeywordForm>();
	return keywordForm && keywordForm->ContainsKeywordString(a_string);
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

bool ConditionalInput::IsValid(const FilterRule& a_rule) const
{
	return a_rule.partialModifier ? IsAnyValid(std::get<std::string>(a_rule.data)) : IsValid(a_rule.data);
}

bool ConditionalInput::IsValid(const ConditionFilters& a_filters) const
{
	const auto matches_all = [&](const FilterGroup& a_group) {
		for (const auto& f : a_group) {
			if (f.excludeModifier) {
				if (IsValid(f)) {
					return false;
				}
			} else if (!IsValid(f)) {
				return false;
			}
		}
		return true;
	};

	const auto matches_any = [&](const std::vector<FilterRule>& a_group) {
		bool hasExact = false;
		bool exactPassed = false;

		bool hasPartial = false;
		bool partialPassed = false;

		for (const auto& f : a_group) {
			if (f.excludeModifier) {
				if (IsValid(f)) {
					return false;
				}
				continue;
			}
			if (f.partialModifier) {
				hasPartial = true;
				if (!partialPassed && IsValid(f)) {
					partialPassed = true;
				}
			} else {
				hasExact = true;
				if (!exactPassed && IsValid(f)) {
					exactPassed = true;
				}
			}
		}

		return (!hasExact || exactPassed) && (!hasPartial || partialPassed);
	};

	// ALL filters; at least one filter group must match (X+Y+Z or A+B+C)
	if (!a_filters.ALL.empty()) {
		bool any_group_matched = false;
		for (const auto& group : a_filters.ALL) {
			if (matches_all(group)) {
				any_group_matched = true;
				break;
			}
		}
		if (!any_group_matched) {
			return false;
		}
	}

	if (!a_filters.ANY.empty() && !matches_any(a_filters.ANY)) {
		return false;
	}

	return true;
}
