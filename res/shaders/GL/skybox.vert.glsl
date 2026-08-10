#version 460

layout(location = 0) in vec3 position;

out vec3 textureDirection;

uniform mat4 view;
uniform mat4 projection;

void main() {
    textureDirection = position;
    vec4 clipPosition = projection * view * vec4(position, 1.0);
    gl_Position = clipPosition.xyww;
}
