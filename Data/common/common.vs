uniform vec4 ProjectionScale;

vec4 GetPosition()
{
	return vec4(gl_Vertex.x * ProjectionScale.x + ProjectionScale.z, gl_Vertex.y * ProjectionScale.y + ProjectionScale.w, 0.0, 1.0);
}