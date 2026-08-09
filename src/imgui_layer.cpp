#include "imgui_layer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>

ImGuiLayer::ImGuiLayer(GLFWwindow* window) {
    if (window == nullptr) {
        throw std::invalid_argument("ImGui requires a valid GLFW window.");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the ImGui GLFW backend.");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the ImGui OpenGL backend.");
    }
}

ImGuiLayer::~ImGuiLayer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(
        0,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode
    );
}

bool ImGuiLayer::drawRendererPanel(bool& vsyncEnabled) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr float panelWidth = 280.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - panelWidth, viewport->WorkPos.y),
        ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(
        ImVec2(panelWidth, viewport->WorkSize.y),
        ImGuiCond_FirstUseEver
    );
    ImGui::Begin("Renderer");
    const bool changed = ImGui::Checkbox("VSync", &vsyncEnabled);
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame time: %.2f ms", 1000.0f / io.Framerate);
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::End();
    return changed;
}

void ImGuiLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::wantsMouseCapture() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::wantsKeyboardCapture() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}
