#pragma once

#include <GLFW/glfw3.h>

class Input {
public:
    Input();

    bool onKeyPress(int key);
    bool onKeyRelease(int key);
    bool onKeyHold(int key) const;
    bool onButtonPress(int button);
    bool onButtonRelease(int button);
    bool onButtonHold(int button) const;
private:
    static bool keys[1024];
    static bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
};
