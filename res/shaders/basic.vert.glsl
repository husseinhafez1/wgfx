#version 460

layout(location = 0) in vec3 position;

out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = position * 0.5 + 0.5;
}
