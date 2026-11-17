#pragma once

using ConditionData = std::variant<RE::TESForm*, RE::FormID, std::string>;

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
	std::string                conditionID{};  // path|condition1,condition2
	std::vector<ConditionData> NOT{};
	std::vector<ConditionData> MATCH{};
};

template <class T>
using ConditionalData = std::map<ConditionFilters, std::vector<T>>;

struct ConditionalInput
{
	ConditionalInput(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_form) :
		ref(a_ref),
		base(a_form),
		currentWorldspace(a_ref->GetWorldspace())
	{
		currentCell = a_ref->GetParentCell();
		if (!currentCell) {
			currentCell = a_ref->GetSaveParentCell();
		}
		if (currentCell) {
			currentCellEDID = editorID::get_editorID(currentCell);
		}
	}

	[[nodiscard]] RE::BGSLocation*   GetCurrentLocation() const;
	[[nodiscard]] RE::TESRegionList* GetCurrentRegionList() const;

	[[nodiscard]] bool IsValid(RE::FormID a_formID) const;
	[[nodiscard]] bool IsValid(RE::TESForm* a_form) const;
	[[nodiscard]] bool IsValid(const std::string& a_edid) const;

	[[nodiscard]] bool IsValid(const ConditionData& a_data) const;
	[[nodiscard]] bool IsValid(const ConditionFilters& a_filters) const;

	// members
	const RE::TESObjectREFR* ref;
	const RE::TESForm*       base;
	RE::TESObjectCELL*       currentCell;
	RE::TESWorldSpace*       currentWorldspace;
	std::string              currentCellEDID;

private:
	mutable std::optional<RE::BGSLocation*>   lazyLocation{};
	mutable std::optional<RE::TESRegionList*> lazyRegionList{};
};
