#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#pragma once

/**
 * @brief A class that stores an index buffer
 */
class IndexBuffer{
private:
    unsigned int m_renderer_id; //ID of the buffer
    unsigned int m_count; //Number of indices

public:

    /**
     * @brief Creates an empty index buffer (needs to be filled)
     */
    IndexBuffer();

    /**
     * @brief Creates a vertex buffer
     * @param data the data to be stored in the buffer
     * @param count the number of indices
     */
    IndexBuffer(const unsigned int* data, unsigned int count);
    
    /**
     * @brief Destroys the vertex buffer
     */
    ~IndexBuffer();

    /**
     * @brief Binds the buffer
     */
    void bind() const;

    /**
     * @brief Unbinds the buffer
     */
    void unbind() const;

    /**
     * @brief Gets the number of indices
     * @return the number of indices
     */
    inline unsigned int getCount() const { return m_count; }


    /**
     * @brief Sets the buffer data
     * @param data the data to be stored in the buffer
     * @param count the number of indices
     */
    void setBuffer(const unsigned int* data, unsigned int count);

};

#endif // INDEX_BUFFER_H