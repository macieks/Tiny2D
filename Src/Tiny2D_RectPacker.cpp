#include "Tiny2D.h"
#include "Tiny2D_Common.h"

#include <algorithm>

using namespace Tiny2D;

bool RectPacker::SolveFixedRect(unsigned int space, unsigned int numLayers, std::vector<Rect>& rects, unsigned int width, unsigned int height)
{
	// Initialize set of free rectangles

	std::vector<Rect> free;
	for (unsigned int i = 0; i < numLayers; i++)
	{
		Rect& r = vector_add(free);
		r.x = 0;
		r.y = 0;
		r.w = width;
		r.h = height;
		r.layer = i;
	}

	// Process all rectangles

	for (unsigned int i = 0; i < rects.size(); i++)
	{
		Rect& r = rects[i];

		// Skip zero area rectangles

		if (r.w == 0 || r.h == 0)
			continue;

		// Find best container

		Rect* best = NULL;
		unsigned int bestIndex = -1;
		float bestFactor = (float) (width * height);
		unsigned int bestW, bestH;
		for (unsigned int j = 0; j < free.size(); j++)
		{
			Rect& c = free[j];

			const unsigned int w = r.w + space;//ueMin(space, width - c.x - c.w);
			const unsigned int h = r.h + space;//ueMin(space, height - c.y - c.h);

			if (c.w < w || c.h < h)
				continue;

			const float factor = (float) (c.w * c.h - w * h);
			if (factor > bestFactor)
				continue;

			best = &c;
			bestIndex = j;
			bestFactor = factor;
			bestW = w;
			bestH = h;
		}

		if (!best)
			return false;

		// Split container into 3 rectangles

		Rect left, right;
		left.layer = best->layer;
		right.layer = best->layer;

		unsigned int wLeftover = best->w - bestW;
		unsigned int hLeftover = best->h - bestH;
		if (wLeftover <= hLeftover) // Split the remaining space in horizontal direction.
		{
			left.x = best->x + bestW;
			left.y = best->y;
			left.w = wLeftover;
			left.h = bestH;

			right.x = best->x;
			right.y = best->y + bestH;
			right.w = best->w;
			right.h = hLeftover;
		}
		else // Split the remaining space in vertical direction.
		{
			left.x = best->x;
			left.y = best->y + bestH;
			left.w = bestW;
			left.h = hLeftover;

			right.x = best->x + bestW;
			right.y = best->y;
			right.w = wLeftover;
			right.h = best->h;
		}

		// Set up rect position

		r.x = best->x;
		r.y = best->y;
		r.layer = best->layer;

		// Remove used rectangle

		vector_remove_at(free, bestIndex);

		// Insert new free rectangles

		if (left.w > space && left.h > space)
			free.push_back(left);
		if (right.w > space && right.h > space)
			free.push_back(right);
	}

	return true;
}

void RectPacker::Solve(unsigned int space, unsigned int numLayers, bool pow2Dim, std::vector<Rect>& rects, unsigned int& width, unsigned int& height)
{
	std::sort(rects.begin(), rects.end(), Rect::Cmp());

	// Find smallest pow2-sized square

	width = height = 16;
	while (1)
	{
		if (SolveFixedRect(space, numLayers, rects, width, height))
			break;

		width *= 2;
		height = width;
	}

	// Shrink height

	unsigned int heightDecr = height / 2;
	while (heightDecr >= 4)
	{
		if (!pow2Dim)
			while (SolveFixedRect(space, numLayers, rects, width, height - heightDecr))
				height -= heightDecr;
		else if (SolveFixedRect(space, numLayers, rects, width, height - heightDecr))
			height -= heightDecr;
		else
			break;

		heightDecr /= 2;
	}

	// Shrink width

	unsigned int widthDecr = width / 2;
	while (widthDecr >= 4)
	{
		if (!pow2Dim)
			while (SolveFixedRect(space, numLayers, rects, width - widthDecr, height))
				width -= widthDecr;
		else if (SolveFixedRect(space, numLayers, rects, width - widthDecr, height))
			width -= widthDecr;
		else
			break;

		widthDecr /= 2;
	}

	// Re-solve (we know it will succeed, because it already did - we lost previous solution by overwriting it with other solutions)
	
	SolveFixedRect(space, numLayers, rects, width, height);
}