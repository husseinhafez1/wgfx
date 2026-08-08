#pragma once

#include <GLFW/glfw3.h>

class Input {
public:
    Input();

    bool onKeyPress(int key);
    bool onKeyRelease(int key);
    bool onButtonPress(int button);
    bool onButtonRelease(int button);
    bool onButtonHold(int button);
private:
    static bool keys[1024];
};
