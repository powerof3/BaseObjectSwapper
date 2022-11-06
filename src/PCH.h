#pragma once

#define WIN32_LEAN_AND_MEAN

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <MergeMapperPluginAPI.h>
#include <SimpleIni.h>

#include <CLibUtil/config.hpp>
#include <CLibUtil/numeric.hpp>
#include <CLibUtil/rng.hpp>
#include <CLibUtil/string.hpp>

#include <ranges>
#include <robin_hood.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <srell.hpp>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = clib_util::string;

using namespace std::literals;

using SeedRNG = clib_util::RNG;

namespace stl
{
	using namespace SKSE::stl;
	using namespace clib_util;

	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}
}

#include "Version.h"
