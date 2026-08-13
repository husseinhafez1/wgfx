#version 460 core

in vec2 uv;
out vec4 fragmentColor;

uniform sampler2D screenTexture;
uniform float gamma;

void main() {
    vec3 linearColor = max(texture(screenTexture, uv).rgb, vec3(0.0));
    vec3 gammaCorrectedColor = pow(linearColor, vec3(1.0 / gamma));
    fragmentColor = vec4(gammaCorrectedColor, 1.0);
}
