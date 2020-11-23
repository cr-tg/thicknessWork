#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2DArray colorTexSet;
uniform int uLayerIndex;
uniform int uAllDepthLayer;

void main()
{  
	vec4 currentColor = texture(colorTexSet,vec3(TexCoords,uLayerIndex));
	if(uLayerIndex == (uAllDepthLayer-1))//要判断前一层是否为最后一层，是的话就不进行混合
		FragColor = currentColor;
	else
	{
		vec4 preColor = texture(colorTexSet,vec3(TexCoords,uLayerIndex+1));
		//混合，源*源的alpha+目标*(1-源的alpha)，如果源的alpha为1，就没有目标的颜色，源就是current color
		if(preColor.a != 0.0)//这里取了一个巧，就是将背景的alpha值设置为0.0，这样的话就可以不与背景进行混合了
		{
			FragColor = vec4(currentColor.rgb*currentColor.a+preColor.rgb*(1-currentColor.a),currentColor.a);
		}
		else
			FragColor = currentColor;
	}

}