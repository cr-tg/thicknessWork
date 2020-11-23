#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//着色点的世界坐标
layout(binding = 1) uniform sampler2DArray depthMapSet;//真实光源下的shadow map
layout(binding = 4,r32ui) uniform restrict writeonly uimage2D uSdfThickImage;

uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;

const int MAX_MARCHING_STEPS = 255*2;
const float EPSILON = 1e-5;
const int MAX_DEPTH = 10;
int steps = 0;

struct ListNodeLight
{
	uint next;
	uint depth;
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};

out vec4 testColor;

float returnZe(float depth, float near, float far);
float linearizeDepth(float depth, float near, float far);
//根据目标点的非线性深度，投影矩阵的远近平面，在裁剪坐标系下的UV坐标和pv矩阵求出这个点的世界坐标
vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4);

//场景中的球的圆心分别为vec3(0.0f, 0.2f, 4.0f),vec3(0.0f, 0.2f, 3.2f),vec3(0.0f, 0.2f, 2.4f),radius都是0.2f
float sphereSDF(vec3 p,vec3 circleCenter,float radius);
float cubeSDF(vec3 p,vec3 circleCenter,float cubeLen);
float intersectSDF(float distA, float distB);
float unionSDF(float distA, float distB);
float differenceSDF(float distA, float distB);
float sceneSDF(vec3 samplePoint);

float calThick(vec3 ro,vec3 rd,float maxDist)
{
    float hitDepths[MAX_DEPTH];
    float totalDist = 0.0f;
    int depthIdx = 0;
	int marchingStep = 0;
    while(depthIdx < MAX_DEPTH && totalDist < maxDist && marchingStep < MAX_MARCHING_STEPS)
    {
        vec3  samplePoint = ro + rd * totalDist;
        float dist = abs(sceneSDF(samplePoint));
        totalDist += dist;
		marchingStep++;
		steps++;
        if(dist < EPSILON)
        {
            hitDepths[depthIdx] = totalDist;
            totalDist += 100.0 * EPSILON;//加一大步，避免一层被记录为两层,100的效果要好一点
            depthIdx++;
        }
    }
    float thickness = 0.0;
    for(int i = 0;i<depthIdx -1;i+=2)
    { 
       thickness += hitDepths[i+1] - hitDepths[i];
    }
	return thickness;
}

void main()
{
	vec4 worldSpaceDst = texelFetch(gPosition,ivec2(gl_FragCoord.xy),0);
	mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//裁剪坐标，-w,w的范围，除以w以后，变换到-1,1的范围
	vec2 sampleCoord = lightClipSpaceDst.xy / lightClipSpaceDst.w;// 采样坐标，从shadowmap中进行采样，得到DST的ORI的z值。
	sampleCoord = sampleCoord * 0.5 +0.5;//xy也是在[-1，1]
	float oriDepth = texture(depthMapSet,vec3(sampleCoord,0)).r;
	//float oriDepth = texture(shadowMapRealLight,sampleCoord).r;
	float oriLinearDepth = linearizeDepth(oriDepth,real_near_plane,real_far_plane);//0-1
	//testColor = vec4(oriLinearDepth);
	vec2 clipSpaceUV = lightClipSpaceDst.xy/lightClipSpaceDst.w;
	vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,real_near_plane,real_far_plane,clipSpaceUV,realLightPV);
	if(worldSpaceOri == vec4(2.0 * real_far_plane))
		return;
	float disInWorld = length(worldSpaceDst.xyz - worldSpaceOri.xyz);//2.0
	vec3 dir = normalize(worldSpaceDst.xyz - worldSpaceOri.xyz);
	vec3 ro = worldSpaceOri.xyz - dir * 0.01;//将ro往反方向移动一点，保证从物体外部出发 
	vec3 rd = dir;//marching的dir
	float maxDist = disInWorld + 0.1;//将距离增大一点，保证能够移动到物体的外部
	float allthick = calThick(ro,rd,maxDist);
	imageStore(uSdfThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}

