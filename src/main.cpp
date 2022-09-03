#include "Manager.h"
#include "MergeMapper.h"

namespace FormSwap
{
	struct InitItemImpl
	{
		static void thunk(RE::TESObjectREFR* a_ref)
		{
			if (const auto base = a_ref->GetBaseObject(); base) {
				Manager::GetSingleton()->LoadFormsOnce();
		
				auto [swapBase, transformData] = Manager::GetSingleton()->GetSwapData(a_ref, base);

				if (swapBase) {								
					if (base != swapBase) {
						a_ref->SetObjectReference(swapBase);
						transformData.SetTransform(a_ref);
					}
				}
			}

			func(a_ref);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static inline constexpr std::size_t size = 0x13;
	};

	inline void Install()
	{
		stl::write_vfunc<RE::TESObjectREFR, InitItemImpl>();

		logger::info("Installed form swap"sv);
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
		FormSwap::Manager::GetSingleton()->PrintConflicts();
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Base Object Swapper");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Base Object Swapper";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver
#	ifndef SKYRIMVR
		< SKSE::RUNTIME_1_5_39
#	else
		> SKSE::RUNTIME_VR_1_4_15_1
#	endif
	) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	logger::info("{:*^30}", "MERGES");

	MergeMapper::GetMerges();

	logger::info("{:*^30}", "HOOKS");

	FormSwap::Install();

	/*const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);*/ 

	return true;
}
