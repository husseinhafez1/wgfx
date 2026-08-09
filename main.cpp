#include <renderer.h>

#include <exception>
#include <iostream>

int main() {
    try {
        wgfx::Renderer renderer;
        renderer.init();
        renderer.run();
    } catch (const std::exception& exception) {
        std::cerr << "Application error: " << exception.what() << '\n';
        return -1;
    }
    return 0;
}
