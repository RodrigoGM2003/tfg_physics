#ifndef TEXTURE_H
#define TEXTURE_H

#pragma once

#include <string>
#include "renderer.h"


/**
 * @brief Class to manage textures
 */
class Texture
{
private:
    unsigned int m_renderer_id; // Texture ID
    std::string m_file_path; // File path
    unsigned char * m_local_buffer; // Image data
    int m_width, m_height, m_bpp; // Image properties
public:
    /**
     * @brief Constructor
     * @param filename Path to the image file
     */
    Texture(const std::string& filename);
    
    /**
     * @brief Destructor
     */
    ~Texture();

    /**
     * @brief Bind the texture
     * @param slot Texture slot
     */
    void bind(unsigned int slot = 0) const;

    /**
     * @brief Unbind the texture
     */
    void unbind() const;

    /**
     * @brief Get the width of the texture
     * @return Width of the texture
     */
    inline int getWidth() const { return m_width; }

    /**
     * @brief Get the height of the texture
     * @return Height of the texture
     */
    inline int getHeight() const { return m_height; }
};



#endif // TEXTURE_H