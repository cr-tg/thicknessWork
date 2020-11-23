#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0)uniform sampler2D listColor;
layout(binding = 1)uniform sampler2D realLightDepthMap;
layout(binding = 3,r32ui) uniform restrict uimage2D uThickImage;

#define renderingShowSize 0.0f


uniform float far_plane;
uniform float near_plane;
uniform vec2 uScreenSize;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
	z = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
	//z = z / far_plane;
	z = (z-near_plane)/(far_plane - near_plane);
    return z;
}

void main()
{  
	if(false)
	{ 
		uint thick = imageLoad(uThickImage,ivec2(gl_FragCoord.xy)).r;
		//float showThick = uintBitsToFloat(thick);
		float showThick = uintBitsToFloat(thick)/far_plane;
		FragColor = vec4(showThick);
		//FragColor = vec4(thick/255.0f);
		//FragColor = vec4(showThick/255.0f);
		//FragColor = vec4(showThick/255.0f*10.0f);
		return;
	}
	if(false)
	{
		//float depth = texture(realLightDepthMap,vec2(TexCoords)).r;
		float depth = texelFetch(realLightDepthMap,ivec2(gl_FragCoord.xy),0).r;
		depth = LinearizeDepth(depth);
		FragColor = vec4(depth);
		return;
	}
	if(gl_FragCoord.x > uScreenSize.x * renderingShowSize && gl_FragCoord.y > uScreenSize.y * renderingShowSize)
	{
		uint thick = imageLoad(uThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		float showThick = uintBitsToFloat(thick)/255.0f*100.0f;
		FragColor = vec4(showThick);
		return;
	}
	//FragColor = vec4(abs(texture(listColor,vec2(TexCoords))/255.0f*10.0f));
	FragColor = vec4(texture(listColor,vec2(TexCoords)));
}