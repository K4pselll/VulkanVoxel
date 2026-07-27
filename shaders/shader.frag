#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightDir;
    float time;
} ubo;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightDir);

    float diff = max(dot(norm, lightDir), 0.0);
    float ambient = 0.35;
    float lighting = ambient + diff * 0.65;

    vec3 result = fragColor * lighting;

    float fogDist = length(fragPos);
    float fog = clamp((fogDist - 40.0) / 60.0, 0.0, 0.7);
    result = mix(result, vec3(0.6, 0.75, 0.95), fog);

    outColor = vec4(result, 1.0);
}
