#include "imgui_layer.h"

#include "lighting.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>
#include <string>

namespace wgfx {

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

ImGuiLayerChanges ImGuiLayer::drawRendererPanel(
    bool& vsyncEnabled,
    bool& msaaEnabled,
    bool& bloomEnabled,
    float& exposure,
    float& bloomThreshold,
    Lighting& lighting
) {
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
    ImGuiLayerChanges changes;
    changes.vsyncChanged = ImGui::Checkbox("VSync", &vsyncEnabled);
    changes.msaaChanged = ImGui::Checkbox("8x MSAA", &msaaEnabled);
    ImGui::Checkbox("Bloom", &bloomEnabled);
    if (bloomEnabled) {
        ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 10.0f, "%.2f");
        ImGui::DragFloat("Bloom threshold", &bloomThreshold, 0.05f, 0.0f, 100.0f, "%.2f");
    }
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame time: %.2f ms", 1000.0f / io.Framerate);
    ImGui::Text("FPS: %.1f", io.Framerate);

    ImGui::SeparatorText("Directional Light");
    ImGui::TextDisabled("Directional lights cast cascaded shadows.");
    if (lighting.hasDirectional()) {
        DirectionalLight& light = lighting.getDirectionalLight();
        const bool directionChanged = ImGui::DragFloat3(
            "Direction##directional",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        );
        if (directionChanged && glm::dot(light.direction, light.direction) < 0.0001f) {
            light.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        changes.lightingChanged |= directionChanged;
        changes.lightingChanged |= ImGui::ColorEdit3("Color##directional", &light.color.x);
        changes.lightingChanged |= ImGui::DragFloat(
            "Intensity##directional",
            &light.intensity,
            0.05f,
            0.0f,
            1000.0f
        );
        if (ImGui::Button("Remove Directional Light")) {
            lighting.clearDirectionalLight();
            changes.lightingChanged = true;
        }
    } else if (ImGui::Button("Add Directional Light")) {
        lighting.setDirectionalLight({});
        changes.lightingChanged = true;
    }

    ImGui::SeparatorText("Point Lights");
    ImGui::TextDisabled("The first point light casts shadows.");
    if (lighting.getPointLights().size() < Lighting::MaxPointLights
        && ImGui::Button("Add Point Light")) {
        lighting.addPointLight({glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(1.0f), 100.0f, 10.0f});
        changes.lightingChanged = true;
        changes.shadowConfigurationChanged = true;
    }
    std::size_t pointToRemove = lighting.getPointLights().size();
    for (std::size_t index = 0; index < lighting.getPointLights().size(); ++index) {
        PointLight& light = lighting.getPointLights()[index];
        ImGui::PushID(static_cast<int>(index));
        const std::string label = "Point " + std::to_string(index + 1);
        if (ImGui::TreeNode(label.c_str())) {
            const bool positionChanged = ImGui::DragFloat3(
                "Position",
                &light.position.x,
                0.05f
            );
            changes.lightingChanged |= positionChanged;
            changes.shadowConfigurationChanged |= positionChanged && index == 0;
            changes.lightingChanged |= ImGui::ColorEdit3("Color", &light.color.x);
            changes.lightingChanged |= ImGui::DragFloat(
                "Intensity",
                &light.intensity,
                0.5f,
                0.0f,
                10000.0f
            );
            const bool rangeChanged = ImGui::DragFloat(
                "Range",
                &light.range,
                0.1f,
                0.2f,
                1000.0f
            );
            changes.lightingChanged |= rangeChanged;
            changes.shadowConfigurationChanged |= rangeChanged && index == 0;
            if (ImGui::Button("Remove")) {
                pointToRemove = index;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (pointToRemove < lighting.getPointLights().size()) {
        lighting.removePointLight(pointToRemove);
        changes.lightingChanged = true;
        changes.shadowConfigurationChanged = true;
    }

    ImGui::SeparatorText("Spot Lights");
    ImGui::TextDisabled("The first spot light casts shadows.");
    if (lighting.getSpotLights().size() < Lighting::MaxSpotLights
        && ImGui::Button("Add Spot Light")) {
        lighting.addSpotLight({});
        changes.lightingChanged = true;
        changes.shadowConfigurationChanged = true;
    }
    std::size_t spotToRemove = lighting.getSpotLights().size();
    for (std::size_t index = 0; index < lighting.getSpotLights().size(); ++index) {
        SpotLight& light = lighting.getSpotLights()[index];
        ImGui::PushID(static_cast<int>(index));
        const std::string label = "Spot " + std::to_string(index + 1);
        if (ImGui::TreeNode(label.c_str())) {
            const bool positionChanged = ImGui::DragFloat3(
                "Position",
                &light.position.x,
                0.05f
            );
            const bool directionChanged = ImGui::DragFloat3(
                "Direction",
                &light.direction.x,
                0.01f,
                -1.0f,
                1.0f
            );
            if (directionChanged && glm::dot(light.direction, light.direction) < 0.0001f) {
                light.direction = glm::vec3(0.0f, -1.0f, 0.0f);
            }
            changes.lightingChanged |= positionChanged || directionChanged;
            changes.shadowConfigurationChanged |= (positionChanged || directionChanged) && index == 0;
            changes.lightingChanged |= ImGui::ColorEdit3("Color", &light.color.x);
            changes.lightingChanged |= ImGui::DragFloat(
                "Intensity",
                &light.intensity,
                0.5f,
                0.0f,
                10000.0f
            );
            const bool rangeChanged = ImGui::DragFloat(
                "Range",
                &light.range,
                0.1f,
                0.2f,
                1000.0f
            );
            changes.lightingChanged |= rangeChanged;
            changes.shadowConfigurationChanged |= rangeChanged && index == 0;
            changes.lightingChanged |= ImGui::DragFloat(
                "Inner Cone",
                &light.innerConeAngle,
                0.1f,
                0.1f,
                88.0f
            );
            const bool outerConeChanged = ImGui::DragFloat(
                "Outer Cone",
                &light.outerConeAngle,
                0.1f,
                0.2f,
                89.0f
            );
            light.innerConeAngle = glm::min(light.innerConeAngle, light.outerConeAngle);
            changes.lightingChanged |= outerConeChanged;
            changes.shadowConfigurationChanged |= outerConeChanged && index == 0;
            if (ImGui::Button("Remove")) {
                spotToRemove = index;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (spotToRemove < lighting.getSpotLights().size()) {
        lighting.removeSpotLight(spotToRemove);
        changes.lightingChanged = true;
        changes.shadowConfigurationChanged = true;
    }

    ImGui::End();
    return changes;
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

} // namespace wgfx
