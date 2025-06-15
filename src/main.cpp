#include "glincludes.h"

#include <cassert>  // 'assert' (enforce preconditions)
#include <cstring>  // 'strlen' (to compile shaders)
#include <iostream> // 'cout' and such
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "renderer.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "vertex_buffer_layout.h"
#include "shader.h"

#include "tests/test_render.h"
#include "tests/test_free_collisions.h"
#include "tests/test_compute_shader.h"
#include "tests/test_rotation.h"
#include "tests/test_complex.h"
#include "tests/test_complex_2.h"
#include "tests/test_complex_3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#include <windows.h>
#endif

//Global variables
GLFWwindow * c_window = nullptr; /*Main Window*/

int
    w_width  = 1920, /*Window width*/
    w_height = 1080; /*Window heigth*/

bool terminate_program = false; /*Program termination*/
bool record = false;

unsigned int samples = 4; /*Number of samples*/

unsigned int skip_frames = 2;

int test_index = 0; // Update this per test, perhaps when switching tests.
int frame_index = 0; // Reset this for each test or keep a global count as needed.




void createDirectory(const std::string &path) {
    #ifdef _WIN32
        _mkdir(path.c_str());
    #endif
}

// Define a structure to hold frame data.
struct FrameData {
    int test_index;
    int frame_index;
    int width;
    int height;
    std::vector<unsigned char> pixels;
};

// Thread-safe queue
std::queue<FrameData> frameQueue;
std::mutex queueMutex;
std::condition_variable queueCV;
bool recordingActive = true;  // Control flag for background thread

// Background thread function to save frames
void frameSaver() {
    while (recordingActive || !frameQueue.empty()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [] { return !frameQueue.empty() || !recordingActive; });
        
        while (!frameQueue.empty()) {
            FrameData frame = frameQueue.front();
            frameQueue.pop();
            lock.unlock();  // Unlock while doing file I/O
            
            // Construct folder path and filename as before
            std::ostringstream folderStream;
            folderStream << "E:/datasets/REDS/sim_dataset/videos/" << std::setfill('0') << std::setw(3) << frame.test_index << "/";
            std::string folderPath = folderStream.str();
            createDirectory(folderPath);
            
            std::ostringstream filenameStream;
            filenameStream << folderPath << std::setfill('0') << std::setw(8) << frame.frame_index << ".png";
            std::string filename = filenameStream.str();
            
            if (stbi_write_png(filename.c_str(), frame.width, frame.height, 3, frame.pixels.data(), frame.width * 3))
                std::cout << "Saved frame to " << filename << std::endl;
            else
                std::cerr << "Failed to save frame to " << filename << std::endl;
            
            lock.lock();
        }
    }
}

// Modified saveFrame that pushes data to the queue instead of saving directly.
void saveFrameAsync(int test_index, int frame_index, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // Optional: flip the image vertically
    std::vector<unsigned char> flipped(pixels.size());
    int rowSize = width * 3;
    for (int y = 0; y < height; ++y) {
        memcpy(&flipped[y * rowSize], &pixels[(height - 1 - y) * rowSize], rowSize);
    }
    
    // Package the frame data
    FrameData frame { test_index, frame_index, width, height, flipped };
    
    // Push frame to the queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        frameQueue.push(frame);
    }
    queueCV.notify_one();
}



/**
 * @brief Initializes GLFW
 */
void initGLFW(int argc, char * argv[]){
    //Check for errors initializing GLFW
    if ( !glfwInit() ){
        std::cerr << "Error when initializing GLFW" << std::endl ;
        exit(1);
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 4 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE ); 
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_SAMPLES, samples );

    //Create window
    c_window = glfwCreateWindow( w_width, w_height, "Hello World", NULL, NULL );
    
    //Check for errors creating window
    if ( !c_window ){
        std::cerr << "Error when creating window" << std::endl ;
        glfwTerminate();
        exit(1);
    }

    //Make the context current
    glfwMakeContextCurrent( c_window );

    glfwSwapInterval(skip_frames); //VSync

    //Get the framebuffer size
    glfwGetFramebufferSize(c_window, &w_width, &w_height);
}

/**
 * @brief Initializes GLEW
 */
void initGLEW(){
    // Initialize GLEW
    GLenum codigoError = glewInit();

    // Check for errors
    if ( codigoError != GLEW_OK )
    {
        std::cout << "Imposible to initialize GLEW: " << std::endl
             << (const char *)glewGetErrorString( codigoError ) << std::endl ;
        exit(1);
    }
    else
        std::cout << "GLEW initialized succesfully" << std::endl << std::flush ;
}

