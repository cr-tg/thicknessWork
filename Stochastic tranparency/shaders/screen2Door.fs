#version 460 core
uniform vec4 uObjColor;

out vec4 FragColor;

float thresholdMatrix[4][4] =
{
{1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0},
{13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0},
{4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0},
{16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0}
};

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

float noise( vec3 x )
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    vec3 p = floor(x);
    vec3 f = fract(x);

    f       = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;

    return mix(mix(mix( hash(n+0.0), hash(n+1.0),f.x),
                   mix( hash(n+57.0), hash(n+58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}



void main()
{
	//对于alpha值一样的物体，总是在相同的pixel处visible/invisible，因此需要在里面添加z分量
	//float threshold = thresholdMatrix[(int(gl_FragCoord.x)%4)][int(gl_FragCoord.y)%4];
	//int z = int((gl_FragCoord.z * 0.5 + 0.5) * 2000); // *2000扩大z分量的敏感度
	//int p = int(rand(gl_FragCoord.xy/gl_FragCoord.z) * 399) % 4;
	//ivec2 index = ivec2((int(gl_FragCoord.x + p)%4),int(gl_FragCoord.y + p)%4);//增长相同的数值，会让它们同时都小于0.5或者大于0.5

	//int p1 = int(rand(gl_FragCoord.xy/gl_FragCoord.z) * 399) % 4;
	//int p2 = int(rand(gl_FragCoord.xy/gl_FragCoord.z) * 499) % 4;
	float randomFactor =noise(uObjColor.rgb);
	int p1 = int(rand(gl_FragCoord.xy/randomFactor) * 399) % 4;
	int p2 = int(rand(gl_FragCoord.xy/randomFactor) * 499) % 4;

	ivec2 index = ivec2((int(gl_FragCoord.x + p1)%4),int(gl_FragCoord.y + p2)%4);//增长相同的数值，会让它们同时都小于0.5或者大于0.5
	
	float threshold = thresholdMatrix[index.x][index.y];
	if(uObjColor.a < threshold)
		discard;
	 // FragColor = vec4(p/25.5);
	//FragColor = vec4(threshold);
	//FragColor = vec4(index.x/3.0,index.y/3.0,0.0,0.0);
	FragColor = uObjColor;
}