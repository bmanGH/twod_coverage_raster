#include "CoverageRaster.h"
#include <algorithm>

// Basically from Fabian "ryg" Giesen's
// http://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// http://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/


struct Point2D
{
	int x, y;
};

static inline int orient2d(const Point2D& a, const Point2D& b, const Point2D& c)
{
	return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

template<typename T>
static inline T min3(T a, T b, T c)
{
	return std::min(a, std::min(b,c));
}

template<typename T>
static inline T max3(T a, T b, T c)
{
	return std::max(a, std::max(b,c));
}

static inline void PutPixel (int x, int y, int width, int height, unsigned char step, unsigned char* image)
{
	image[y * width + x] += step;
}


// Note: coordinates should be in [-16384,16383] range, otherwise risk overflows
// and incorrect raster.
static void RasterizeTri(
						 const Point2D& v0, const Point2D& v1, const Point2D& v2,
						 int width, int height, unsigned char step, unsigned char* image
						 )
{
	// Compute triangle bounding box
	int minX = min3(v0.x, v1.x, v2.x);
	int minY = min3(v0.y, v1.y, v2.y);
	int maxX = max3(v0.x, v1.x, v2.x);
	int maxY = max3(v0.y, v1.y, v2.y);

	// Clip against bounds
	minX = std::max(minX, 0);
	minY = std::max(minY, 0);
	maxX = std::min(maxX, width - 1);
	maxY = std::min(maxY, height - 1);
	
	// Triangle setup
	int A01 = v0.y - v1.y, B01 = v1.x - v0.x;
	int A12 = v1.y - v2.y, B12 = v2.x - v1.x;
	int A20 = v2.y - v0.y, B20 = v0.x - v2.x;

	// Barycentric coordinates at minX/minY corner
	Point2D p = { minX, minY };
	int w0_row = orient2d(v1, v2, p);
	int w1_row = orient2d(v2, v0, p);
	int w2_row = orient2d(v0, v1, p);

	// Rasterize
	for (p.y = minY; p.y <= maxY; p.y++)
	{
		// Barycentric coordinates at start of row
		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;
		
		for (p.x = minX; p.x <= maxX; p.x++)
		{
			// If p is on or inside all edges, render pixel.
			if ((w0 | w1 | w2) >= 0)
			{
				PutPixel (p.x, p.y, width, height, step, image);
			}
			
			// One step to the right
			w0 += A12;
			w1 += A20;
			w2 += A01;
		}
		
		// One row step
		w0_row += B12;
		w1_row += B20;
		w2_row += B01;
	}
}


void RasterizeCoverage(
                       int triCount,
					   const int* indices,
					   const float* vertices,
					   int width,
					   int height,
                       unsigned char step,
					   unsigned char* image
                       )
{
	for (int t = 0; t < triCount; ++t, indices += 3)
	{
		int idx0 = indices[0]*2, idx1 = indices[1]*2, idx2 = indices[2]*2;
		Point2D v0, v1, v2;
		v0.x = vertices[idx0] * width;
		v0.y = vertices[idx0+1] * height;
		v1.x = vertices[idx1] * width;
		v1.y = vertices[idx1+1] * height;
		v2.x = vertices[idx2] * width;
		v2.y = vertices[idx2+1] * height;
		
		if (orient2d(v0, v1, v2) < 0)
		{
			std::swap (v1, v2);
		}
		
		RasterizeTri(v0, v1, v2, width, height, step, image);
	}
}
