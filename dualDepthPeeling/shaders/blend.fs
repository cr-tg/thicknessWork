#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D uFrontTex;
layout(binding = 1) uniform sampler2D uBackTex;


void main()
{  
	vec4 frontColor = texture(uFrontTex,TexCoords);
	vec4 backColor = texture(uBackTex,TexCoords);
	float alphaMultipier = 1.0f - frontColor.a;
	if(backColor.rgb != vec3(0.0f))
		FragColor = vec4(frontColor.rgb * alphaMultipier  + backColor.rgb  * frontColor.a,1.0f);
		//FragColor = vec4(frontColor.rgb * frontColor.a + backColor.rgb  * (1.0 - frontColor.a),frontColor.a);
	else
		FragColor = frontColor;
}