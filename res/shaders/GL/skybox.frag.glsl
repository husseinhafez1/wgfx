#version 460

in vec3 textureDirection;

layout(location = 0) out vec4 fragmentColor;

uniform samplerCube skybox;

void main() {
    fragmentColor = texture(skybox, textureDirection);
}
