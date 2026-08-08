#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 vertexUV;
out vec4 lightSpacePosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    worldNormal = normalize(mat3(transpose(inverse(model))) * normal);
    vertexUV = uv;
    lightSpacePosition = lightSpaceMatrix * world;
    gl_Position = projection * view * world;
}
