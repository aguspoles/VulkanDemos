#version 320 es

precision mediump float;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Push{
    mat4 transform; // projection * view * model (later we will use uniform buffers)
    mat4 modelMatrix;
} push;

void main()
{
	out_color = vec4(fragColor, 1.0);
}
