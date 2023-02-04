#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "draw.h"


static uint32_t sLastBufferID = 0;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

uint32_t LSCompositeValues(uint32_t a, uint32_t b, uint32_t m);
uint32_t LSCompositePixels(uint32_t sp, uint32_t dp);

// This creates a graph buffer, if you pass NULL
// for the memory parameter, it allocates it, and frees it with the buffer
// when you call LSDeleteGraphBuffer
GraphicsBuffer *NewGraphBuffer( 
	Pixel *memory, 
	uint32_t width,
	uint32_t height,
	uint32_t rowPixels,
	uint32_t size )
{	
	uint32_t id = ++sLastBufferID;
	
	GraphicsBuffer *buffer = (GraphicsBuffer*)malloc(sizeof(GraphicsBuffer));
	if( !buffer )
    {
        // TODO: better error handling.
        return NULL;
    }

	// assign a unique id.
	buffer->id = id;
	
	if( !memory  )
	{
		assert(size >= sizeof(Pixel) * width * height);
		buffer->ptr = (Pixel*)malloc(size);
		buffer->ownsPtr = TRUE;
		if( !(buffer->ptr ) ) {
            // TODO: better error handling
            return NULL;
        }
	}
	else
	{
		buffer->ptr = memory;
		buffer->ownsPtr = FALSE;
	}
	buffer->width = width;
	buffer->height = height;
	buffer->rowPixels = rowPixels;
	buffer->size = size;
	return buffer;	
}

// This frees a graph buffer, and if the memory for it
// was allocated with the buffer, it frees it.
void DeleteGraphBuffer( GraphicsBuffer *buffer )
{
	// log deletion
	if( buffer )
	{
		if( buffer->ptr && buffer->ownsPtr)
			free(buffer->ptr);

		free(buffer);
	}
}

void PixelComponents(Pixel pixel, uint8_t* rp, uint8_t* gp, uint8_t* bp)
{
	assert(rp); assert(gp); assert(bp);
	*rp = (pixel >> 24) & 0xff;
	*gp = (pixel >> 16) & 0xff;
	*bp = (pixel >> 8) & 0xff;
}

// this composites to 8 bit channel values using an alpha
uint32_t LSCompositeValues(uint32_t a, uint32_t b, uint32_t m)
{
    // >> 8 instead of 255 causes too much rounding error
    // when you mask several images in a row.
	return (m*(a-b) + 255*b) / 255;
}


// arguments are source pixel, current dest pixel.
uint32_t LSCompositePixels(uint32_t sp, uint32_t dp)
{
	uint8_t np[4],*bsrc,*bdst;
	bsrc=(uint8_t*)&sp;
	bdst=(uint8_t*)&dp;
	uint32_t  mask = bsrc[3];
	np[0] = LSCompositeValues( bsrc[0], bdst[0], mask );
	np[1] = LSCompositeValues( bsrc[1], bdst[1], mask );
	np[2] = LSCompositeValues( bsrc[2], bdst[2], mask );
	np[3] = bdst[3];

	return *(uint32_t *)np;
}


// horizontal line drawing:  x1 has to be less or same than x2
void DrawHorzLine(GraphicsBuffer *buffer, Pixel color, int32_t x1, int32_t x2, int32_t y)
{
	// check that the line is on the screen
	if( y >= 0 && y < buffer->height && x2 >= 0 && x1 < (int32_t)buffer->width)
	{
		// clip x coordinates to the screen.
		if( x1 < 0 )
			x1 = 0;
		if( x2 >= buffer->width )
			x2 = buffer->width - 1;
		
		// get the buffer.
		uint32_t rowPixels = buffer->rowPixels;
		Pixel *pix = buffer->ptr;
		pix += y * rowPixels;
		pix += x1;
		int32_t count = x2 - x1 + 1;
		while(count--)
		{
			Pixel pval = *pix;
			*pix++ = LSCompositePixels( color, pval );
		}
		// that's it.
	}
}


// vertical line drawing: NOTE y1 has to be less than or same as y2
void DrawVertLine(GraphicsBuffer *buffer, Pixel color, int32_t y1, int32_t y2, int32_t x)
{
	// check that the line is on the screen
	if( x >= 0 && x < buffer->width && y2 >= 0 && y1 < buffer->height )
	{
		if( y1 < 0 )
			y1 = 0;
		if( y2 >= buffer->height )
			y2 = buffer->height - 1;

		uint32_t rowPixels= buffer->rowPixels;
		Pixel *pix = buffer->ptr;//LSGetDoubleBuffer(&rowPixels);
		pix += y1 * rowPixels;
		pix += x;
		int32_t count = y2 - y1 + 1;
		while(count--)
		{
			// *pix = color;
			*pix = LSCompositePixels( color, *pix );
			pix += rowPixels;
		}
	}
}


// rect drawing, does clip.
void DrawRect(GraphicsBuffer *buffer, Pixel color, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	// not normal for rects to include
	// the pixels on the right and bottom edge,
	// but the functions we call do, so dec them.
	bottom--;
	right--;
	
	DrawHorzLine(buffer, color, left, right, top );
	DrawHorzLine(buffer, color, left, right, bottom );
	
	// don't draw the 4 corner pixels twice.
	top++;
	bottom--;
	if( top <= bottom )
	{
		DrawVertLine(buffer, color,top,bottom,left);
		DrawVertLine(buffer, color,top,bottom,right);
	}
	
}

