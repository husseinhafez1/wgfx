#pragma once

struct GLFWwindow;

class ImGuiLayer {
public:
    explicit ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void beginFrame();
    bool drawRendererPanel(bool& vsyncEnabled);
    void endFrame();
    [[nodiscard]] bool wantsMouseCapture() const;
    [[nodiscard]] bool wantsKeyboardCapture() const;
};
