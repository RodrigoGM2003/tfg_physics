#include "texture.h"
#include "stb_image.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const std::string& filename)
    : m_local_buffer(nullptr), m_width(0), m_height(0), m_bpp(0) {
    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/mat/" + filename;

    // Flip images vertically because OpenGL expects the 0.0 coordinate on the y-axis to be at the bottom
    stbi_set_flip_vertically_on_load(1);
    
    // Load the image
    m_local_buffer = stbi_load(m_file_path.c_str(), &m_width, &m_height, &m_bpp, 4);
    
    if (!m_local_buffer) {
        std::cout << "Failed to load texture: " << m_file_path << std::endl;
        std::cout << "STB Error: " << stbi_failure_reason() << std::endl;
        return;
    }

    GLCall(glGenTextures(1, &m_renderer_id));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_renderer_id));

    //Set the texture wrapping/filtering options
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)); // Changed to REPEAT for noise texture
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)); // Changed to REPEAT for noise texture

    // Upload texture to GPU
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_local_buffer));
    
    // Free the local buffer as it's now on the GPU
    if (m_local_buffer) {
        stbi_image_free(m_local_buffer);
        m_local_buffer = nullptr;
    }
    
    // Unbind the texture
    unbind();
}

Texture::~Texture() {
    GLCall(glDeleteTextures(1, &m_renderer_id));
}

void Texture::bind(unsigned int slot) const {
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_renderer_id));
}

void Texture::unbind() const {
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}
