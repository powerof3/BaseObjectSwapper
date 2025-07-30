#include "RNG.h"

std::uint64_t BOS_RNG::get_form_seed(const RE::TESForm* a_form)
{
	if (a_form->IsDynamicForm()) {
		return a_form->GetFormID();
	}

	std::uint64_t result = 0;
	boost::hash_combine(result, a_form->GetLocalFormID());

	auto fileName = a_form->GetFile(0)->GetFilename();
	if (a_form->AsReference() && (a_form->GetFormID() & 0xFF000000) == 0) {
		fileName = "Skyrim.esm"sv;
	}
	boost::hash_combine(result, fileName);

	return result;
}

BOS_RNG::BOS_RNG(const Chance& a_chance, const RE::TESObjectREFR* a_ref) :
	type(a_chance.chanceType)
{
	switch (type) {
	case CHANCE_TYPE::kRefHash:
		seed = get_form_seed(a_ref);
		break;
	case CHANCE_TYPE::kLocationHash:
		{
			auto base = a_ref->GetBaseObject();
			auto location = a_ref->GetEditorLocation();
			auto cell = a_ref->GetSaveParentCell();

			if (!location && !cell) {
				seed = get_form_seed(a_ref);
			} else {
				RE::TESForm* cellOrLoc = nullptr;
				if (location) {
					cellOrLoc = location;
				} else if (cell) {
					cellOrLoc = cell;
				}
				// generate hash based on location + baseID
				std::uint64_t result = 0;
				boost::hash_combine(result, get_form_seed(cellOrLoc));
				boost::hash_combine(result, get_form_seed(base));
				seed = result;
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
