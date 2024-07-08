#include "ObjectProperties.h"

RandValueParams::RandValueParams(Chance a_chance, const RE::TESObjectREFR* a_ref) :
	rng(a_chance, a_ref)
{}

FloatRange::FloatRange(const std::string& a_str)
{
	const auto splitRange = string::split(a_str, R"(/)");
	min = string::to_num<float>(splitRange[0]);
	max = splitRange.size() > 1 ? string::to_num<float>(splitRange[1]) : min;
}

bool FloatRange::operator==(const FloatRange& a_rhs) const
{
	return (min == a_rhs.min && max == a_rhs.min);
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

float FloatRange::GetRandomValue(const RandValueParams& a_params) const
{
	float value = is_exact() ? min : a_params.rng.generate(min, max);

	if (a_params.clamp) {
		value = std::clamp(value, a_params.clampMin, a_params.clampMax);
	}

	return value;
}

ScaleRange::ScaleRange(const std::string& a_str) :
	absolute(a_str.contains('A'))
{
	if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, regex::generic)) {
		value = FloatRange(match[1].str());
	}
}

void ScaleRange::SetScale(RE::TESObjectREFR* a_ref, const RandValueParams& a_params) const
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
	if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, regex::transform)) {
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

RE::NiPoint3 Point3Range::GetRandomValue(const RandValueParams& a_params) const
{
	return is_exact() ? min() : RE::NiPoint3{ x.GetRandomValue(a_params), y.GetRandomValue(a_params), z.GetRandomValue(a_params) };
}

void Point3Range::SetTransform(RE::NiPoint3& a_inPoint, const RandValueParams& a_params) const
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

	if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, regex::generic)) {
		for (const auto& str : string::split(match[1].str(), ",")) {
			flags |= string::to_num<std::uint32_t>(str, true);
		}
	}
}

ObjectProperties::ObjectProperties(const std::string& a_str)
{
	if (distribution::is_valid_entry(a_str)) {
		auto split_properties = util::split_with_regex(a_str, regex::string);
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

void ObjectProperties::SetChance(Chance a_chance)
{
	chance = a_chance;
}

void ObjectProperties::SetTransform(RE::TESObjectREFR* a_refr) const
{
	if (location || rotation || refScale) {
		RandValueParams params(chance, a_refr);
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