// rect filling, does clip.
void FillRectOpaque(GraphicsBuffer *buffer, Pixel color, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	Pixel *pix;
	int32_t hCount;
	Pixel *row;
	int32_t width, height;
	uint32_t rowPixels = buffer->rowPixels;
	
	// check that it is on the screen
	if( bottom >= 0 && top < buffer->height && right >= 0 && left < buffer->width)
	{
		// clip.
		if( left < 0 )
			left = 0;
		if( top < 0 )
			top = 0;
		if( right > buffer->width )
			right = buffer->width; // - 1
		if( bottom > buffer->height )
			bottom = buffer->height; // - 1

		height = bottom - top;
		width = right - left;
		row = buffer->ptr;
		row += top * rowPixels; // go to the top.
		row += left; // go to the left side.
		while( height-- )
		{
			pix = row;
			hCount = width;
			while(hCount--)
				*pix++ = color;
			
			row += rowPixels;
		}
	}
	
}

// no transparency version
#define LSSetPixelOpaque(buffer,x,y,color) \
	if(x<buffer->width && y<buffer->height && x>0 && y>0) \
		{ buffer->ptr[(buffer->rowPixels)*(y)+(x)] = (color); }

// handles transparency
#define LSSetPixel(buffer,x,y,color) \
	if(x<buffer->width && y<buffer->height && x>0 && y>0) \
		{ buffer->ptr[(buffer->rowPixels)*(y)+(x)] = LSCompositePixels((color), buffer->ptr[(buffer->rowPixels)*(y)+(x)]); }



// This is a macro used by LSDrawCircle
#define _PlotCirclePoints(x,y)  \
	LSSetPixel(buffer, xCenter + x, yCenter + y, color ); \
	LSSetPixel(buffer, xCenter - x, yCenter + y, color ); \
	LSSetPixel(buffer, xCenter + x, yCenter - y, color ); \
	LSSetPixel(buffer, xCenter - x, yCenter - y, color ); \
	LSSetPixel(buffer, xCenter + y, yCenter + x, color ); \
	LSSetPixel(buffer, xCenter - y, yCenter + x, color ); \
	LSSetPixel(buffer, xCenter + y, yCenter - x, color ); \
	LSSetPixel(buffer, xCenter - y, yCenter - x, color )



// Draw a circle
void DrawCircle(GraphicsBuffer *buffer, Pixel color, int32_t xCenter, int32_t yCenter, int32_t radius)
{
	// f_circle(x,y) = x*x + y*y - r*r
	// negative if interior, positive if outside, 0 if on boundary.
	
	// we do an increment using 0,0 as origin,
	// for each next pixel in the calc. octant from x = 0, to x= y
	// we determine if the next pixel should be 
	// after xk,yk, the choices are xk+1,yk and xk+1,yk-1
	// We decide based on the circle function above evaluated
	// for the midpoint between these points.
	
	
	// pk(decision parameter) = f_circle(xk + 1, yk - 1/2)
	// = (xk + 1)^2 + (yk-1/2)^2 - r^2
	
	// if pk is negative yk is closer to circle boundary,
	// otherwise the mid-position is outside or on the pixel boundary
	// and we select the pixel on scanline yk-1
	
	// the incremental version of pk is
	// pk+1 = f_circle( xk+1, yk+1 - 1/2)
	// = [ (xk + 1) + 1  ]^2 + (yk+1 - 1/2)^2 - r^2
	// OR
	// pk+1 = pk + 2(xk + 1) + ( (yk+1)^2 - yk^2) - (yk+1 - yk) + 1
	
	// RqUns32 rowPixels = buffer->rowPixels;
	// Pixel *dest = buffer->ptr;
	
	
	int32_t p, x, y;

	x = 0;
	y = radius;
	// plot the first points.
	_PlotCirclePoints( x, y );
	p = 1 - radius;
	while( x < y )
	{
		if( p < 0 )
		{
			x++;
			p = p + 2 * x + 1;
		}
		else
		{
			x++;
			y--;
			p = p + 2 *(x-y) + 1;
		}
		_PlotCirclePoints( x, y );
	}
}

void FillCircle(GraphicsBuffer *buffer, Pixel color, int32_t xCenter, int32_t yCenter, int32_t radius)
{
	int32_t p, x, y;
	
	x = 0;
	y = radius;
	// Draw the first line.
	DrawHorzLine(buffer, color, xCenter - x, xCenter + x, yCenter + y );
	DrawHorzLine(buffer, color,  xCenter - x, xCenter + x, yCenter - y );
	DrawHorzLine(buffer, color,  xCenter - y, xCenter + y, yCenter + x );
	DrawHorzLine(buffer, color,  xCenter - y, xCenter + y, yCenter - x );

	p = 1 - radius;
	while( x < y )
	{
		if( p < 0 )
		{
			x++;
			p = p + 2 * x + 1;
		}
		else
		{
			x++;
			y--;
			p = p + 2 *(x-y) + 1;
		}
		
		// 4 lines each time
		DrawHorzLine(buffer, color, xCenter - x, xCenter + x, yCenter + y );
		DrawHorzLine(buffer, color,  xCenter - x, xCenter + x, yCenter - y );
		DrawHorzLine(buffer, color,  xCenter - y, xCenter + y, yCenter + x );
		DrawHorzLine(buffer, color,  xCenter - y, xCenter + y, yCenter - x );
		
	}
}

