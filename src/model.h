#pragma once

#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <string>

#include "buffer.h"

class Model {
public:
    Model(const std::string& path);
    ~Model();

    void draw() const;
private:
    unsigned int vao;
    Buffer vbo;
    Buffer ebo;
    std::size_t indexCount;
};
