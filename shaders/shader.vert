#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightDir;
    float time;
} ubo;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    fragColor = inColor;
    fragNormal = mat3(ubo.model) * inNormal;
    gl_Position = ubo.proj * ubo.view * worldPos;
}
