#version 460 core

in vec2 uv;
out vec4 fragmentColor;

uniform sampler2D screenTexture;
uniform sampler2D bloomTexture;
uniform bool bloomEnabled;
uniform float exposure;
uniform float gamma;

void main() {
    vec3 hdrColor = max(texture(screenTexture, uv).rgb, vec3(0.0));
    if (!bloomEnabled) {
        vec3 displayColor = pow(clamp(hdrColor, 0.0, 1.0), vec3(1.0 / gamma));
        fragmentColor = vec4(displayColor, 1.0);
        return;
    }

    hdrColor += texture(bloomTexture, uv).rgb;
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * exposure);
    fragmentColor = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);
}
