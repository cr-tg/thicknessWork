#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

//注意colorTexSet中存放的是每层blend以后的结果了，因为在blend的pass每次都会更新colorTexSet的值
layout(binding = 0) uniform sampler2DArray colorTexSet;

#define ySmallerSize 5.0
uniform int uDepthNum;

void main()
{  
	if(gl_FragCoord.y > 144)//0.2,0.2
	{
		FragColor = vec4(texture(colorTexSet,vec3(TexCoords,0.0))); 
		return;
	}
	vec4 color = vec4(0.0);
	float interval = 1.0/uDepthNum;
	for(int i=1;i<=uDepthNum;++i)
	{
		if(TexCoords.x < float(i) * interval)
		{
			color = texture(colorTexSet, vec3((TexCoords - vec2((i-1.0) * interval, 0.0)) * vec2(uDepthNum, ySmallerSize), i-1));
			break;
		}
	}
   FragColor = vec4(pow(vec3(color.rgb),vec3(1.0)), color.a); 
   //FragColor = vec4(pow(vec3(color.rgb),vec3(1.0/2.2)), color.a); 
}