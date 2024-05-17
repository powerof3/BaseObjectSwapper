#pragma once

struct ConditionFilters
{
public:
	ConditionFilters() = default;
	ConditionFilters(std::vector<std::string>& a_conditions);

	void append_range(const ConditionFilters& a_rhs);

	// members
	std::vector<FormIDStr> NOT{};
	std::vector<FormIDStr> MATCH{};
};

template <class T>
struct ConditionalData
{
public:
	ConditionalData() = default;
	ConditionalData(const ConditionFilters& a_filters, const T& a_data) :
		filters(a_filters)
	{
		data.push_back(a_data);
	}

	// members
	ConditionFilters filters;
	std::vector<T>   data;
};

struct ConditionalInput
{
	ConditionalInput(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_form) :
		ref(a_ref),
		base(a_form),
		currentCell(a_ref->GetSaveParentCell()),
		currentLocation(a_ref->GetCurrentLocation()),
		currentRegionList(currentCell ? currentCell->GetRegionList(false): nullptr)
	{}

	[[nodiscard]] bool IsValid(RE::FormID a_formID) const;
	[[nodiscard]] bool IsValid(const std::string& a_edid) const;

	[[nodiscard]] bool IsValid(const FormIDStr& a_data) const;
	[[nodiscard]] bool IsValid(const ConditionFilters& a_filters) const;

	// members
	const RE::TESObjectREFR* ref;
	const RE::TESForm*       base;
	RE::TESObjectCELL*       currentCell;
	RE::BGSLocation*         currentLocation;
	RE::TESRegionList*       currentRegionList;
};