//场景中的球的圆心分别为vec3(0.0f, 0.2f, 4.0f),vec3(0.0f, 0.2f, 3.2f),vec3(0.0f, 0.2f, 2.4f),radius都是0.2f
float sphereSDF(vec3 p,vec3 circleCenter,float radius)
{
	return length(p - circleCenter) - radius;//返回的是负数就是在球的内部，返回的是正数就是球的外部
}

float cubeSDF(vec3 p,vec3 circleCenter,float cubeLen) {
    // If d.x < 0, then -1 < p.x < 1, and same logic applies to p.y, p.z
    // So if all components of d are negative, then p is inside the unit cube
    vec3 d = abs(p - circleCenter) - vec3(0.5 * cubeLen);
    
    // Assuming p is inside the cube, how far is it from the surface?
    // Result will be negative or zero.
    float insideDistance = min(max(d.x, max(d.y, d.z)), 0.0);
    
    // Assuming p is outside the cube, how far is it from the surface?
    // Result will be positive or zero.
    float outsideDistance = length(max(d, 0.0));
    //如果点在内部，那么outsideDis就是0，insideDistance就是离边最近的一个维度的距离的负数
	//如果点在外部，那么insideDis就是0，outsideDistance就是到cube上的所有点中最近的距离
    return insideDistance + outsideDistance;
}


//因为为0的点代表的是图形的边界点，在内部的点是小于0的点，即值越大，就越不容易是图形的内部，因此这里求max就是取两个图形的交集
//比如一个球在A，一个球在B，它们之间不相交的话，intersect的结果就永远都大于0，即没有图形
float intersectSDF(float distA, float distB) {
    return max(distA, distB);
}

//反之，取较小值的话就是说只要点在任意一个图形的内部，那么就在结果的内部中，这是求并集
float unionSDF(float distA, float distB) {
    return min(distA, distB);
}

//这个也是求交集，只不过现在图形B是在内部为正，外部为负，意思就是求图形B外部与图形A的交集，
//形象点就是在A中间挖了一个B
float differenceSDF(float distA, float distB) {
    return max(distA, -distB);
}

float sceneSDF(vec3 samplePoint) {
	vec3 circleCenter1 = vec3(0.0,0.2,4.0);
	vec3 circleCenter2 = vec3(0.0,0.2,3.2);
	vec3 circleCenter3 = vec3(0.0,0.2,2.4);
	float radius = 0.2f;
    float sphereDist1 = sphereSDF(samplePoint,circleCenter1,radius);
    float sphereDist2 = sphereSDF(samplePoint,circleCenter2,radius);
    float sphereDist3 = sphereSDF(samplePoint,circleCenter3,radius);
	return unionSDF(sphereDist1,unionSDF(sphereDist2,sphereDist3));
}

float returnZe(float depth, float near, float far)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1，ze = -near
	z = (2.0 * near * far) / (z * (far - near) - (far + near)); // range: -near...-far,
	return z;
}

float linearizeDepth(float depth, float near, float far)
{
	float z = -returnZe(depth, near, far);
	z = (z - near) / (far - near); // range: 0...1
	return z;
}


vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth,nearPlane,farPlane);//0-1
	float bias = 1e-5;
	if(oriLinearDepth >= 1.0 -bias)//减少在深度图以外的采样，g3d中似乎从gbuffer中拿片段的时候就已经自动做了这一步
		return vec4(2.0 * farPlane);
	float zc = -nearPlane+oriLinearDepth*(nearPlane+farPlane);
	float ze = returnZe(depth,nearPlane,farPlane);
	float wc = -ze;
	float xc = clipPosUV.x * wc;
	float yc = clipPosUV.y * wc;
	vec4 clipPos = vec4(xc,yc,zc,wc);
	worldPos = inverse(pvMat4) * clipPos;
	return worldPos;
}




