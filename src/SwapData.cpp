#include "SwapData.h"

namespace FormSwap
{
	ObjectData::ObjectData(const Input& a_input) :
		properties(a_input.properties),
		chance(a_input.chance),
		record(a_input.record),
		path(a_input.path)
	{
		properties.SetChanceType(chance.chanceType);
	}

	bool ObjectData::HasValidProperties(const RE::TESObjectREFR* a_ref) const
	{
		return chance.PassedChance(a_ref) && properties.IsValid();
	}

	void ObjectData::GetProperties(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, ObjectData&)> a_func)
	{
		const auto formPair = string::split(a_str, "|");
		if (const auto baseFormID = util::GetFormID(formPair[0]); baseFormID != 0) {
			const Input input(
				formPair[1],                                        // transform
				formPair.size() > 2 ? formPair[2] : std::string{},  // traits
				a_str,
				a_path);
			ObjectData objectData(input);
			a_func(baseFormID, objectData);
		} else {
			logger::error("\t\t\t\tfailed to process {} (BASE formID not found)", a_str);
		}
	}

	SwapFormData::SwapFormData(FormIDOrSet a_id, const Input& a_input) :
		ObjectData(a_input),
		formIDSet(std::move(a_id))
	{}

	RE::TESBoundObject* SwapFormData::GetSwapBase(const RE::TESObjectREFR* a_ref) const
	{
		if (!chance.PassedChance(a_ref)) {
			return nullptr;
		}

		if (const auto formID = std::get_if<RE::FormID>(&formIDSet); formID) {
			return RE::TESForm::LookupByID<RE::TESBoundObject>(*formID);
		} else {  // return random element from set
			auto& set = std::get<FormIDSet>(formIDSet);

			const auto setEnd = std::distance(set.begin(), set.end()) - 1;
			const auto randIt = BOS_RNG(chance.chanceType, a_ref).generate<std::int64_t>(0, setEnd);

			return RE::TESForm::LookupByID<RE::TESBoundObject>(*std::next(set.begin(), randIt));
		}
	}

	void SwapFormData::GetForms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, SwapFormData&)> a_func)
	{
		constexpr auto swap_empty = [](const FormIDOrSet& a_set) {
			if (const auto formID = std::get_if<RE::FormID>(&a_set); formID) {
				return *formID == 0;
			} else {
				return std::get<FormIDSet>(a_set).empty();
			}
		};

		const auto formPair = string::split(a_str, "|");

		if (const auto baseFormID = util::GetFormID(formPair[0]); baseFormID != 0) {
			if (const auto swapFormID = util::GetSwapFormID(formPair[1]); !swap_empty(swapFormID)) {
				const Input input(
					formPair.size() > 2 ? formPair[2] : std::string{},  // transform
					formPair.size() > 3 ? formPair[3] : std::string{},  // traits
					a_str,
					a_path);
				SwapFormData swapFormData(swapFormID, input);
				a_func(baseFormID, swapFormData);
			} else {
				logger::error("\t\t\t\tfailed to process {} (SWAP formID not found)", a_str);
			}
		} else {
			logger::error("\t\t\t\tfailed to process {} (BASE formID not found)", a_str);
		}
	}
}
