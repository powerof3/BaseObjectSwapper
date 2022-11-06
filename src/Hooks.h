#pragma once

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
		static inline constexpr std::size_t size = 0x13;

		static void Install()
		{
			stl::write_vfunc<T, InitItemImpl>();
			logger::info("Installed {} form swap"sv, typeid(T).name());
		}
	};

	void Install();
}
