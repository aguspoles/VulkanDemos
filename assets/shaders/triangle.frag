#version 320 es

precision mediump float;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(fragColor, 1.0);
}
