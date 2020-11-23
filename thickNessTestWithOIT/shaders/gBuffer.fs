#version 460 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;

in vec4 worldPos;
in vec4 worldNormal;

void main()
{
	gPosition = worldPos;
	gNormal = worldNormal;
}

