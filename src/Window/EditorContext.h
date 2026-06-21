#pragma once

#include "../Base/Config.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/SceneManager.h"
#include <cassert>
#include <functional>
#include <memory>
#include <optional>

namespace RetroRenderer {

class Camera;
class MaterialManager;
class Scene;

struct EditorContext {
    std::shared_ptr<Config> config;
    std::shared_ptr<Stats> stats;
    SceneManager* sceneManager = nullptr;
    MaterialManager* materialManager = nullptr;
    std::function<void(const Event&)> dispatchImmediate;
    std::function<void(std::unique_ptr<Event>)> enqueueEvent;
    std::optional<int> selectedModelIndex;

    [[nodiscard]] Camera* GetCamera() const {
        return sceneManager != nullptr ? sceneManager->GetCamera() : nullptr;
    }

    [[nodiscard]] std::shared_ptr<Scene> GetScene() const {
        return sceneManager != nullptr ? sceneManager->GetScene() : nullptr;
    }

    [[nodiscard]] bool HasScene() const {
        return GetScene() != nullptr;
    }

    [[nodiscard]] bool HasSelectedModel() const {
        return selectedModelIndex.has_value();
    }

    [[nodiscard]] SceneManager& GetSceneManager() const {
        assert(sceneManager != nullptr && "EditorContext scene manager not bound");
        return *sceneManager;
    }

    [[nodiscard]] MaterialManager& GetMaterialManager() const {
        assert(materialManager != nullptr && "EditorContext material manager not bound");
        return *materialManager;
    }
};

} // namespace RetroRenderer
