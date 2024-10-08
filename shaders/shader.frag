#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D mainTexture;

void main()
{
	//outFragColor = ((vec4(vec3(dot(fragNormal,vec3(-0.5, 1.0, 1.0))), 1.0) * 0.4) + 0.5) * texture(mainTexture, fragTexCoord * 4.0);
	outFragColor = texture(mainTexture, fragTexCoord * 4.0);
}