#pragma once

#include "Manager.h"

namespace BaseObjectSwapper
{
	namespace detail
	{
		void swap_base(RE::TESObjectREFR* a_ref);
	}

	template <class T>
	struct InitItemImpl
	{
		static void thunk(T* a_ref)
		{
			detail::swap_base(a_ref);

			func(a_ref);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            size{ 0x13 };

		static void Install()
		{
			stl::write_vfunc<T, InitItemImpl>();
			logger::info("Installed {} form swap"sv, typeid(T).name());
		}
	};

	template <class T>
	struct SetObjectReference
	{
		static void thunk(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_object)
		{
			if (FormSwap::Manager::GetSingleton()->IsLeveledItemRefSwapped(a_ref)) {
				return;
			}

			func(a_ref, a_object);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            size{ 0x84 };

		static void Install()
		{
			stl::write_vfunc<T, SetObjectReference>();
		}
	};

	void Install();
}
