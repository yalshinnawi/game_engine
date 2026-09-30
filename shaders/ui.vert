#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 Color;

uniform mat4 u_projection;

void main() {
    Color = aColor;
    gl_Position = u_projection * vec4(aPos, 0.0, 1.0);
}