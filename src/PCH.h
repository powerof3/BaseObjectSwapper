#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ranges>

#include "RE/Skyrim.h"
#include "REX/REX.h"
#include "SKSE/SKSE.h"

#include <MergeMapperPluginAPI.h>

#include <boost/regex.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <spdlog/sinks/basic_file_sink.h>

#include <CLibUtil/distribution.hpp>
#include <ClibUtil/editorID.hpp>

#include <SimpleIni.h>
#undef ERROR

namespace distribution = clib_util::distribution;
namespace editorID = clib_util::editorID;
using namespace std::literals;

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

using FormIDStr = std::variant<RE::FormID, std::string>;

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using Map = boost::unordered_flat_map<K, D, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using Set = boost::unordered_flat_set<K, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using ConcurrentSet = boost::concurrent_flat_set<K, H, KEqual>;

template <class T>
using OrderedSet = std::set<T>;

using FormIDSet = std::vector<RE::FormID>;
using FormIDOrSet = std::variant<RE::FormID, FormIDSet>;
using FormIDOrderedSet = OrderedSet<RE::FormID>;

template <class T>
using FormIDMap = Map<RE::FormID, T>;

namespace stl
{
	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
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

#include "RE.h"
#include "Version.h"
