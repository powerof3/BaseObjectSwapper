#pragma once

#include "ConditionalData.h"
#include "ObjectProperties.h"

namespace FormSwap
{
	class ObjectData
	{
	public:
		struct Input
		{
			std::string properties;
			std::string chance;
			std::string record;
			std::string path;
		};

		ObjectData() = delete;
		explicit ObjectData(const Input& a_input);

		bool        HasValidProperties(const RE::TESObjectREFR* a_ref) const;
		static void GetProperties(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, ObjectData&)> a_func);

		// members
		ObjectProperties properties{};
		Chance           chance{};

		// used for logging conflicts
		std::string record{};
		std::string path{};
	};

	class SwapFormData : public ObjectData
	{
	public:
		SwapFormData() = delete;
		SwapFormData(FormIDOrSet a_id, const Input& a_input);

		RE::TESBoundObject* GetSwapBase(const RE::TESObjectREFR* a_ref) const;
		static void         GetForms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, SwapFormData&)> a_func);

		// members
		FormIDOrSet formIDSet{};
	};

	using ObjectDataVec = std::vector<ObjectData>;
	using ObjectDataConditional = ConditionalData<ObjectData>;

	using SwapFormDataVec = std::vector<SwapFormData>;
	using SwapFormDataConditional = ConditionalData<SwapFormData>;

	using SwapFormResult = std::pair<RE::TESBoundObject*, std::optional<ObjectProperties>>;
}
