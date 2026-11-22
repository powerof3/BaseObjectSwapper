#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ranges>

#include "RE/Skyrim.h"
#include "REX/REX.h"
#include "SKSE/SKSE.h"

#include <MergeMapperPluginAPI.h>

#include <boost/regex.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/unordered_node_map.hpp>
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
using FormIDFormMap = std::map<RE::FormID, RE::TESForm*>;

template <class T>
using FormIDMap = Map<RE::FormID, T>;

struct string_hash
{
	using is_transparent = void;

	std::size_t operator()(const char* str) const
	{
		return boost::hash<std::string_view>{}(str);
	}

	std::size_t operator()(std::string_view str) const
	{
		return boost::hash<std::string_view>{}(str);
	}

	std::size_t operator()(const std::string& str) const
	{
		return boost::hash<std::string>{}(str);
	}
};

template <class D>
using StringMap = Map<std::string, D, string_hash, std::equal_to<>>;

template <class D>
using StringNodeMap = boost::unordered_node_map<std::string, D, string_hash, std::equal_to<>>;

template <class K, class D>
class InsertionMap
{
public:
	D& operator[](const K& a_key)
	{
		if (!_map.empty() && _map.back().first == a_key) {
			return _map.back().second;
		}
		return _map.emplace_back(std::piecewise_construct, std::forward_as_tuple(a_key), std::forward_as_tuple()).second;
	}

	template <class Comp>
	void sort(Comp&& a_comp)
	{
		std::ranges::stable_sort(_map, std::forward<Comp>(a_comp));
	}

	void shrink_to_fit()
	{
		for (auto& value : _map | std::views::values) {
			value.shrink_to_fit();
		}
		_map.shrink_to_fit();
	}

	[[nodiscard]] auto begin() { return _map.begin(); }
	[[nodiscard]] auto end() { return _map.end(); }
	[[nodiscard]] auto begin() const { return _map.begin(); }
	[[nodiscard]] auto end() const { return _map.end(); }

	[[nodiscard]] bool        empty() const { return _map.empty(); }
	[[nodiscard]] std::size_t size() const { return _map.size(); }

private:
	std::vector<std::pair<K, D>> _map{};
};

namespace stl
{
	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
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
