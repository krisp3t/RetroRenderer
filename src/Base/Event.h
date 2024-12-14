#pragma once
#include <string>
#include <glm/glm.hpp>

namespace RetroRenderer
{
	enum class EventType
	{
		Output_Image_Resize,
		Scene_Load,
		Scene_Reset
	};

	static constexpr const char* EventTypeToString(EventType type)
	{
		switch (type)
		{
		case EventType::Output_Image_Resize:
			return "Output_Image_Resize";
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

	struct OutputImageResizeEvent : public Event
	{
		glm::ivec2 resolution;

		OutputImageResizeEvent(glm::ivec2 res)
		{
			type = EventType::Output_Image_Resize;
			resolution = res;
		}
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