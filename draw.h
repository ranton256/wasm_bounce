#ifndef __DRAW__
#define __DRAW__

#include <stdint.h>

typedef uint32_t Pixel;

// our rect structure.
typedef struct {
	int32_t left, top, right, bottom;
} LSRect;

typedef uint8_t LSBool;

// This structure(it's a structure and not a class for a reason)
// is for easy parameter passing and other upkeep purposes
// to do with the intermediate procedures used for drawing,
// the buffer may be the screen,it may be a SpriteImage,
// or it may just be an intermediate buffer.
typedef struct {
	uint32_t id; // unique id of this graph buffer.
	Pixel *ptr; // ptr to the pixels.
	uint32_t width, height; // width and height( in use/current )
	uint32_t rowPixels; // pixels per row
	uint32_t size; // size of the buffer in bytes,
		// this doesn't need to be set for everything,
		// set it to 0 if it doesn't matter in case
		// something ever needs to know, as in width, height,
		// and rowPixels won't change during the life of the buffer.
		
	LSBool ownsPtr;// true if ptr should be deleted with the buffer.
} GraphicsBuffer;


// This creates a graph buffer, if you pass NULL
// for the memory parameter, it allocates it, and frees it with the buffer
// when you call LSDeleteGraphBuffer
GraphicsBuffer *NewGraphBuffer( 
	Pixel *memory, 
	uint32_t width,
	uint32_t height,
	uint32_t rowPixels,
	uint32_t size );


// This frees a graph buffer, and if the memory for it
// was allocated with the buffer, it frees it.
void DeleteGraphBuffer( GraphicsBuffer *buffer );

// Get byte components of pixel into *rp, *gp, and *bp
void PixelComponents(Pixel pixel, uint8_t* rp, uint8_t* gp, uint8_t* bp);

// horizontal line drawing:  x1 has to be less or same than x2
void DrawHorzLine(GraphicsBuffer *buffer, Pixel color, int32_t x1, int32_t x2, int32_t y);

// vertical line drawing: NOTE y1 has to be less than or same as y2
void DrawVertLine(GraphicsBuffer *buffer, Pixel color, int32_t y1, int32_t y2, int32_t x);

// rect drawing, does clip.
void DrawRect(GraphicsBuffer *buffer, Pixel color, int32_t left, int32_t top, int32_t right, int32_t bottom);

// rect filling, does clip.
void FillRectOpaque(GraphicsBuffer *buffer, Pixel color, int32_t left, int32_t top, int32_t right, int32_t bottom);

void DrawCircle(GraphicsBuffer *buffer, Pixel color, int32_t xCenter, int32_t yCenter, int32_t radius);

void FillCircle(GraphicsBuffer *buffer, Pixel color, int32_t xCenter, int32_t yCenter, int32_t radius);

#endif