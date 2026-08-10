#version 460

layout(location = 0) in vec3 position;

out vec3 worldPosition;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    worldPosition = vec3(model * vec4(position, 1.0));
    gl_Position = lightSpaceMatrix * vec4(worldPosition, 1.0);
}
