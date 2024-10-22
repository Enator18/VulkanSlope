#version 460

layout(binding = 0) uniform Camera
{
    mat4 view;
    mat4 proj;
} camera;

layout(std140,binding = 1) readonly buffer InstanceBuffer
{
    mat4 instances[];
} instanceBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
    mat4 model = instanceBuffer.instances[gl_BaseInstance];
    gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = (model * vec4(inNormal, 0.0)).xyz;
}