#include "SwapData.h"

namespace FormSwap
{
	namespace detail
	{
		static Transform::minMax<float> get_min_max(const std::string& a_str)
		{
			constexpr auto get_float = [](const std::string& str) {
				return string::lexical_cast<float>(str);
			};

			if (const auto splitNum = string::split(a_str, R"(/)"); splitNum.size() > 1) {
			    return { get_float(splitNum[0]), get_float(splitNum[1]) };
			} else {
				auto num = get_float(splitNum[0]);
				return { num, num };
			}
		}

		static float get_random_value(float a_min, float a_max)
		{
			if (a_min == a_max) {
				return a_min;
			}

			return stl::RNG::GetSingleton()->Generate(a_min, a_max);
		}

		static RE::NiPoint3 get_random_value(const RE::NiPoint3& a_min, const RE::NiPoint3& a_max)
		{
			if (a_min == a_max) {
				return a_min;
			}

			return { get_random_value(a_min.x, a_max.x), get_random_value(a_min.y, a_max.y), get_random_value(a_min.z, a_max.z) };
		}
	}

	Transform::relData<RE::NiPoint3> Transform::get_transform_from_string(const std::string& a_str)
    {
		bool relative = a_str.contains('R');
		minMax<RE::NiPoint3> transformData;

		srell::cmatch match;
		if (srell::regex_search(a_str.c_str(), match, transformRegex)) {
			const auto [minX, maxX] = detail::get_min_max(match[1].str());  //match[0] gets the whole string
			transformData.first.x = minX;
			transformData.second.x = maxX;

			const auto [minY, maxY] = detail::get_min_max(match[2].str());
			transformData.first.y = minY;
			transformData.second.y = maxY;

			const auto [minZ, maxZ] = detail::get_min_max(match[2].str());
			transformData.first.z = minZ;
			transformData.second.z = maxZ;
		}

		return std::make_pair(relative, transformData);
	}

	std::optional<Transform::minMax<float>> Transform::get_scale_from_string(const std::string& a_str)
    {
		minMax<float> scale{ 1.0f, 1.0f };

		srell::cmatch match;
		if (srell::regex_search(a_str.c_str(), match, scaleRegex)) {
			scale = detail::get_min_max(match[1].str());
		}

		return scale;
	}

	Transform::Transform(const std::string& a_str)
	{
		if (!a_str.empty()) {
			const auto transformStrs = string::split(a_str, ",");
			for (auto& transformStr : transformStrs) {
				if (transformStr.contains("pos")) {
					location = get_transform_from_string(transformStr);
				} else if (transformStr.contains("rot")) {
					rotation = get_transform_from_string(transformStr);
				} else if (transformStr.contains("scale")) {
					refScale = get_scale_from_string(transformStr);
				}
			}
		}
	}

	void Transform::SetTransform(RE::TESObjectREFR* a_refr)
	{
		if (location) {
			auto [relative, minMax] = *location;
			auto [min, max] = minMax;
			a_refr->data.location = relative ? a_refr->data.location + detail::get_random_value(min, max) : detail::get_random_value(min, max);
		}
		if (rotation) {
			auto [relative, minMax] = *location;
			auto [min, max] = minMax;
			a_refr->data.angle = relative ? a_refr->data.angle + detail::get_random_value(min, max) : detail::get_random_value(min, max);
		}
		if (refScale) {
			auto& [min, max] = *refScale;
			const auto scale = std::clamp(detail::get_random_value(min, max), 0.0f, 1000.0f);
			a_refr->refScale = static_cast<std::uint16_t>(a_refr->refScale * scale);
		}
	}

	SwapData::SwapData(RE::FormID a_id, Transform a_transform) :
		formID(a_id),
		transform(std::move(a_transform))
	{}

	RE::FormID SwapData::GetFormID(const std::string& a_str)
	{
		if (a_str.find('~') != std::string::npos) {
			const auto formPair = string::split(a_str, "~");

			const auto [modName, formID] = MergeMapper::GetNewFormID(formPair[1], formPair[0]);

			return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
			return form->GetFormID();
		}

		return static_cast<RE::FormID>(0);
	}
}
