#include "SwapData.h"

namespace FormSwap
{
	std::uint64_t ObjectData::Input::GenerateHash() const
	{
		std::uint64_t seed = 0;
		boost::hash_combine(seed, record);
		return seed;
	}

	const ObjectProperties* IndexedProperties::GetOrCreate(const std::string& a_str)
	{
		if (a_str.empty()) {
			return nullptr;
		}
		const auto [it, result] = index.try_emplace(a_str, nullptr);
		if (result) {
			if (ObjectProperties props(a_str); props.IsValid()) {
				it->second = storage.emplace_back(std::make_unique<const ObjectProperties>(std::move(props))).get();
			}
		}
		return it->second;
	}

	void IndexedProperties::ReleaseIndex()
	{
		StringMap<const ObjectProperties*>().swap(index);
		storage.shrink_to_fit();
	}

	ObjectData::ObjectData(const Input& a_input, IndexedProperties& a_properties) :
		properties(a_properties.GetOrCreate(a_input.properties)),
		chance(a_input.chance),
		entryHash(a_input.GenerateHash())
	{}

	std::uint64_t ObjectData::GetPropertiesHash() const
	{
		std::uint64_t hash = entryHash;
		boost::hash_combine(hash, 'TRF');
		return hash;
	}

	bool ObjectData::HasValidProperties(const RE::TESObjectREFR* a_ref) const
	{
		if (!properties) {
			return false;
		}
		BOS_RNG rng(chance);
		return chance.PassedChance(rng, a_ref, entryHash);
	}

	void ObjectData::ApplyProperties(RE::TESObjectREFR* a_ref) const
	{
		properties->SetTransform(a_ref, chance, GetPropertiesHash());
		properties->SetRecordFlags(a_ref);
	}

	void ObjectData::GetProperties(const std::string& a_str, IndexedProperties& a_properties, bool a_resolveForm, std::function<void(RE::FormID, const RE::TESForm*, ObjectData&&)> a_func)
	{
		const auto formPair = REX::STR::SPLIT(a_str, "|");
		if (const auto [baseFormID, baseForm] = RE::GetFormWithID(formPair[0], a_resolveForm); baseFormID != 0) {
			const Input input(
				formPair[1],                                        // transform
				formPair.size() > 2 ? formPair[2] : std::string{},  // traits
				a_str);
			a_func(baseFormID, baseForm, ObjectData(input, a_properties));
		} else {
			REX::ERROR("\t\t\t\tfail : [{}] (BASE formID not found)", a_str);
		}
	}

	SwapTargets SwapFormData::make_targets(const FormIDOrSet& a_id)
	{
		if (const auto formID = std::get_if<RE::FormID>(&a_id)) {
			return RE::TESForm::LookupByID<RE::TESBoundObject>(*formID);
		}

		const auto&                      ids = std::get<FormIDSet>(a_id);
		std::vector<RE::TESBoundObject*> targets;
		targets.reserve(ids.size());
		for (const auto id : ids) {
			targets.emplace_back(RE::TESForm::LookupByID<RE::TESBoundObject>(id));
		}
		return targets;
	}

	SwapFormData::SwapFormData(const FormIDOrSet& a_id, const Input& a_input, IndexedProperties& a_properties) :
		ObjectData(a_input, a_properties),
		targets(make_targets(a_id))
	{}

	SwapFormData::SwapFormData(const ObjectData& a_objectData, const FormIDOrSet& a_id) :
		ObjectData(a_objectData),
		targets(make_targets(a_id))
	{}

	RE::TESBoundObject* SwapFormData::GetSwapBase(const RE::TESObjectREFR* a_ref) const
	{
		BOS_RNG rng(chance);
		if (!chance.PassedChance(rng, a_ref, entryHash)) {
			return nullptr;
		}

		if (const auto target = std::get_if<RE::TESBoundObject*>(&targets)) {
			return *target;
		}
		if (const auto& set = std::get<std::vector<RE::TESBoundObject*>>(targets); !set.empty()) {
			rng.Seed(a_ref, entryHash);
			return set[rng.Generate<std::size_t>(0, set.size() - 1)];
		}

		return nullptr;
	}

