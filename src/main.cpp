#include "FormSwapManager.h"

namespace FormSwap
{
	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			const auto base = a_ref->GetBaseObject();

		    auto replaceBase = base && !a_ref->IsDynamicForm() ? FormSwapManager::GetSingleton()->GetSwapRef(a_ref) : std::make_pair(nullptr, SWAP_FLAGS::kNone);
			if (!replaceBase.first) {
				replaceBase = base && !base->IsDynamicForm() ? FormSwapManager::GetSingleton()->GetSwapForm(base) : std::make_pair(nullptr, SWAP_FLAGS::kNone);
			}

			if (replaceBase.first && base != replaceBase.first) {
				a_ref->SetObjectReference(replaceBase.first);
				a_ref->ResetInventory(false);
			}

			const auto node = func(a_ref, a_backgroundLoading);

		    if (node && replaceBase.second.any(SWAP_FLAGS::kApplyMaterialShader)) {
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

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Base Object Swapper";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	logger::info("{:*^30}", "INI");

	FormSwapManager::GetSingleton()->LoadINI();

	logger::info("{:*^30}", "HOOKS");

	FormSwap::Install();

    const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
