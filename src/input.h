#pragma once

#include <GLFW/glfw3.h>

class Input {
public:
    Input();

    [[nodiscard]] bool onKeyPress(int key);
    [[nodiscard]] bool onKeyRelease(int key);
    [[nodiscard]] bool onKeyHold(int key) const;
    [[nodiscard]] bool onButtonPress(int button);
    [[nodiscard]] bool onButtonRelease(int button);
    [[nodiscard]] bool onButtonHold(int button) const;
private:
    static bool keys[1024];
    static bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
};
