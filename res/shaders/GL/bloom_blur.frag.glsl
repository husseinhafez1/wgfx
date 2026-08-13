#version 460 core

in vec2 uv;
out vec4 fragmentColor;

uniform sampler2D image;
uniform bool horizontal;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(image, 0));
    vec3 result = texture(image, uv).rgb * weights[0];
    for (int index = 1; index < 5; ++index) {
        vec2 offset = horizontal
            ? vec2(texelSize.x * index, 0.0)
            : vec2(0.0, texelSize.y * index);
        result += texture(image, uv + offset).rgb * weights[index];
        result += texture(image, uv - offset).rgb * weights[index];
    }
    fragmentColor = vec4(result, 1.0);
}
