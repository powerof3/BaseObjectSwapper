#pragma once

namespace Cache
{
	class EditorID
	{
	public:
		static EditorID* GetSingleton()
		{
			static EditorID singleton;
			return std::addressof(singleton);
		}

		void GetEditorIDs()
		{
			const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
			const RE::BSReadLockGuard locker{ lock };
			if (map) {
				for (auto& [id, form] : *map) {
					if (form->IsBoundObject() || form->Is(RE::FormType::Reference, RE::FormType::ActorCharacter)) {
						_formIDToEditorIDMap.emplace(form->GetFormID(), id.c_str());
						_editorIDToFormIDMap.emplace(id.c_str(), form->GetFormID());
					}
				}
			}
		}

		std::string GetEditorID(RE::FormID a_formID)
		{
			const auto it = _formIDToEditorIDMap.find(a_formID);
			return it != _formIDToEditorIDMap.end() ? it->second : std::string();
		}

		RE::FormID GetFormID(const std::string& a_editorID)
		{
			const auto it = _editorIDToFormIDMap.find(a_editorID);
			return it != _editorIDToFormIDMap.end() ? it->second : 0;
		}

	protected:
		using Lock = std::mutex;
		using Locker = std::scoped_lock<Lock>;

		EditorID() = default;
		EditorID(const EditorID&) = delete;
		EditorID(EditorID&&) = delete;
		~EditorID() = default;

		EditorID& operator=(const EditorID&) = delete;
		EditorID& operator=(EditorID&&) = delete;

	private:
		robin_hood::unordered_flat_map<RE::FormID, std::string> _formIDToEditorIDMap;
		robin_hood::unordered_flat_map<std::string, RE::FormID> _editorIDToFormIDMap;
	};
}
