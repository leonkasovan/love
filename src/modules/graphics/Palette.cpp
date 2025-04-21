#include "Palette.h"
#include <fstream>
#include <cstring> // For memcpy
#include "common/Exception.h"

namespace love
{
namespace graphics
{

// Define the static type member
love::Type Palette::type("Palette", &Object::type);

// Constructor: Initialize the palette with an array of Color32
Palette::Palette() {
    std::memset(color, 0, sizeof(Color32) * 256); // Default to black if null
}

Palette::Palette(const Color32* colors)
{
    // Copy the provided colors into the palette
    if (colors != nullptr)
        std::memcpy(color, colors, sizeof(Color32) * 256);
    else
        std::memset(color, 0, sizeof(Color32) * 256); // Default to black if null
}

// Constructor: Load the palette from a .act file (C-string version) and its size 768 byte (RGB format).
Palette::Palette(const char* act_file)
{
    if (act_file != nullptr)
    {
        std::ifstream file(act_file, std::ios::binary);
        if (file.is_open())
        {
			// Temporary buffer to read the 768-byte RGB data
			unsigned char buffer[768];
			file.read(reinterpret_cast<char*>(buffer), 768);

			// Ensure the file contains exactly 768 bytes
			if (file.gcount() == 768) {
				// Convert the RGB data to Color32 format
				for (int i = 0; i < 256; ++i) {
					color[i].r = buffer[i * 3];     // Red
					color[i].g = buffer[i * 3 + 1]; // Green
					color[i].b = buffer[i * 3 + 2]; // Blue
					color[i].a = i ? 255 : 0; // Set the first palette index (index 0) to transparent
				}
			} else {
				file.close();
				throw love::Exception("Invalid ACT format with size=%u. Size should be 768 bytes.", file.gcount());
			}
			file.close();
		}
        else
        {
			throw love::Exception("Failed to open ACT file: %s", act_file);
        }
    }
    else
    {
        throw love::Exception("ACT file path is null.");
    }
}

// Constructor: Load the palette from a .act file (std::string version)
Palette::Palette(std::string act_file)
    : Palette(act_file.c_str()) // Delegate to the C-string constructor
{
}

// Destructor
Palette::~Palette()
{
    // No dynamic memory to clean up, so nothing to do here
}

} // namespace graphics
} // namespace love