#pragma once
#include <string>

namespace RetroRenderer
{
	enum class EventType
	{
		Window_Resize,
		Scene_Load,
		Scene_Reset
	};

	static constexpr const char* EventTypeToString(EventType type)
	{
		switch (type)
		{
		case EventType::Window_Resize:
			return "Window_Resize";
		case EventType::Scene_Load:
			return "Scene_Load";
		case EventType::Scene_Reset:
			return "Scene_Reset";
		default:
			return "Unknown";
		}
	}

	struct Event
	{
		EventType type;
		bool handled = false;
	};

	struct WindowResizeEvent : public Event
	{
		Event base{ EventType::Window_Resize };
		uint32_t width;
		uint32_t height;
	};

	struct SceneLoadEvent : public Event
	{
		std::string scenePath;

		SceneLoadEvent(std::string path)
		{
			type = EventType::Scene_Load;
			scenePath = path;
		}
	};

	struct SceneResetEvent : public Event
	{
		SceneResetEvent()
		{
			type = EventType::Scene_Reset;
		}
	};
}