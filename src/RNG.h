#pragma once

struct BOS_RNG
{
public:
	enum class CHANCE_TYPE
	{
		kRandom,
		kRefHash,
		kLocationHash
	};

	BOS_RNG() = default;
	BOS_RNG(CHANCE_TYPE a_type, const RE::TESObjectREFR* a_ref);

	template <class T>
	T generate(T a_min, T a_max) const
	{
		if (type == CHANCE_TYPE::kRandom) {
			return SeedRNG().generate<T>(a_min, a_max);
		}
		return SeedRNG(seed).generate<T>(a_min, a_max);
	}

	// members
	CHANCE_TYPE   type;
	std::uint64_t seed;
};

using CHANCE_TYPE = BOS_RNG::CHANCE_TYPE;

struct Chance
{
public:
	Chance() = default;
	explicit Chance(const std::string& a_str);

	bool PassedChance(const RE::TESObjectREFR* a_ref) const;

	// members
	CHANCE_TYPE   chanceType{ CHANCE_TYPE::kRefHash };
	float chanceValue{ 100.0f };
};
