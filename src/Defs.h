#pragma once

template <class T>
using minMax = std::pair<T, T>;
template <class T>
using relData = std::pair<bool, minMax<T>>;  //relative vs absolute

template <class K, class D>
using Map = ankerl::unordered_dense::map<K, D>;
template <class T>
using Set = ankerl::unordered_dense::set<T>;

using FormIDStr = std::variant<RE::FormID, std::string>;

using FormIDSet = Set<RE::FormID>;
using FormIDOrSet = std::variant<RE::FormID, FormIDSet>;

template <class T>
using SwapMap = Map<RE::FormID, T>;
