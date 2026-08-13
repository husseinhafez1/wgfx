#pragma once

struct GLFWwindow;

namespace wgfx {

class Lighting;

struct ImGuiLayerChanges {
    bool vsyncChanged = false;
    bool msaaChanged = false;
    bool lightingChanged = false;
    bool shadowConfigurationChanged = false;
};

class ImGuiLayer {
public:
    explicit ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void beginFrame();
    ImGuiLayerChanges drawRendererPanel(
        bool& vsyncEnabled,
        bool& msaaEnabled,
        bool& bloomEnabled,
        float& exposure,
        float& bloomThreshold,
        Lighting& lighting
    );
    void endFrame();
    [[nodiscard]] bool wantsMouseCapture() const;
    [[nodiscard]] bool wantsKeyboardCapture() const;
};

} // namespace wgfx
