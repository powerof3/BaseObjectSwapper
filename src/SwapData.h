#pragma once

#include "ConditionalData.h"
#include "ObjectProperties.h"

namespace FormSwap
{
	class IndexedProperties
	{
	public:
		const ObjectProperties* GetOrCreate(const std::string& a_str);
		void                    ReleaseIndex();

	private:
		std::vector<std::unique_ptr<const ObjectProperties>> storage;
		StringMap<const ObjectProperties*>                   index;
	};

	class ObjectData
	{
	public:
		struct Input
		{
			[[nodiscard]] std::uint64_t GenerateHash() const;

			std::string properties{};
			std::string chance{};
			std::string record{};
		};

		ObjectData() = delete;
		ObjectData(const Input& a_input, IndexedProperties& a_properties);

		[[nodiscard]] bool          HasValidProperties(const RE::TESObjectREFR* a_ref) const;
		[[nodiscard]] std::uint64_t GetPropertiesHash() const;
		void                        ApplyProperties(RE::TESObjectREFR* a_ref) const;

		static void GetProperties(const std::string& a_str, IndexedProperties& a_properties, bool a_resolveForm, std::function<void(RE::FormID, const RE::TESForm*, ObjectData&&)> a_func);

		// members
		const ObjectProperties* properties{ nullptr };
		RNGParams               chance{};
		std::uint64_t           entryHash{ 0 };
		std::uint32_t           fileIndex{ 0 };
		std::uint32_t           recordIndex{ 0 };  // used for logging conflicts
	};

	using SwapTargets = std::variant<RE::TESBoundObject*, std::vector<RE::TESBoundObject*>>;

	class SwapFormData : public ObjectData
	{
	public:
		SwapFormData() = delete;
		SwapFormData(const FormIDOrSet& a_id, const Input& a_input, IndexedProperties& a_properties);
		SwapFormData(const ObjectData& a_objectData, const FormIDOrSet& a_id);

		RE::TESBoundObject* GetSwapBase(const RE::TESObjectREFR* a_ref) const;
		static void         GetForms(const std::string& a_str, IndexedProperties& a_properties, std::function<void(RE::FormID, SwapFormData&&)> a_func);

		SwapTargets targets{};

	private:
		static SwapTargets make_targets(const FormIDOrSet& a_id);
	};

	struct SwapFormResult
	{
		RE::TESBoundObject* swapBase{ nullptr };
		const ObjectData*   propertySource{ nullptr };
	};

	using ObjectDataVec = std::vector<ObjectData>;
	using ObjectDataConditional = ConditionalData<ObjectData>;

	using SwapFormDataVec = std::vector<SwapFormData>;
	using SwapFormDataConditional = ConditionalData<SwapFormData>;
}
