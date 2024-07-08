#pragma once

#include "RNG.h"

struct RandValueParams
{
	RandValueParams(Chance a_chance, const RE::TESObjectREFR* a_ref);

	BOS_RNG rng{};
	bool    clamp{ false };
	float   clampMin{ 0.0f };
	float   clampMax{ 0.0f };
};

struct FloatRange
{
public:
	FloatRange() = default;
	FloatRange(const std::string& a_str);

	bool operator==(const FloatRange& a_rhs) const;
	bool operator!=(const FloatRange& a_rhs) const;

	bool is_exact() const;
	void convert_to_radians();

	float GetRandomValue(const RandValueParams& a_params) const;

	// members
	float min{};
	float max{};
};

struct ScaleRange
{
public:
	ScaleRange() = default;
	ScaleRange(const std::string& a_str);

	void SetScale(RE::TESObjectREFR* a_ref, const RandValueParams& a_params) const;

	// members
	bool       absolute{ false };
	FloatRange value{};
};

struct Point3Range
{
public:
	Point3Range() = default;
	Point3Range(const std::string& a_str, bool a_convertToRad = false);

	RE::NiPoint3 min() const;
	RE::NiPoint3 max() const;
	bool         is_exact() const;

	RE::NiPoint3 GetRandomValue(const RandValueParams& a_params) const;
	void         SetTransform(RE::NiPoint3& a_inPoint, const RandValueParams& a_params) const;

	// members
	bool       relative;
	FloatRange x;
	FloatRange y;
	FloatRange z;
};

class ObjectProperties
{
public:
	ObjectProperties() = default;
	explicit ObjectProperties(const std::string& a_str);

	bool IsValid() const;

	void SetChance(Chance a_chance);
	void SetTransform(RE::TESObjectREFR* a_refr) const;
	void SetRecordFlags(RE::TESObjectREFR* a_refr) const;

private:
	void assign_record_flags(const std::string& a_str, bool a_unsetFlag);

	// members
	Chance chance{};

	std::optional<Point3Range> location{ std::nullopt };
	std::optional<Point3Range> rotation{ std::nullopt };
	std::optional<ScaleRange>  refScale{ std::nullopt };

	std::uint32_t recordFlagsSet{ 0 };
	std::uint32_t recordFlagsUnset{ 0 };
};
