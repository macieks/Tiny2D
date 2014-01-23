#include "Tiny2D.h"
#include "Tiny2D_Common.h"

namespace Tiny2D
{

Shape::VertexStream::VertexStream()
{}

Shape::VertexStream::VertexStream(const void* data, VertexFormat format, int count, bool isNormalized, VertexUsage usage, int usageIndex, int stride)
{
	this->data = data;
	this->format = format;
	this->count = count;
	this->isNormalized = isNormalized;
	this->usage = usage;
	this->usageIndex = usageIndex;
	this->stride = stride;
}

Shape::Geometry::Geometry() :
	type(Type_TriangleFan),
	numVerts(0),
	numStreams(0),
	numIndices(0),
	indices(NULL)
{}

Shape::DrawParams::DrawParams() :
	color(Color::White),
	blending(Blending_Default)
{}

void Shape::Draw(const Shape::DrawParams* params)
{
#if 1
	Material& material = App::GetDefaultMaterial();
	material.SetTechnique("col");
	material.SetFloatParameter("Color", (const float*) &params->color, 4);
	material.Draw(params);
#else
	glDisableTexture2D();
	Shape_SetBlending(params->color.a != 1.0f, params->blending);

	glColor4fv((const GLfloat*) &params->color);
	glBegin(Geometry::Type_ToOpenGL(params->primitiveType));
	{
		const float* xy = params->xy;
		for (int i = 0; i < params->numVerts; i++, xy += 2)
			glVertex2f(xy[0], xy[1]);
	}
	glEnd();

	glEnableTexture2D();
#endif
}

void Shape::DrawRectangle(const Rect& rect, float rotation, const Color& color)
{
	float xy[] =
	{
		rect.left, rect.top,
		rect.left + rect.width, rect.top,
		rect.left + rect.width, rect.top + rect.height,
		rect.left, rect.top + rect.height
	};

	if (rotation != 0)
	{
		const float centerX = rect.left + rect.width * 0.5f;
		const float centerY = rect.top + rect.height * 0.5f;

		const float rotationSin = sinf(rotation);
		const float rotationCos = cosf(rotation);

		for (unsigned int i = 0; i < 4 * 2; i += 2)
			Vertex_Rotate(xy[i], xy[i + 1], centerX, centerY, rotationSin, rotationCos);
	}

	Shape::DrawParams params;
	params.color = color;
	params.SetNumVerts(4);
	params.SetGeometryType(Geometry::Type_TriangleFan);
	params.SetPosition(xy);

	Shape::Draw(&params);
}

void Shape::DrawCircle(const Vec2& center, float radius, int numSegments, float rotation, const Color& color)
{
	std::vector<Vec2> xy(numSegments);

	float angle = rotation;
	const float angleStep = 3.141592654f * 2.0f / (float) numSegments;
	for (unsigned int i = 0; i < xy.size(); i++)
	{
		xy[i] = Vec2(
			sinf(angle) * radius + center.x,
			cosf(angle) * radius + center.y);

		angle += angleStep;
	}

	Shape::DrawParams params;
	params.color = color;
	params.SetNumVerts(numSegments);
	params.SetGeometryType(Geometry::Type_TriangleFan);
	params.SetPosition(&xy[0]);

	Shape::Draw(&params);
}

void Shape::DrawLine(const Vec2& start, const Vec2& end, const Color& color)
{
	const Vec2 points[] = {start, end};

	Shape::DrawParams params;
	params.color = color;
	params.SetNumVerts(2);
	params.SetGeometryType(Geometry::Type_Lines);
	params.SetPosition(points);

	Shape::Draw(&params);
}

void Shape::DrawLines(const Vec2* xy, int count, const Color& color)
{
	Shape::DrawParams params;
	params.color = color;
	params.SetNumVerts(count * 2);
	params.SetGeometryType(Geometry::Type_Lines);
	params.SetPosition(xy);

	Shape::Draw(&params);
}

};