	void SwapFormData::GetForms(const std::string& a_str, IndexedProperties& a_properties, std::function<void(RE::FormID, SwapFormData&&)> a_func)
	{
		constexpr auto swap_empty = [](const FormIDOrSet& a_set) {
			if (const auto formID = std::get_if<RE::FormID>(&a_set); formID) {
				return *formID == 0;
			} else {
				return std::get<FormIDSet>(a_set).empty();
			}
		};

		constexpr auto base_same_as_swap = [](RE::FormID a_baseID, const FormIDOrSet& a_set) {
			if (const auto formID = std::get_if<RE::FormID>(&a_set); formID) {
				return *formID == a_baseID;
			} else {
				return false;
			}
		};

		const auto formPair = REX::STR::SPLIT(a_str, "|");

		if (const auto baseFormID = RE::GetFormID(formPair[0]); baseFormID != 0) {
			if (const auto swapFormID = RE::GetSwapFormID(formPair[1]); !swap_empty(swapFormID)) {
				auto properties = formPair.size() > 2 ? formPair[2] : std::string{};
				auto chance = formPair.size() > 3 ? formPair[3] : std::string{};

				if (base_same_as_swap(baseFormID, swapFormID) && !distribution::is_valid_entry(properties)) {
					REX::ERROR("\t\t\t\tfail : [{}] (BASE formID == SWAP formID)", a_str);
					return;
				}

				const Input input(properties, chance, a_str);

				a_func(baseFormID, SwapFormData(swapFormID, input, a_properties));
			} else {
				REX::ERROR("\t\t\t\tfail : [{}] (SWAP formID not found)", a_str);
			}
		} else if (const auto baseFormIDs = RE::GetFormIDOrderedSet(formPair[0]); !baseFormIDs.empty()) {
			if (auto swapFormIDs = RE::GetFormIDOrderedSet(formPair[1]); !swapFormIDs.empty()) {
				auto properties = formPair.size() > 2 ? formPair[2] : std::string{};
				auto chance = formPair.size() > 3 ? formPair[3] : std::string{};

				// assign each baseFormID the same swapFormID
				if (swapFormIDs.size() == 1) {
					const Input  input(properties, chance, a_str);
					SwapFormData swapFormData(*(swapFormIDs.begin()), input, a_properties);

					for (auto itBaseFormID : baseFormIDs) {
						a_func(itBaseFormID, SwapFormData(swapFormData));
					}

					// randomly assign each baseFormID to a unique swapFormID
				} else if (swapFormIDs.size() >= baseFormIDs.size()) {
					const Input      input(properties, std::string{}, a_str);
					const ObjectData objectData(input, a_properties);

					auto rng = BOS_RNG(RNGParams(chance), objectData.entryHash);

					for (auto itBaseFormID : baseFormIDs) {
						const auto setEnd = std::distance(swapFormIDs.begin(), swapFormIDs.end()) - 1;
						const auto randIt = rng.Generate<std::int64_t>(0, setEnd);
						auto       swapFormID = swapFormIDs.extract(*std::next(swapFormIDs.begin(), randIt));
						if (swapFormID) {
							a_func(itBaseFormID, SwapFormData(objectData, swapFormID.value()));
						}
					}
				} else {
					REX::ERROR("\t\t\t\tfail : [{}] (SWAP formIDSet.size() must be 1 OR equal/greater than BASE formIDSet.size())", a_str);
				}
			} else {
				REX::ERROR("\t\t\t\tfail : [{}] (SWAP formID set not found)", a_str);
			}
		} else {
			REX::ERROR("\t\t\t\tfail : [{}] (BASE formID not found)", a_str);
		}
	}
}
