#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = vec4(fragColor.x,fragColor.y,fragColor.z,1.0f);
}