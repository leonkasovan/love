#include "MugenSprite.h"
#include <fstream>
#include <iostream> // For debugging/logging (optional)

namespace love
{
namespace graphics
{

// Define the static type member
love::Type MugenSprite::type("MugenSprite", &Object::type);

// Constructor: Load the sprite from an SFF file (C-string version)
MugenSprite::MugenSprite(const char* sff_file)
{
    if (sff_file != nullptr)
    {
        std::ifstream file(sff_file, std::ios::binary);
        if (file.is_open())
        {
            // TODO: Implement logic to parse the SFF file format
            std::cout << "Successfully opened SFF file: " << sff_file << std::endl;

            // Example: Read and process the file content
            // file.read(...);

            file.close();
        }
        else
        {
            std::cerr << "Failed to open SFF file: " << sff_file << std::endl;
        }
    }
    else
    {
        std::cerr << "SFF file path is null!" << std::endl;
    }
}

// Constructor: Load the sprite from an SFF file (std::string version)
MugenSprite::MugenSprite(std::string sff_file)
    : MugenSprite(sff_file.c_str()) // Delegate to the C-string constructor
{
}

// Destructor
MugenSprite::~MugenSprite()
{
    // TODO: Clean up any allocated resources if necessary
}

} // namespace graphics
} // namespace love