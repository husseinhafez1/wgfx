#include "input.h"

namespace wgfx {

bool Input::keys[1024]{};
bool Input::mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1]{};

Input::Input() {
    for (bool& key : keys) {
        key = false;
    }
    for (bool& button : mouseButtons) {
        button = false;
    }
}

bool Input::onKeyPress(int key) {
    if (key < 0 || key >= 1024) {
        return false;
    }

    const bool isPressed = glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
    const bool pressedThisFrame = isPressed && !keys[key];
    keys[key] = isPressed;
    return pressedThisFrame;
}

bool Input::onKeyRelease(int key) {
    if (key < 0 || key >= 1024) {
        return false;
    }

    const bool isPressed = glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
    const bool releasedThisFrame = !isPressed && keys[key];
    keys[key] = isPressed;
    return releasedThisFrame;
}

bool Input::onKeyHold(int key) const {
    return key >= 0 && key < 1024
        && glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
}

bool Input::onButtonPress(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }

    const bool isPressed = glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS;
    const bool pressedThisFrame = isPressed && !mouseButtons[button];
    mouseButtons[button] = isPressed;
    return pressedThisFrame;
}

bool Input::onButtonRelease(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }

    const bool isPressed = glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS;
    const bool releasedThisFrame = !isPressed && mouseButtons[button];
    mouseButtons[button] = isPressed;
    return releasedThisFrame;
}

bool Input::onButtonHold(int button) const {
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST
        && glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS;
}

} // namespace wgfx
