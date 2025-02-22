#include "index_buffer.h"
#include "../renderer.h"

#include <iostream>

IndexBuffer::IndexBuffer()
:   m_count(0){
    GLCall(glGenBuffers(1, &m_renderer_id)); //Generate buffer
}   

IndexBuffer::IndexBuffer(const unsigned int *data, unsigned int count)
:   m_count(count){
    //Check if the size of unsigned int is the same as GLuint
    ASSERT(sizeof(unsigned int) == sizeof(GLuint)); 
    
    GLCall(glGenBuffers(1, &m_renderer_id)); //Generate buffer
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id)); //Bind (select) buffer
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * count, data, GL_STATIC_DRAW)); //Fill buffer with data
}

IndexBuffer::~IndexBuffer(){
    GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
}

void IndexBuffer::bind() const{
    if (m_count == 0){
        std::cerr << "Warning: Index buffer with id "<< m_renderer_id << " is empty" << std::endl;
        return;
    }
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id)); //Bind (select) buffer
}

void IndexBuffer::unbind() const{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); //Unbind buffer
}

void IndexBuffer::setBuffer(const unsigned int *data, unsigned int count){
    if (m_count != 0){
        std::cerr << "Warning: Index buffer with id "<< m_renderer_id << " is being overwritten" << std::endl;
        GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
    }

    m_count = count;
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id)); //Bind (select) buffer
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * count, data, GL_STATIC_DRAW)); //Fill buffer with data
}

