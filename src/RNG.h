#pragma once

enum class CHANCE_TYPE
{
	kRandom,
	kRefHash,
	kLocationHash
};

struct Chance
{
public:
	Chance() = default;
	explicit Chance(const std::string& a_str);

	bool PassedChance(const RE::TESObjectREFR* a_ref) const;

	// members
	CHANCE_TYPE   chanceType{ CHANCE_TYPE::kRefHash };
	float         chanceValue{ 100.0f };
	std::uint64_t seed{ 0 };
};

struct BOS_RNG
{
public:
	BOS_RNG() = default;
	BOS_RNG(const Chance& a_chance, const RE::TESObjectREFR* a_ref);
	BOS_RNG(const Chance& a_chance);

	template <class T>
	T generate(T a_min, T a_max) const
	{
		if (type == CHANCE_TYPE::kRandom && seed == 0) {
			return SeedRNG().generate<T>(a_min, a_max);
		}
		return SeedRNG(seed).generate<T>(a_min, a_max);
	}

	// members
	CHANCE_TYPE   type;
	std::uint64_t seed{ 0 };
};
