#version 320 es

precision mediump float;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Push{
    mat4 transform; // projection * view * model (later we will use uniform buffers)
    mat4 modelMatrix;
} push;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
	out_color = vec4(fragColor, 1.0) * texture(texSampler, fragTexCoord);
}
