struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerConeCos;
    float outerConeCos;
};

const int MAX_POINT_LIGHTS = 8;
const int MAX_SPOT_LIGHTS = 8;
const int MAX_CASCADES = 8;
const float PI = 3.14159265359;

uniform samplerCube environmentMap;
uniform vec3 cameraPosition;
uniform mat4 view;

uniform bool hasDirectionalLight;
uniform DirectionalLight directionalLight;
uniform sampler2DArrayShadow shadowMap;
uniform int cascadeCount;
uniform float cascadePlaneDistances[MAX_CASCADES];
uniform float cascadeDepthRanges[MAX_CASCADES];
uniform mat4 lightSpaceMatrices[MAX_CASCADES];

uniform int pointLightCount;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform samplerCubeShadow pointShadowMap;
uniform int pointShadowLightIndex;
uniform float pointShadowFarPlane;

uniform int spotLightCount;
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform sampler2DShadow spotShadowMap;
uniform mat4 spotLightSpaceMatrix;
uniform int spotShadowLightIndex;

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

vec3 evaluateLight(
    vec3 light,
    vec3 radiance,
    vec3 normal,
    vec3 viewDirection,
    vec3 albedo,
    float surfaceMetallic,
    float surfaceRoughness,
    vec3 reflectance
) {
    vec3 halfway = normalize(viewDirection + light);
    float nDotL = max(dot(normal, light), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);
    float normalDistribution = distributionGGX(normal, halfway, surfaceRoughness);
    float geometry = geometrySmith(normal, viewDirection, light, surfaceRoughness);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDirection), 0.0), reflectance);
    vec3 specular = normalDistribution * geometry * fresnel
                  / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - surfaceMetallic);
    return (diffuseWeight * albedo / PI + specular) * radiance * nDotL;
}

float distanceAttenuation(float distanceToLight, float range) {
    float normalizedDistance = distanceToLight / max(range, 0.0001);
    float rangeFade = clamp(1.0 - pow(normalizedDistance, 4.0), 0.0, 1.0);
    return rangeFade * rangeFade / max(distanceToLight * distanceToLight, 0.01);
}

float sampleDirectionalShadowLayer(int layer, vec3 normal, vec3 light) {
    vec4 lightPosition = lightSpaceMatrices[layer] * vec4(worldPosition, 1.0);
    vec3 projected = lightPosition.xyz / lightPosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z > 1.0 || projected.z < 0.0
        || any(lessThan(projected.xy, vec2(0.0)))
        || any(greaterThan(projected.xy, vec2(1.0)))) {
        return 0.0;
    }

    float worldSpaceBias = max(0.15 * (1.0 - dot(normal, light)), 0.02);
    float bias = worldSpaceBias / cascadeDepthRanges[layer];
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0).xy);
    float visibility = 0.0;
    float totalWeight = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float weight = float(3 - abs(x)) * float(3 - abs(y));
            visibility += weight * texture(
                shadowMap,
                vec4(projected.xy + vec2(x, y) * texelSize, float(layer), projected.z - bias)
            );
            totalWeight += weight;
        }
    }
    return 1.0 - visibility / totalWeight;
}

float calculateDirectionalShadow(vec3 normal, vec3 light) {
    float viewDepth = -(view * vec4(worldPosition, 1.0)).z;
    int layer = cascadeCount - 1;
    for (int cascade = 0; cascade < cascadeCount; ++cascade) {
        if (viewDepth < cascadePlaneDistances[cascade]) {
            layer = cascade;
            break;
        }
    }

    float shadow = sampleDirectionalShadowLayer(layer, normal, light);
    if (layer < cascadeCount - 1) {
        float cascadeNear = layer == 0 ? 0.0 : cascadePlaneDistances[layer - 1];
        float transitionWidth = (cascadePlaneDistances[layer] - cascadeNear) * 0.1;
        float transition = smoothstep(
            cascadePlaneDistances[layer] - transitionWidth,
            cascadePlaneDistances[layer],
            viewDepth
        );
        if (transition > 0.0) {
            shadow = mix(
                shadow,
                sampleDirectionalShadowLayer(layer + 1, normal, light),
                transition
            );
        }
    }
    return shadow;
}

const vec3 POINT_SHADOW_OFFSETS[8] = vec3[](
    vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0),
    vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0),
    vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0)
);

