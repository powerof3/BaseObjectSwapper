#include "ObjectProperties.h"

RandValueParams::RandValueParams(const RNGParams& a_chance, const RE::TESObjectREFR* a_ref, std::uint64_t a_entryHash) :
	rng(a_chance),
	ref(a_ref),
	entryHash(a_entryHash)
{}

float RandValueParams::Generate(float a_min, float a_max)
{
	rng.Seed(ref, entryHash);
	return rng.Generate(a_min, a_max);
}

FloatRange::FloatRange(const std::string& a_str)
{
	const auto splitRange = REX::STR::SPLIT(a_str, R"(/)");
	min = REX::STR::TO_NUM<float>(splitRange[0]);
	max = splitRange.size() > 1 ? REX::STR::TO_NUM<float>(splitRange[1]) : min;
}

bool FloatRange::operator==(const FloatRange& a_rhs) const
{
	return (min == a_rhs.min && max == a_rhs.max);
}

bool FloatRange::operator!=(const FloatRange& a_rhs) const
{
	return !operator==(a_rhs);
}

bool FloatRange::is_exact() const
{
	return min == max;
}

void FloatRange::convert_to_radians()
{
	min = RE::deg_to_rad(min);
	max = RE::deg_to_rad(max);
}

float FloatRange::GetRandomValue(RandValueParams& a_params) const
{
	float value = is_exact() ? min : a_params.Generate(min, max);

	if (a_params.clamp) {
		value = std::clamp(value, a_params.clampMin, a_params.clampMax);
	}

	return value;
}

ScaleRange::ScaleRange(const std::string& a_str) :
	absolute(a_str.contains('A'))
{
	if (boost::cmatch match; boost::regex_search(a_str.c_str(), match, regex::generic)) {
		value = FloatRange(match[1].str());
	}
}

void ScaleRange::SetScale(RE::TESObjectREFR* a_ref, RandValueParams& a_params) const
{
	if (absolute) {
		a_ref->refScale = static_cast<std::uint16_t>(value.GetRandomValue(a_params));
	} else {
		a_ref->refScale = static_cast<std::uint16_t>(a_ref->refScale * value.GetRandomValue(a_params));
	}
}

Point3Range::Point3Range(const std::string& a_str, bool a_convertToRad) :
	relative(a_str.contains('R'))
{
	if (boost::cmatch match; boost::regex_search(a_str.c_str(), match, regex::transform)) {
		//match[0] gets the whole string
		x = FloatRange(match[1].str());
		y = FloatRange(match[2].str());
		z = FloatRange(match[3].str());
		if (a_convertToRad) {
			x.convert_to_radians();
			y.convert_to_radians();
			z.convert_to_radians();
		}
	}
}

RE::NiPoint3 Point3Range::min() const
{
	return RE::NiPoint3(x.min, y.min, z.min);
}

RE::NiPoint3 Point3Range::max() const
{
	return RE::NiPoint3(x.max, y.max, z.max);
}

bool Point3Range::is_exact() const
{
	return min() == max();
}

RE::NiPoint3 Point3Range::GetRandomValue(RandValueParams& a_params) const
{
	if (is_exact()) {
		return min();
	}

	return RE::NiPoint3{ x.GetRandomValue(a_params), y.GetRandomValue(a_params), z.GetRandomValue(a_params) };
}

void Point3Range::SetTransform(RE::NiPoint3& a_inPoint, RandValueParams& a_params) const
{
	if (relative) {
		a_inPoint += GetRandomValue(a_params);
	} else {
		a_inPoint = GetRandomValue(a_params);
	}
}

void ObjectProperties::assign_record_flags(const std::string& a_str, bool a_unsetFlag)
{
	auto& flags = a_unsetFlag ? recordFlagsUnset : recordFlagsSet;

	if (boost::cmatch match; boost::regex_search(a_str.c_str(), match, regex::generic)) {
		for (const auto& str : REX::STR::SPLIT(match[1].str(), ",")) {
			flags |= REX::STR::TO_NUM<std::uint32_t>(str, true);
		}
	}
}

ObjectProperties::ObjectProperties(const std::string& a_str)
{
	if (distribution::is_valid_entry(a_str)) {
		constexpr auto split_with_regex = [](const std::string& a_str, const boost::regex& a_regex) -> std::vector<std::string> {
			boost::sregex_token_iterator iter(a_str.begin(),
				a_str.end(),
				a_regex,
				-1);
			boost::sregex_token_iterator end{};
			return { iter, end };
		};

		auto split_properties = split_with_regex(a_str, regex::string);
		for (const auto& propStr : split_properties) {
			if (propStr.contains("pos")) {
				location = Point3Range(propStr);
			} else if (propStr.contains("rot")) {
				rotation = Point3Range(propStr, true);
			} else if (propStr.contains("scale")) {
				refScale = ScaleRange(propStr);
			} else if (propStr.contains("flags")) {
				assign_record_flags(propStr, propStr.contains("C"));
			}
		}
	}
}

bool ObjectProperties::IsValid() const
{
	return location || rotation || refScale || recordFlagsSet != 0 || recordFlagsUnset != 0;
}

bool ObjectProperties::ChangesXYLocation() const
{
	return location && (!location->relative || location->x != FloatRange{} || location->y != FloatRange{});
}

bool ObjectProperties::ChangesRecordFlag(std::uint32_t a_flag) const
{
	return ((recordFlagsSet | recordFlagsUnset) & a_flag) != 0;
}

void ObjectProperties::SetTransform(RE::TESObjectREFR* a_refr, const RNGParams& a_chance, std::uint64_t a_propertiesHash) const
{
	if (location || rotation || refScale) {
		RandValueParams params(a_chance, a_refr, a_propertiesHash);
		if (location) {
			location->SetTransform(a_refr->data.location, params);
		}
		if (rotation) {
			params.clamp = true;
			params.clampMin = -RE::NI_TWO_PI;
			params.clampMax = RE::NI_TWO_PI;
			rotation->SetTransform(a_refr->data.angle, params);
		}
		if (refScale) {
			params.clamp = true;
			params.clampMin = 0.0f;
			params.clampMax = 1000.0f;
			refScale->SetScale(a_refr, params);
		}
	}
}

void ObjectProperties::SetRecordFlags(RE::TESObjectREFR* a_refr) const
{
	if (recordFlagsUnset != 0) {
		a_refr->formFlags &= ~recordFlagsUnset;
	}

	if (recordFlagsSet != 0) {
		a_refr->formFlags |= recordFlagsSet;
	}
}