/**
 * @brief Initializes OpenGL
 */
void initOpenGL(){
    // Initialize GLEW
    initGLEW();

    //Background color
    GLCall(glClearColor(0.24705882352941178, 0.30196078431372547, 0.3254901960784314, 1.0 )); 
    
    GLCall(glEnable(GL_MULTISAMPLE));

    // //Enable blending
    // GLCall(glEnable(GL_BLEND));

    //Draw all triangles
    GLCall(glDisable( GL_CULL_FACE ));
}

void mainLoop() {
    Renderer renderer;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(c_window, true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    test::Test* current_test = nullptr;
    test::TestMenu* test_menu = new test::TestMenu(current_test);
    current_test = test_menu;

    test_menu->registerTest<test::TestRender>("0. Render");
    test_menu->registerTest<test::TestFreeCollisions>("1. Deteccion de colisiones");
    test_menu->registerTest<test::TestComputeShader>("2. Stacking extremo");
    test_menu->registerTest<test::TestRotation>("3. Interaccion física");
    test_menu->registerTest<test::TestComplex>("4. Colisiones Complejas");
    test_menu->registerTest<test::TestComplex2>("5. Colisiones Complejas 2");
    test_menu->registerTest<test::TestComplex3>("6. Colisiones Complejas 3");
    // Variables to handle key press state
    bool r_key_pressed = false;

    // Frame rate limiting variables
    const int target_fps = 30;              // Target frames per second
    const double frame_time = 1.0 / (144.0f / skip_frames);  // Time per frame in seconds
    double last_frame_time = glfwGetTime();
    double delta_time = 0.0f;

    std::thread saverThread(frameSaver);

    GLint maxSSBOSize = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);
    printf("Max SSBO size: %d bytes\n", maxSSBOSize);
    
    while (!terminate_program) {
        // Calculate frame start time
        double frame_start_time = glfwGetTime();
        
        // Calculate time since last frame
        delta_time = frame_start_time - last_frame_time;
        last_frame_time = frame_start_time;

        // Cap delta time to prevent large jumps
        if (delta_time > 0.25f)
            delta_time = 0.25f;

        // Handle 'R' key press to toggle recording
        if (glfwGetKey(c_window, GLFW_KEY_R) == GLFW_PRESS) {
            if (!r_key_pressed) {
                // Only toggle if we're not in the test menu
                if (current_test != test_menu) {
                    record = !record;
                    std::cout << "Recording " << (record ? "started" : "stopped") << std::endl;
                    
                    // If we start recording, reset frame index
                    if (record) {
                        frame_index = 0;
                    }
                }
                r_key_pressed = true;
            }
        } else {
            r_key_pressed = false;
        }

        // GLCall(glClearColor(0.24705882352941178, 0.30196078431372547, 0.3254901960784314, 1.0 )); 
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0 )); 
        renderer.clear();

        // Always initialize ImGui for the frame, even when recording
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (current_test) {
            current_test->onUpdate(static_cast<float>(delta_time));
            current_test->onRender();
            
            ImGui::Begin("Test");
            if (current_test != test_menu && ImGui::Button("<-")) {
                // If we're recording, stop recording when going back to menu
                if (record) {
                    record = false;
                    std::cout << "Recording stopped due to test exit" << std::endl;
                }
                delete current_test;
                current_test = test_menu;
            }
            
            // Only render ImGui test content if not recording
            if (!record) {
                current_test->onImGuiRender();
            }
            ImGui::End();
        }

        // Always render ImGui, but if recording, don't display it
        ImGui::Render();
        if (!record) {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        // Save the frame if we're recording and not in the test menu
        if (record && current_test != test_menu) {
            // Make sure the current OpenGL context is ready for frame capture
            glFinish();
            saveFrameAsync(test_index, frame_index, w_width, w_height);
            frame_index++;
            if (frame_index >= 1000)
                record = false;
        }

        glfwSwapBuffers(c_window);
        glfwPollEvents();

        terminate_program = glfwWindowShouldClose(c_window) || terminate_program;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        recordingActive = false;
    }

    queueCV.notify_one();
    saverThread.join();

    delete current_test;
    if (current_test != test_menu)
        delete test_menu;
}

int main( int argc, char *argv[] )
{
    //Initialize GLFW
    initGLFW(argc, argv);

    //Initialize OpenGL
    initOpenGL();

    const GLubyte* renderer = glGetString(GL_RENDERER); // Get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // Get version string
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    //Main loop
    mainLoop();

    //Terminate ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //Terminate GLFW
    glfwTerminate();
}
