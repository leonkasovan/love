/**
 * Copyright (c) 2025 Dhani Novan
 *
 **/

#ifndef LOVE_GRAPHICS_PALETTE_H
#define LOVE_GRAPHICS_PALETTE_H

// LOVE
#include "common/Object.h"
#include "common/Color.h"

namespace love
{
namespace graphics
{

class Palette : public Object
{
public:

	static love::Type type;

	// Color32 color32[256];
	Colorf colorf[256];	// for shader

	Palette();
	Palette(const Colorf* color);
	Palette(const char* act_file);
	Palette(std::string act_file);
	~Palette();

private:

}; // Palette

} // graphics
} // love

#endif // LOVE_GRAPHICS_PALETTE_H
