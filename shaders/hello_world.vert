#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 color;
} ubo;

layout( push_constant ) uniform PushConstants {
    mat4 matrix;
} pushConstant;

layout(location = 0)  in vec3 inPosition;
layout(location = 1)  in vec3 inColor;
layout(location = 2)  in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = pushConstant.matrix * vec4(inPosition, 1.0);
    fragColor = inColor * ubo.color;
    fragTexCoord = inTexCoord;
}
