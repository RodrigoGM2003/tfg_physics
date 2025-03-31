#include "texture.h"

Texture::Texture(const std::string& filename)
    : m_local_buffer(nullptr), m_width(0), m_height(0), m_bpp(0){
    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/mat/" + filename;

    GLCall(glGenTextures(1, &m_renderer_id));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_renderer_id));

    //Set the texture wrapping/filtering options
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    //Load the texture

}

Texture::~Texture(){
    GLCall(glDeleteTextures(1, &m_renderer_id));
}

void Texture::bind(unsigned int slot) const{
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_renderer_id));
}

void Texture::unbind() const{
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}
