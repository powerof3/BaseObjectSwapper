#include "RNG.h"

BOS_RNG::BOS_RNG(const Chance& a_chance, const RE::TESObjectREFR* a_ref) :
	type(a_chance.chanceType)
{
	switch (type) {
	case CHANCE_TYPE::kRefHash:
		seed = a_ref->GetFormID();
		break;
	case CHANCE_TYPE::kLocationHash:
		{
			auto baseID = a_ref->GetBaseObject()->GetFormID();
			auto location = a_ref->GetCurrentLocation();
			auto cell = a_ref->GetSaveParentCell();

			if (!location && !cell) {
				seed = a_ref->GetFormID();
			} else {
				RE::FormID locID = 0;
				if (location) {
					locID = location->GetFormID();
				} else if (cell) {
					locID = cell->GetFormID();
				}
				// generate hash based on location + baseID
				seed = hash::szudzik_pair(locID, baseID);
			}
		}
		break;
	case CHANCE_TYPE::kRandom:
		seed = a_chance.seed;
		break;
	default:
		break;
	}
}

BOS_RNG::BOS_RNG(const Chance& a_chance) :
	type(a_chance.chanceType),
	seed(a_chance.seed)
{}

Chance::Chance(const std::string& a_str)
{
	if (distribution::is_valid_entry(a_str)) {
		if (a_str.contains("chance")) {
			if (a_str.contains("R")) {
				chanceType = CHANCE_TYPE::kRandom;
			} else if (a_str.contains("L")) {
				chanceType = CHANCE_TYPE::kLocationHash;
			} else {
				chanceType = CHANCE_TYPE::kRefHash;
			}

			if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, regex::generic)) {
				const auto chanceOptions = string::split(match[1].str(), ",");
				chanceValue = string::to_num<float>(chanceOptions[0]);
				seed = chanceOptions.size() > 1 ? string::to_num<std::uint64_t>(chanceOptions[1]) : 0;
			}
		}
	}
}

bool Chance::PassedChance(const RE::TESObjectREFR* a_ref) const
{
	if (chanceValue < 100.0f) {
		BOS_RNG rng(*this, a_ref);
		if (const auto rngValue = rng.generate<float>(0.0f, 100.0f); rngValue > chanceValue) {
			return false;
		}
	}
	return true;
}
