#pragma once

#include <XoshiroCpp.hpp>

enum class CHANCE_TYPE
{
	kRandom,
	kRefHash,
	kLocationHash
};

struct RNGBase
{
	RNGBase() = default;

	CHANCE_TYPE   type{ CHANCE_TYPE::kRefHash };
	std::uint64_t seed{ 0 };
};

struct BOS_RNG : RNGBase
{
	BOS_RNG(const RNGBase& a_base);
	BOS_RNG(const RNGBase& a_base, std::uint64_t a_entryHash);

	void Seed(const RE::TESObjectREFR* a_ref, std::uint64_t a_entryHash);

	template <class T>
	T Generate(T a_min, T a_max)
	{
		if (!rng) {
			rng.emplace(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		}
		
		if constexpr (std::is_integral_v<T>) {
			return std::uniform_int_distribution<T>(a_min, a_max)(*rng);
		} else {
			return std::uniform_real_distribution<T>(a_min, a_max)(*rng);
		}
	}

private:
	static std::uint64_t get_form_seed(const RE::TESForm* a_form);

	std::optional<XoshiroCpp::Xoshiro256StarStar> rng{};
};

struct RNGParams : RNGBase
{
	RNGParams() = default;
	explicit RNGParams(const std::string& a_str);

	bool PassedChance(BOS_RNG& a_rng, const RE::TESObjectREFR* a_ref, std::uint64_t a_entryHash) const;

	// members
	float chanceValue{ 100.0f };
};
