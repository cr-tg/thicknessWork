#version 430 core

uniform float pTransmission[3];
flat in int index;//openGL 430 以后，需要使用全局gl_layer，而不是从几何着色器传过来的index

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}



void main()
{        
	//float p = rand(gl_FragCoord.xy/gl_FragCoord.z);//每一个fragment生成一个随机数，这样会造成透光的部分更少
	float p = rand(gl_FragCoord.xy);//每一个pixel共用同一个随机数
	if(p < pTransmission[gl_Layer])//透射率为0，就不丢弃片段,透射率为1，就全部丢弃
		discard;
    //gl_FragDepth = gl_FragCoord.z;
}