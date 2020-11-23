#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

flat out int index;

void main()
{
    for (int Layer = 0; Layer < 3; ++Layer) 
	{
        for (int VertexIndex = 0; VertexIndex < gl_in.length(); ++VertexIndex)
		{
            gl_Layer = Layer;
			index = Layer;
			gl_Position = gl_in[VertexIndex].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}