float calculatePointShadow(int lightIndex, vec3 normal, vec3 light) {
    vec3 lightToFragment = worldPosition - pointLights[lightIndex].position;
    float currentDepth = length(lightToFragment);
    if (currentDepth >= pointShadowFarPlane) {
        return 0.0;
    }

    float worldSpaceBias = max(0.12 * (1.0 - dot(normal, light)), 0.04);
    float comparisonDepth = (currentDepth - worldSpaceBias) / pointShadowFarPlane;
    float diskRadius = 0.003 * currentDepth;
    float visibility = 0.0;
    for (int sampleIndex = 0; sampleIndex < 8; ++sampleIndex) {
        visibility += texture(
            pointShadowMap,
            vec4(lightToFragment + POINT_SHADOW_OFFSETS[sampleIndex] * diskRadius, comparisonDepth)
        );
    }
    return 1.0 - visibility / 8.0;
}

float calculateSpotShadow(vec3 normal, vec3 light) {
    vec4 lightPosition = spotLightSpaceMatrix * vec4(worldPosition, 1.0);
    if (lightPosition.w <= 0.0) {
        return 0.0;
    }

    vec3 projected = lightPosition.xyz / lightPosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z > 1.0 || projected.z < 0.0
        || any(lessThan(projected.xy, vec2(0.0)))
        || any(greaterThan(projected.xy, vec2(1.0)))) {
        return 0.0;
    }

    float bias = max(0.004 * (1.0 - dot(normal, light)), 0.0005);
    vec2 texelSize = 1.0 / vec2(textureSize(spotShadowMap, 0));
    float visibility = 0.0;
    float totalWeight = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float weight = float(2 - abs(x)) * float(2 - abs(y));
            visibility += weight * texture(
                spotShadowMap,
                vec3(projected.xy + vec2(x, y) * texelSize, projected.z - bias)
            );
            totalWeight += weight;
        }
    }
    return 1.0 - visibility / totalWeight;
}

vec3 calculateDirectLighting(
    vec3 normal,
    vec3 viewDirection,
    vec3 albedo,
    float surfaceMetallic,
    float surfaceRoughness,
    vec3 reflectance
) {
    vec3 direct = vec3(0.0);
    if (hasDirectionalLight) {
        vec3 light = normalize(-directionalLight.direction);
        float shadow = calculateDirectionalShadow(normal, light);
        direct += (1.0 - shadow) * evaluateLight(
            light, directionalLight.color * directionalLight.intensity,
            normal, viewDirection, albedo, surfaceMetallic, surfaceRoughness, reflectance
        );
    }

    for (int index = 0; index < pointLightCount; ++index) {
        vec3 toLight = pointLights[index].position - worldPosition;
        float distanceToLight = length(toLight);
        vec3 light = normalize(toLight);
        vec3 radiance = pointLights[index].color * pointLights[index].intensity
                      * distanceAttenuation(distanceToLight, pointLights[index].range);
        float shadow = index == pointShadowLightIndex
            ? calculatePointShadow(index, normal, light)
            : 0.0;
        direct += (1.0 - shadow) * evaluateLight(
            light, radiance, normal, viewDirection, albedo,
            surfaceMetallic, surfaceRoughness, reflectance
        );
    }

    for (int index = 0; index < spotLightCount; ++index) {
        vec3 toLight = spotLights[index].position - worldPosition;
        float distanceToLight = length(toLight);
        vec3 light = normalize(toLight);
        float coneCos = dot(-light, normalize(spotLights[index].direction));
        float cone = smoothstep(
            spotLights[index].outerConeCos,
            spotLights[index].innerConeCos,
            coneCos
        );
        vec3 radiance = spotLights[index].color * spotLights[index].intensity * cone
                      * distanceAttenuation(distanceToLight, spotLights[index].range);
        float shadow = index == spotShadowLightIndex
            ? calculateSpotShadow(normal, light)
            : 0.0;
        direct += (1.0 - shadow) * evaluateLight(
            light, radiance, normal, viewDirection, albedo,
            surfaceMetallic, surfaceRoughness, reflectance
        );
    }
    return direct;
}

vec3 calculateEnvironmentLighting(
    vec3 normal,
    vec3 viewDirection,
    vec3 albedo,
    float surfaceMetallic,
    float surfaceRoughness,
    vec3 reflectance
) {
    float nDotV = max(dot(normal, viewDirection), 0.0);
    vec3 environmentDiffuse = pow(texture(environmentMap, normal).rgb, vec3(2.2));
    vec3 reflection = reflect(-viewDirection, normal);
    vec3 environmentSpecular = pow(
        textureLod(environmentMap, reflection, surfaceRoughness * 8.0).rgb,
        vec3(2.2)
    );
    vec3 environmentFresnel = fresnelSchlickRoughness(nDotV, reflectance, surfaceRoughness);
    vec3 ambientDiffuse = (vec3(1.0) - environmentFresnel) * (1.0 - surfaceMetallic)
                        * environmentDiffuse * albedo;
    return ambientDiffuse + environmentSpecular * environmentFresnel;
}
