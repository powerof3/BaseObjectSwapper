#include "FormSwapManager.h"

namespace FormSwap
{
	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			const auto base = a_ref->GetBaseObject();

			auto replaceBaseData = base && !a_ref->IsDynamicForm() ? FormSwapManager::GetSingleton()->GetSwapRef(a_ref) : std::make_pair(nullptr, SWAP_FLAGS::kNone);
			if (!replaceBaseData.first) {
				replaceBaseData = base && !base->IsDynamicForm() ? FormSwapManager::GetSingleton()->GetSwapForm(base) : std::make_pair(nullptr, SWAP_FLAGS::kNone);
			}

			auto& [replaceBase, flags] = replaceBaseData;
			if (replaceBase && base != replaceBase) {
				a_ref->SetObjectReference(replaceBase);
				a_ref->ResetInventory(false);
			}

			const auto node = func(a_ref, a_backgroundLoading);
			if (node && flags.all(SWAP_FLAGS::kApplyMaterialShader)) {
				const auto stat = base ? base->As<RE::TESObjectSTAT>() : nullptr;
				if (const auto shader = stat ? stat->data.materialObj : nullptr; shader) {
					const auto projectedUVParams = RE::NiColorA{
						shader->directionalData.falloffScale,
						shader->directionalData.falloffBias,
						1.0f / shader->directionalData.noiseUVScale,
						std::cosf(RE::deg_to_rad(stat->data.materialThresholdAngle))
					};

					node->SetProjectedUVData(
						projectedUVParams,
						shader->directionalData.singlePassColor,
						shader->directionalData.flags.any(RE::BSMaterialObject::DIRECTIONAL_DATA::Flag::kSnow));
				}
			}

			return node;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static inline constexpr std::size_t size = 0x6A;
	};

	inline void Install()
	{
		stl::write_vfunc<RE::TESObjectREFR, Load3D>();
		logger::info("Installed form swap"sv);
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
		Cache::EditorID::GetSingleton()->GetEditorIDs();

		FormSwapManager::GetSingleton()->LoadForms();
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Base Object Swapper"sv);
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	logger::info("{:*^30}", "INI");

	FormSwapManager::GetSingleton()->LoadINI();

	logger::info("{:*^30}", "HOOKS");

	FormSwap::Install();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
