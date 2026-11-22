#pragma once

#include "SwapData.h"

namespace FormSwap
{
	struct FormEntries
	{
		void Sort();

		// members
		SwapFormDataConditional conditionalSwaps;       // [Forms|...]
		SwapFormDataVec         formSwaps;              // [Forms]
		ObjectDataVec           properties;             // [Properties]
		ObjectDataConditional   conditionalProperties;  // [Properties|...]
	};

	struct RefEntries
	{
		void Sort();

		// members
		SwapFormDataVec         refSwaps;               // [References]
		ObjectDataVec           properties;             // [Properties]
	};

	class Manager :
		public REX::TSingleton<Manager>,
		public RE::BSTEventSink<RE::TESFormDeleteEvent>
	{
	public:
		void Register();

		void LoadFormsOnce();

		void LogFormSwaps() const;
		void LogConflicts(const std::vector<std::string>& a_configs, const std::vector<std::string>& a_records);
		void PrintConflicts() const;

		SwapFormResult GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

		void InsertLeveledItemRef(const RE::TESObjectREFR* a_refr);
		bool IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const;

	private:
		void LoadForms();

		[[nodiscard]] const FormEntries* GetBaseEntries(RE::FormID a_formID) const;
		[[nodiscard]] const RefEntries*  GetRefEntries(RE::FormID a_formID) const;

		static SwapFormResult    GetSwapFormConditional(const FormEntries& a_baseEntries, const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, std::optional<ConditionalInput>& a_input);
		static const ObjectData* GetObjectPropertiesConditional(const FormEntries& a_baseEntries, const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, std::optional<ConditionalInput>& a_input);

		RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override;

		// members
		FormIDMap<FormEntries>    baseEntryMap;
		FormIDMap<RefEntries>     refEntryMap;
		IndexedProperties         indexedProperties;
		ConcurrentSet<RE::FormID> swappedLeveledItemRefs{};
		std::once_flag            init{};
		std::atomic<bool>         hasLeveledItemRefs{};
		bool                      hasConflicts{ false };
	};
}
