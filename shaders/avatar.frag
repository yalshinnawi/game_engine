#version 430 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 u_skyColor;
uniform vec3 u_cameraPos;
uniform float u_fogDistance;

void main() {
    vec3 N = normalize(Normal);
    vec3 sunDir = normalize(vec3(0.4, 0.8, 0.3));
    float diff = max(dot(N, sunDir), 0.0) * 0.75 + 0.35;
    vec3 litColor = Color * diff;

    float dist = length(FragPos - u_cameraPos);
    float fogStart = u_fogDistance * 0.6;
    float fogFactor = clamp((u_fogDistance - dist) / (u_fogDistance - fogStart), 0.0, 1.0);
    vec3 finalColor = mix(u_skyColor, litColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}