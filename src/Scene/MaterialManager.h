#pragma once

#include "../Renderer/MaterialTypes.h"
#include "../Renderer/RenderServices.h"
#include "../Renderer/ShaderHandle.h"
#include "../Renderer/ShaderResource.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace RetroRenderer {

class Scene;

struct ShaderProgram {
    ShaderHandle handle;
    std::shared_ptr<const RenderShaderSnapshot> source;
    std::string name;
    std::string vertexPath;
    std::string fragmentPath;
};

class MaterialManager {
  public:
    MaterialManager() = default;
    ~MaterialManager() = default;

    void BindRenderServices(IRenderInvalidationSink& renderInvalidationSink);
    void BindSceneAccessor(std::function<std::shared_ptr<Scene>()> sceneAccessor);
    bool Init();

    void LoadTexture(const std::string& path);
    void LoadTexture(const uint8_t* data, const size_t size);
    void ClearTexture();

    void SelectSceneMaterial(SceneMaterialHandle handle);
    [[nodiscard]] SceneMaterialHandle GetSelectedSceneMaterialHandle() const {
        return m_SelectedSceneMaterialHandle;
    }
    [[nodiscard]] SceneMaterial* GetSelectedSceneMaterial();
    [[nodiscard]] const SceneMaterial* GetSelectedSceneMaterial() const;
    [[nodiscard]] const Texture* GetSelectedPreviewTexture() const;

    [[nodiscard]] std::shared_ptr<const CompiledMaterialTemplate> GetCompiledTemplate(const std::filesystem::path& templatePath) const;
    [[nodiscard]] std::shared_ptr<const CompiledMaterialTemplate> GetCompiledTemplate(const SceneMaterial& material) const;
    [[nodiscard]] std::filesystem::path ResolveBuiltInTemplatePath(bool useVertexColor) const;

    static ShaderProgram CreateShaderProgram(IShaderCompiler& shaderCompiler,
                                             const std::string& vertexPath,
                                             const std::string& fragmentPath);
    static ShaderProgram CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

  private:
    static std::string ReadTextFile(const std::string& path);
    static std::string ReadTextFile(const std::filesystem::path& path);

    std::shared_ptr<const CompiledMaterialTemplate> LoadAndCompileTemplate(const std::filesystem::path& templatePath) const;
    void LoadTextureIntoSelectedMaterial(Texture texture);
    SceneMaterial* ResolveSelectedSceneMaterial();
    const SceneMaterial* ResolveSelectedSceneMaterial() const;
    std::filesystem::path ResolveTemplateAssetPath(const std::filesystem::path& templatePath) const;

  private:
    IRenderInvalidationSink* p_RenderInvalidationSink_ = nullptr;
    std::function<std::shared_ptr<Scene>()> m_SceneAccessor;
    SceneMaterialHandle m_SelectedSceneMaterialHandle = kInvalidSceneMaterialHandle;
    mutable std::unordered_map<std::string, std::shared_ptr<const CompiledMaterialTemplate>> m_TemplateCache;
};

} // namespace RetroRenderer
