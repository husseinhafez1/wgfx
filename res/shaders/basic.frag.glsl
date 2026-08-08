#version 460

in vec3 vertexNormal;
in vec2 vertexUV;

layout(location = 0) out vec4 fragmentColor;

uniform vec4 baseColor;
uniform sampler2D baseColorTexture;
uniform bool hasBaseColorTexture;

void main() {
    vec4 color = baseColor;
    if (hasBaseColorTexture) {
        color *= texture(baseColorTexture, vertexUV);
    }
    fragmentColor = color;
}
