#pragma once

#include "SwapData.h"

namespace FormSwap
{
	template <class T>
	using SwapMap = Map<RE::FormID, T>;

	using SwapDataVec = std::vector<SwapData>;
	using TransformDataVec = std::vector<TransformData>;

	using SwapDataConditional = Map<FormIDStr, SwapDataVec>;
	using TransformDataConditional = Map<FormIDStr, TransformDataVec>;

	using ConditionalInput = std::tuple<const RE::TESObjectREFR*, const RE::TESForm*, RE::TESObjectCELL*, RE::BGSLocation*>;

	using TransformResult = std::optional<Transform>;
    using SwapResult = std::pair<RE::TESBoundObject*, TransformResult>;

	class Manager
	{
	public:
		[[nodiscard]] static Manager* GetSingleton()
		{
			static Manager singleton;
			return std::addressof(singleton);
		}

		void LoadFormsOnce();

		void PrintConflicts() const;

		SwapResult GetSwapData(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

		SwapResult GetSwapConditionalBase(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);
		TransformResult GetTransformConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base);

	protected:
		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		~Manager() = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

	private:
		void LoadForms();

	    SwapMap<SwapDataVec>& get_form_map(const std::string& a_str);

        void get_forms(const std::string& a_path, const std::string& a_str, SwapMap<SwapDataVec>& a_map);
		void get_forms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);

		void get_transforms(const std::string& a_path, const std::string& a_str);
		void get_transforms(const std::string& a_path, const std::string& a_str, const std::vector<FormIDStr>& a_conditionalIDs);

        [[nodiscard]] bool get_conditional_result(const FormIDStr& a_data, const ConditionalInput& a_input) const;

		SwapMap<SwapDataVec> swapForms{};
		SwapMap<SwapDataVec> swapRefs{};
		SwapMap<SwapDataConditional> swapFormsConditional{};

		SwapMap<TransformDataVec> transforms{};
		SwapMap<TransformDataConditional> transformsConditional{};

		bool hasConflicts{ false };
		std::once_flag init{};
	};
}
