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

uniform samplerCube environmentMap;
uniform vec3 cameraPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;

const float PI = 3.14159265359;

float distributionGGX(vec3 normal, vec3 halfway, float surfaceRoughness) {
    float a = surfaceRoughness * surfaceRoughness;
    float a2 = a * a;
    float nDotH = max(dot(normal, halfway), 0.0);
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.0001);
}

float geometrySchlickGGX(float nDotV, float surfaceRoughness) {
    float r = surfaceRoughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
}

float geometrySmith(vec3 normal, vec3 viewDirection, vec3 light, float surfaceRoughness) {
    return geometrySchlickGGX(max(dot(normal, viewDirection), 0.0), surfaceRoughness)
         * geometrySchlickGGX(max(dot(normal, light), 0.0), surfaceRoughness);
}

vec3 fresnelSchlick(float cosine, vec3 reflectance) {
    return reflectance + (1.0 - reflectance) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosine, vec3 reflectance, float surfaceRoughness) {
    return reflectance
         + (max(vec3(1.0 - surfaceRoughness), reflectance) - reflectance)
         * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}

void main() {
    vec4 textureColor = hasBaseColorTexture ? texture(baseColorTexture, vertexUV) : vec4(1.0);
    vec3 albedo = baseColor.rgb * pow(textureColor.rgb, vec3(2.2));
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
    vec3 light = normalize(-lightDirection);
    vec3 halfway = normalize(viewDirection + light);
    vec3 reflectance = mix(vec3(0.04), albedo, surfaceMetallic);

    float nDotL = max(dot(normal, light), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);
    float normalDistribution = distributionGGX(normal, halfway, surfaceRoughness);
    float geometry = geometrySmith(normal, viewDirection, light, surfaceRoughness);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDirection), 0.0), reflectance);
    vec3 specular = normalDistribution * geometry * fresnel
                  / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - surfaceMetallic);
    vec3 direct = (diffuseWeight * albedo / PI + specular) * lightColor * nDotL;

    vec3 environmentDiffuse = pow(texture(environmentMap, normal).rgb, vec3(2.2));
    vec3 reflection = reflect(-viewDirection, normal);
    vec3 environmentSpecular = pow(
        textureLod(environmentMap, reflection, surfaceRoughness * 8.0).rgb,
        vec3(2.2)
    );
    vec3 environmentFresnel = fresnelSchlickRoughness(nDotV, reflectance, surfaceRoughness);
    vec3 ambientDiffuse = (vec3(1.0) - environmentFresnel) * (1.0 - surfaceMetallic)
                        * environmentDiffuse * albedo;
    vec3 ambient = ambientDiffuse + environmentSpecular * environmentFresnel;

    vec3 color = direct + ambient * 0.35;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    fragmentColor = vec4(color, alpha);
}
