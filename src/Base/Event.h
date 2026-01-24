#pragma once
#include <glm/glm.hpp>
#include <string>
#include <utility>

namespace RetroRenderer {
enum class EventType {
    Output_Image_Resize,
    Window_Resize,
    Scene_Load,
    Scene_Reset,
    Texture_Load,
};

static constexpr const char *EventTypeToString(EventType type) {
    switch (type) {
    case EventType::Output_Image_Resize:
        return "Output_Image_Resize";
    case EventType::Window_Resize:
        return "Window_Resize";
    case EventType::Scene_Load:
        return "Scene_Load";
    case EventType::Texture_Load:
        return "Texture_Load";
    case EventType::Scene_Reset:
        return "Scene_Reset";
    default:
        return "Unknown";
    }
}

struct Event {
    EventType type;
    bool handled = false;
};

struct OutputImageResizeEvent : public Event {
    glm::ivec2 resolution;

    OutputImageResizeEvent(glm::ivec2 res) {
        type = EventType::Output_Image_Resize;
        resolution = res;
    }
};
struct WindowResizeEvent : public Event {
    glm::ivec2 resolution;

    WindowResizeEvent(glm::ivec2 res) {
        type = EventType::Window_Resize;
        resolution = res;
    }
};
struct TextureLoadEvent : public Event {
    std::string texturePath;
    std::vector<uint8_t> textureDataBuffer;
    size_t textureDataSize = 0;
    bool loadFromMemory = false;
    TextureLoadEvent(std::vector<uint8_t> buffer, size_t size) {
        type = EventType::Texture_Load;
        textureDataBuffer = buffer;
        textureDataSize = size;
        loadFromMemory = true;
    }
};

struct SceneLoadEvent : public Event {
    std::string scenePath;
    const uint8_t *sceneData = nullptr;
    std::vector<uint8_t> sceneDataBuffer;
    size_t sceneDataSize = 0;
    bool loadFromMemory = false;

    SceneLoadEvent(std::string path) {
        type = EventType::Scene_Load;
        scenePath = std::move(path);
        loadFromMemory = false;
    }
    SceneLoadEvent(std::vector<uint8_t> buffer, size_t size) {
        type = EventType::Scene_Load;
        sceneDataBuffer = buffer;
        sceneDataSize = size;
        loadFromMemory = true;
    }
    SceneLoadEvent(const uint8_t *data, size_t size) {
        // TODO: implement
        type = EventType::Scene_Load;
        sceneData = data;
        sceneDataSize = size;
        loadFromMemory = true;
    }
};

struct SceneResetEvent : public Event {
    SceneResetEvent() {
        type = EventType::Scene_Reset;
    }
};
} // namespace RetroRenderer