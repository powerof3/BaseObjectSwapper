#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ranges>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <MergeMapperPluginAPI.h>

#include <ankerl/unordered_dense.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <srell.hpp>

#include <CLibUtil/distribution.hpp>
#include <CLibUtil/hash.hpp>
#include <CLibUtil/numeric.hpp>
#include <CLibUtil/rng.hpp>
#include <CLibUtil/string.hpp>
#include <ClibUtil/simpleINI.hpp>
#include <ClibUtil/singleton.hpp>
#include <ClibUtil/editorID.hpp>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
using namespace std::literals;

using namespace clib_util;
using namespace clib_util::singleton;
using SeedRNG = clib_util::RNG;

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

using FormIDStr = std::variant<RE::FormID, std::string>;

template <class K, class D>
using Map = ankerl::unordered_dense::map<K, D>;
template <class T>
using Set = ankerl::unordered_dense::set<T>;

using FormIDSet = Set<RE::FormID>;
using FormIDOrSet = std::variant<RE::FormID, FormIDSet>;

template <class T>
using FormIDMap = Map<RE::FormID, T>;

namespace stl
{
	using namespace SKSE::stl;

	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

#include "Util.h"
#include "Version.h"
