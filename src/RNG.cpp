#include "RNG.h"

BOS_RNG::BOS_RNG(const RNGBase& a_base) :
	RNGBase(a_base)
{}

BOS_RNG::BOS_RNG(const RNGBase& a_base, std::uint64_t a_entryHash) :
	RNGBase(a_base)
{
	std::uint64_t mixSeed = seed;
	boost::hash_combine(mixSeed, a_entryHash);
	rng.emplace(mixSeed);
}

void BOS_RNG::Seed(const RE::TESObjectREFR* a_ref, std::uint64_t a_entryHash)
{
	if (rng) {
		return;
	}

	const auto make_seed = [&](std::uint64_t a_seed) {
		std::uint64_t mixSeed = a_seed;
		boost::hash_combine(mixSeed, a_entryHash);
		boost::hash_combine(mixSeed, seed);
		return mixSeed;
	};

	switch (type) {
	case CHANCE_TYPE::kRefHash:
		{
			if (a_ref) {
				seed = make_seed(get_form_seed(a_ref));
			} else {
				type = CHANCE_TYPE::kRandom;
				seed = seed != 0 ? make_seed(0) : 0;
			}
		}
		break;
	case CHANCE_TYPE::kLocationHash:
		{
			const auto locOrCell = a_ref ? RE::GetLocationOrCell(a_ref) : nullptr;
			const auto base = a_ref ? a_ref->GetBaseObject() : nullptr;

			if (locOrCell && base) {
				auto mixSeed = make_seed(get_form_seed(locOrCell));
				boost::hash_combine(mixSeed, get_form_seed(base));
				seed = mixSeed;
			} else if (a_ref) {
				seed = make_seed(get_form_seed(a_ref));
			} else {
				type = CHANCE_TYPE::kRandom;
				seed = seed != 0 ? make_seed(0) : 0;
			}
		}
		break;
	case CHANCE_TYPE::kRandom:
		{
			if (seed != 0) {
				seed = make_seed(a_ref ? get_form_seed(a_ref) : 0);
			}
		}
		break;
	default:
		break;
	}

	if (type == CHANCE_TYPE::kRandom && seed == 0) {
		rng.emplace(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	} else {
		rng.emplace(seed);
	}
}

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

RNGParams::RNGParams(const std::string& a_str)
{
	if (distribution::is_valid_entry(a_str)) {
		if (a_str.contains("chanceR")) {
			type = CHANCE_TYPE::kRandom;
		} else if (a_str.contains("chanceL")) {
			type = CHANCE_TYPE::kLocationHash;
		} else if (a_str.contains("chance")) {
			type = CHANCE_TYPE::kRefHash;
		} else {
			return;
		}

		if (boost::cmatch match; boost::regex_search(a_str.c_str(), match, regex::generic)) {
			if (const auto chanceOptions = REX::STR::SPLIT(match[1].str(), ","); !chanceOptions.empty()) {
				chanceValue = REX::STR::TO_NUM<float>(chanceOptions[0]);
				seed = chanceOptions.size() > 1 ? REX::STR::TO_NUM<std::uint64_t>(chanceOptions[1]) : 0;
			}
		}
	}
}

bool RNGParams::PassedChance(BOS_RNG& a_rng, const RE::TESObjectREFR* a_ref, std::uint64_t a_entryHash) const
{
	if (chanceValue < 100.0f) {
		a_rng.Seed(a_ref, a_entryHash);
		if (const auto rngValue = a_rng.Generate<float>(0.0f, 100.0f); rngValue > chanceValue) {
			return false;
		}
	}
	return true;
}
