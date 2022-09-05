#pragma once

namespace FormSwap
{
	struct Transform
	{
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

	class SwapData
	{
	public:
		SwapData() = delete;
		SwapData(RE::FormID a_id, Transform a_transform);

		[[nodiscard]] static RE::FormID GetFormID(const std::string& a_str);

		RE::FormID formID{};
		Transform transform{};
	};
}
