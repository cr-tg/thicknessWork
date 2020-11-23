#version 430 core

uniform float pTransmission[3];
flat in int index;

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{        
	float p = rand(gl_FragCoord.xy);//����0-1�������
	if(p < pTransmission[gl_Layer])//͸����Ϊ0���Ͳ�����Ƭ��,͸����Ϊ1����ȫ������
		discard;
    //gl_FragDepth = gl_FragCoord.z;
}