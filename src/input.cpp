#include "input.h"

bool Input::keys[1024]{};

Input::Input() {
    for (bool& key : keys) {
        key = false;
    }
}

bool Input::onKeyPress(int key) {
    if (key >= 0 && key < 1024 && glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS) {
        keys[key] = true;
        return true;
    }
    return false;
}

bool Input::onKeyRelease(int key) {
    if (key >= 0 && key < 1024 && glfwGetKey(glfwGetCurrentContext(), key) == GLFW_RELEASE) {
        keys[key] = false;
        return true;
    }
    return false;
}

bool Input::onButtonPress(int button) {
    if (button >= 0 && button < 1024 && glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS) {
        keys[button] = true;
        return true;
    }
    return false;
}

bool Input::onButtonRelease(int button) {
    if (button >= 0 && button < 1024 && glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_RELEASE) {
        keys[button] = false;
        return true;
    }
    return false;
}

bool Input::onButtonHold(int button) {
    if (button >= 0 && button < 1024 && glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS) {
        return true;
    }
    return false;
}