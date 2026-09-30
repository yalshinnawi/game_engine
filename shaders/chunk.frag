#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float BlockType;
in float FogFactor;

out vec4 FragColor;

uniform vec3 u_skyColor;
uniform vec3 u_cameraPos;

// Block type colors (procedural - no textures needed!)
vec3 getBlockColor(float blockType, vec3 normal) {
    int bt = int(blockType + 0.5);

    // Base colors for each block type
    vec3 color;
    switch (bt) {
        case 1: // GRASS
            if (normal.y > 0.5) {
                color = vec3(0.36, 0.63, 0.24); // Green top
            } else if (normal.y < -0.5) {
                color = vec3(0.55, 0.38, 0.24); // Dirt bottom
            } else {
                // Side: dirt with grass edge
                float grassEdge = smoothstep(0.3, 0.7, TexCoord.y);
                color = mix(vec3(0.55, 0.38, 0.24), vec3(0.36, 0.63, 0.24), grassEdge);
            }
            break;
        case 2: // DIRT
            color = vec3(0.55, 0.38, 0.24);
            break;
        case 3: // STONE
            color = vec3(0.5, 0.5, 0.5);
            // Subtle variation
            float stoneNoise = fract(sin(dot(FragPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
            color += (stoneNoise - 0.5) * 0.08;
            break;
        case 4: // SAND
            color = vec3(0.86, 0.80, 0.55);
            break;
        case 5: // WATER
            color = vec3(0.15, 0.45, 0.75);
            break;
        case 6: // WOOD
            color = vec3(0.45, 0.30, 0.15);
            break;
        case 7: // LEAVES
            color = vec3(0.22, 0.52, 0.15);
            break;
        case 8: // BEDROCK
            color = vec3(0.2, 0.2, 0.2);
            break;
        case 9: // SNOW
            color = vec3(0.95, 0.95, 0.98);
            break;
        default:
            color = vec3(1.0, 0.0, 1.0); // Magenta = unknown
            break;
    }
    return color;
}

void main() {
    vec3 blockColor = getBlockColor(BlockType, Normal);

    // ── Lighting ──
    // Sun direction (warm afternoon light)
    vec3 sunDir = normalize(vec3(0.4, 0.8, 0.3));
    vec3 sunColor = vec3(1.0, 0.95, 0.85);

    // Ambient
    float ambientStrength = 0.35;
    vec3 ambient = ambientStrength * vec3(0.6, 0.7, 0.9); // Slight sky tint

    // Diffuse (hemisphere lighting: sun + sky bounce)
    float sunDiffuse = max(dot(Normal, sunDir), 0.0);
    float skyDiffuse = max(dot(Normal, vec3(0, 1, 0)), 0.0) * 0.15;
    vec3 diffuse = sunDiffuse * sunColor + skyDiffuse * vec3(0.4, 0.5, 0.7);

    // Combine lighting
    vec3 litColor = blockColor * (ambient + diffuse);

    // ── Fog blending ──
    vec3 finalColor = mix(u_skyColor, litColor, FogFactor);

    // Water transparency
    float alpha = 1.0;
    if (int(BlockType + 0.5) == 5) { // WATER
        alpha = 0.7;
    }

    FragColor = vec4(finalColor, alpha);
}
