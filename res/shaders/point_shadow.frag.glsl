#version 460

in vec3 worldPosition;

uniform vec3 lightPosition;
uniform float farPlane;

void main() {
    gl_FragDepth = length(worldPosition - lightPosition) / farPlane;
}
