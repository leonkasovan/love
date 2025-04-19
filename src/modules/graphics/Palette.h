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

	Color32 color[256];

	Palette(const Color32* color);
	Palette(const char* act_file);
	Palette(std::string act_file);
	~Palette();

private:

}; // Palette

} // graphics
} // love

#endif // LOVE_GRAPHICS_PALETTE_H
