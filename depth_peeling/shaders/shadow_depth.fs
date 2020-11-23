#version 430 core

uniform float pTransmission[3];
flat in int index;

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{        
	float p = rand(gl_FragCoord.xy);//生成0-1的随机数
	if(p < pTransmission[gl_Layer])//透射率为0，就不丢弃片段,透射率为1，就全部丢弃
		discard;
    //gl_FragDepth = gl_FragCoord.z;
}