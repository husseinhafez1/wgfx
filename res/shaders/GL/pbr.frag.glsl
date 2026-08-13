#version 460

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 vertexUV;

layout(location = 0) out vec4 fragmentColor;

uniform vec4 baseColor;
uniform sampler2D baseColorTexture;
uniform bool hasBaseColorTexture;
uniform float metallic;
uniform float roughness;
uniform sampler2D metallicRoughnessTexture;
uniform bool hasMetallicRoughnessTexture;

#include "common/lighting.glsl"

void main() {
    vec4 textureColor = hasBaseColorTexture ? texture(baseColorTexture, vertexUV) : vec4(1.0);
    vec3 albedo = baseColor.rgb * textureColor.rgb;
    float alpha = baseColor.a * textureColor.a;

    float surfaceMetallic = metallic;
    float surfaceRoughness = roughness;
    if (hasMetallicRoughnessTexture) {
        vec4 materialSample = texture(metallicRoughnessTexture, vertexUV);
        surfaceRoughness *= materialSample.g;
        surfaceMetallic *= materialSample.b;
    }
    surfaceMetallic = clamp(surfaceMetallic, 0.0, 1.0);
    surfaceRoughness = clamp(surfaceRoughness, 0.04, 1.0);

    vec3 normal = normalize(worldNormal);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 reflectance = mix(vec3(0.04), albedo, surfaceMetallic);
    vec3 direct = calculateDirectLighting(
        normal, viewDirection, albedo, surfaceMetallic, surfaceRoughness, reflectance
    );
    vec3 ambient = calculateEnvironmentLighting(
        normal, viewDirection, albedo, surfaceMetallic, surfaceRoughness, reflectance
    );

    vec3 color = direct + ambient * 0.35;
    color = color / (color + vec3(1.0));
    fragmentColor = vec4(color, alpha);
}
