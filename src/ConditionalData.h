#pragma once

struct ConditionFilters
{
public:
	ConditionFilters() = default;
	ConditionFilters(std::string a_conditionID, std::vector<std::string>& a_conditions);

	bool operator==(const ConditionFilters& a_rhs) const
	{
		return conditionID == a_rhs.conditionID;
	}

	bool operator<(const ConditionFilters& a_rhs) const
	{
		return conditionID < a_rhs.conditionID;
	}

	// members
	std::string            conditionID{}; // path|condition1,condition2
	std::vector<FormIDStr> NOT{};
	std::vector<FormIDStr> MATCH{};
};

template <class T>
using ConditionalData = std::map<ConditionFilters, std::vector<T>>;

struct ConditionalInput
{
	ConditionalInput(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_form) :
		ref(a_ref),
		base(a_form),
		currentCell(a_ref->GetSaveParentCell()),
		currentWorldspace(a_ref->GetWorldspace()),
		currentLocation(a_ref->GetCurrentLocation()),
		currentRegionList(currentCell ? currentCell->GetRegionList(false) : nullptr)
	{}

	[[nodiscard]] bool IsValid(RE::FormID a_formID) const;
	[[nodiscard]] bool IsValid(const std::string& a_edid) const;

	[[nodiscard]] bool IsValid(const FormIDStr& a_data) const;
	[[nodiscard]] bool IsValid(const ConditionFilters& a_filters) const;

	// members
	const RE::TESObjectREFR* ref;
	const RE::TESForm*       base;
	RE::TESObjectCELL*       currentCell;
	RE::TESWorldSpace*       currentWorldspace;
	RE::BGSLocation*         currentLocation;
	RE::TESRegionList*       currentRegionList;
};
