#pragma once

#include "MergeMapper.h"

namespace FormSwap
{
	using FormOrEditorID = std::variant<RE::FormID, std::string>;

	template <class K, class D>
	using Map = robin_hood::unordered_flat_map<K, D>;
	template <class T>
	using Set = robin_hood::unordered_flat_set<T>;

	using FormIDSet = Set<RE::FormID>;
	using FormIDOrSet = std::variant<RE::FormID, FormIDSet>;
    inline bool swap_empty(const FormIDOrSet& a_set)
	{
		if (const auto formID = std::get_if<RE::FormID>(&a_set); formID) {
			return *formID == 0;
		} else {
			return std::get<FormIDSet>(a_set).empty();
		}
	}

	class SeedRNG
	{
	public:
		SeedRNG() = delete;
		explicit SeedRNG(const RE::FormID a_seed) :
			rng(a_seed)
		{}

		template <class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
		T Generate(T a_min, T a_max)
		{
			if constexpr (std::is_integral_v<T>) {
				std::uniform_int_distribution<T> distr(a_min, a_max);
				return distr(rng);
			} else {
				std::uniform_real_distribution<T> distr(a_min, a_max);
				return distr(rng);
			}
		}

	private:
		XoshiroCpp::Xoshiro256StarStar rng;
	};

	class Transform
	{
	public:
		template <class T>
		using minMax = std::pair<T, T>;
		template <class T>
		using relData = std::pair<bool, minMax<T>>;  //relative vs absolute

		Transform() = default;
		explicit Transform(const std::string& a_str);

		void SetTransform(RE::TESObjectREFR* a_refr);

	private:
		[[nodiscard]] static relData<RE::NiPoint3> get_transform_from_string(const std::string& a_str);
		[[nodiscard]] static std::optional<minMax<float>> get_scale_from_string(const std::string& a_str);

		std::optional<relData<RE::NiPoint3>> location{ std::nullopt };
		std::optional<relData<RE::NiPoint3>> rotation{ std::nullopt };
		std::optional<minMax<float>> refScale{ std::nullopt };

		static inline srell::regex transformRegex{ R"(\((.*?),(.*?),(.*?)\))" };
		static inline srell::regex scaleRegex{ R"(\((.*?)\))" };
	};

	struct Traits
	{
		Traits() = default;
		explicit Traits(const std::string& a_str);

	    bool trueRandom{ false };
	};

	class SwapData
	{
	public:
		SwapData() = delete;
		SwapData(FormIDOrSet a_id, const std::string& a_transformStr, const std::string& a_traitsStr);

		[[nodiscard]] static RE::FormID GetFormID(const std::string& a_str);
		[[nodiscard]] static FormIDOrSet GetSwapFormID(const std::string& a_str);

		RE::TESBoundObject* GetSwapBase(const RE::TESObjectREFR* a_ref) const;

		FormIDOrSet formIDSet{};
		Transform transform{};
		Traits traits{};
	};
}
