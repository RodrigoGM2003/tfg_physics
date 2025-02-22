#include "vertex_array.h"
#include "vertex_buffer_layout.h"
#include <iostream>

VertexArray::VertexArray(){
    GLCall(glGenVertexArrays(1, &m_renderer_id)); //Generate vertex array
    GLCall(glBindVertexArray(m_renderer_id)); //Bind (select) vertex array

    m_current_index = 0;
}

VertexArray::~VertexArray(){
    GLCall(glDeleteVertexArrays(1, &m_renderer_id)); //Delete vertex array
}

void VertexArray::addBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout){
    this->bind(); //Bind the vertex array

    vb.bind(); //Bind the vertex buffer
    
    //Take the elements of the layout
    const auto& elements = layout.getElements();
    unsigned int offset = 0;

    //For each element, enable the vertex attribute and set the vertex attribute pointer
    for (const auto& element : elements){

        //Print the element information (DEBUG)
        // std::cout << "Index: " << m_current_index << ", Count: " << element.count << ", Type: " << element.type 
        // << ", Normalized: " << element.normalized << ", Stride: " << layout.getStride() 
        // << ", Offset: " << offset << ", Instanced: " << layout.isInstanced() << std::endl;

        GLCall(glEnableVertexAttribArray(m_current_index)); //Enable vertex attribute
        GLCall(glVertexAttribPointer(m_current_index, element.count, element.type, element.normalized, 
                                    layout.getStride(), (const void*)offset)); //Set vertex attribute pointer

        //Set the divisor if the layout is instanced
        if (layout.isInstanced()){
            GLCall(glVertexAttribDivisor(m_current_index, 1)); //Set the divisor to 1
        }
        
        offset += element.count * VertexBufferElement::getSizeOfType(element.type); //Update the offset
        m_current_index++;
    }
}

void VertexArray::bind() const{
    GLCall(glBindVertexArray(m_renderer_id)); //Bind (select) vertex array
}

void VertexArray::unbind() const{
    GLCall(glBindVertexArray(0)); //Unbind vertex array
}
