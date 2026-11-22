#pragma once

using ConditionData = std::variant<RE::TESForm*, RE::FormID, std::string>;

// X / -X / *X / -*X
struct FilterRule
{
	FilterRule() = default;

	static FilterRule          FromEntry(std::string a_entry);
	[[nodiscard]] std::int32_t GetFilterCost(bool a_allFilter) const;

	// members
	bool          excludeModifier{ false };  // -
	bool          partialModifier{ false };  // *
	ConditionData data{};

private:
	FilterRule(bool a_excludeModifier, bool a_partialModifier, const std::string& a_value);
};

using FilterGroup = std::vector<FilterRule>;  // Guard+*Mage+-Thief+-*Bandit

struct ConditionFilters
{
	ConditionFilters() = default;
	ConditionFilters(std::vector<std::string>& a_conditions, std::uint32_t a_fileIndex);

	// members
	std::vector<FilterGroup> ALL{};  // Guard+*Mage,-Thief+Horse
	FilterGroup              ANY{};  // Guard,*Mage,-Thief,-*Bandit
	std::uint32_t            fileIndex{ 0 };
};

using ConditionFiltersPtr = std::shared_ptr<const ConditionFilters>;

template <class T>
using ConditionalData = InsertionMap<ConditionFiltersPtr, std::vector<T>>;

struct ConditionalInput
{
	ConditionalInput(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_form) :
		ref(a_ref),
		base(a_form),
		currentWorldspace(a_ref->GetWorldspace())
	{
		currentCell = RE::GetParentCell(a_ref);
	}

	[[nodiscard]] std::string_view   GetCurrentCellEDID() const;
	[[nodiscard]] RE::BGSLocation*   GetCurrentLocation() const;
	[[nodiscard]] RE::TESRegionList* GetCurrentRegionList() const;

	[[nodiscard]] bool IsValid(RE::FormID a_formID) const;
	[[nodiscard]] bool IsValid(RE::TESForm* a_form) const;
	[[nodiscard]] bool IsValid(const std::string& a_string) const;
	[[nodiscard]] bool IsAnyValid(const std::string& a_string) const;

	[[nodiscard]] bool IsValid(const ConditionData& a_data) const;
	[[nodiscard]] bool IsValid(const FilterRule& a_rule) const;
	[[nodiscard]] bool IsValid(const ConditionFilters& a_filters) const;

	// members
	const RE::TESObjectREFR* ref;
	const RE::TESForm*       base;
	RE::TESObjectCELL*       currentCell;
	RE::TESWorldSpace*       currentWorldspace;

private:
	mutable std::optional<std::string_view>   lazyCellEDID{};
	mutable std::optional<RE::BGSLocation*>   lazyLocation{};
	mutable std::optional<RE::TESRegionList*> lazyRegionList{};
};
