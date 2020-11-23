#version 460 core

out vec4 FragColor;

uniform sampler2DMS msaaColorTex;
uniform int uSampleNum;

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
	for(int i=0;i<uSampleNum;i++)
	{
		vec4 colorSample = texelFetch(msaaColorTex, ivec2(gl_FragCoord.xy), i);  // 第i个子样本
		float randP = rand(gl_FragCoord.xy * (float(i+1)/uSampleNum));
		if(randP < colorSample.a)//这种情况下保留样本
		{
			FragColor.rgb +=colorSample.rgb;
		}
	}
	FragColor.rgb /= float(uSampleNum);
	FragColor.a = 1.0f;
}