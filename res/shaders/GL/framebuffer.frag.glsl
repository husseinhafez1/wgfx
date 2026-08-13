#version 460 core

in vec2 uv;
out vec4 fragmentColor;

uniform sampler2D screenTexture;

void main() {
    fragmentColor = texture(screenTexture, uv);
}
