#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aBlockType;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float BlockType;
out float FogFactor;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_cameraPos;
uniform float u_fogDistance;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    TexCoord = aTexCoord;
    BlockType = aBlockType;

    // Distance-based fog
    float dist = length(aPos - u_cameraPos);
    float fogStart = u_fogDistance * 0.6;
    float fogEnd = u_fogDistance;
    FogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);

    gl_Position = u_projection * u_view * vec4(aPos, 1.0);
}
