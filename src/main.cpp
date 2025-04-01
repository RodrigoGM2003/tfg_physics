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

#include "tests/test_refactor.h"
#include "tests/test_multiple.h"
#include "tests/test_lights.h"
#include "tests/test_compute_shader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//Global variables
GLFWwindow * c_window = nullptr; /*Main Window*/

int
    w_width  = 512, /*Window width*/
    w_height = 512 ; /*Window heigth*/

bool terminate_program = false; /*Program termination*/
bool record = true;

unsigned int samples = 4; /*Number of samples*/


void createDirectory(const std::string &path) {
    #ifdef _WIN32
        _mkdir(path.c_str());
    #endif
}

void saveFrame(int test_index, int frame_index, int width, int height) {
    // Allocate memory for the pixel data (3 channels: RGB)
    std::vector<unsigned char> pixels(width * height * 3);
    // Read pixels from the framebuffer (bottom-left origin)
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // (Optional) Flip the image vertically if needed.
    // This step is often required since OpenGL's origin is bottom-left.
    std::vector<unsigned char> flipped(pixels.size());
    int rowSize = width * 3;
    for (int y = 0; y < height; ++y) {
        memcpy(&flipped[y * rowSize], &pixels[(height - 1 - y) * rowSize], rowSize);
    }
    
    // Create folder path for the current test using zero-padded test_index (e.g., "000")
    std::ostringstream folderStream;
    folderStream << "../../../out/videos/" << std::setfill('0') << std::setw(3) << test_index << "/";
    std::string folderPath = folderStream.str();
    
    // Create the directory if it doesn't exist
    createDirectory(folderPath);
    
    // Create the filename using zero-padded frame_index (e.g., "00000000.png")
    std::ostringstream filenameStream;
    filenameStream << folderPath << std::setfill('0') << std::setw(8) << frame_index << ".png";
    std::string filename = filenameStream.str();
    
    // Write the image as a PNG (ensure you have stb_image_write integrated)
    std::cout<<filename.c_str()<<std::endl;
    if(stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3)) {
        std::cout << "Saved frame to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save frame to " << filename << std::endl;
    }
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

    glfwSwapInterval(1); //VSync

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
    GLCall(glClearColor( 0.0, 0.0, 0.0, 1.0 )); 
    
    GLCall(glEnable(GL_MULTISAMPLE));

    // //Enable blending
    // GLCall(glEnable(GL_BLEND));

    //Draw all triangles
    GLCall(glDisable( GL_CULL_FACE ));
}

/**
 * @brief Simulator's main loop
 */
void mainLoop(){
    Renderer renderer;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(c_window, true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    test::Test* current_test = nullptr;
    test::TestMenu* test_menu = new test::TestMenu(current_test);
    current_test = test_menu;
    test_menu->registerTest<test::TestRefactor>("Refactor");
    test_menu->registerTest<test::TestMultiple>("Multiple");
    test_menu->registerTest<test::TestLights>("Lights");
    test_menu->registerTest<test::TestComputeShader>("ComputeShader");


    double last_frame_time = glfwGetTime();
    float delta_time = 0.0f;

    int test_index = 0; // Update this per test, perhaps when switching tests.
    int frame_index = 0; // Reset this for each test or keep a global count as needed.

    while ( !terminate_program )
    {
        double current_frame_time = glfwGetTime();
        float delta_time = static_cast<float>(current_frame_time - last_frame_time);
        last_frame_time = current_frame_time;
        if (delta_time > 0.25f)
            delta_time = 0.25f;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        renderer.clear();

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (current_test){
            current_test->onUpdate(delta_time);
            current_test->onRender();
            ImGui::Begin("Test");
            if (current_test != test_menu && ImGui::Button("<-")){
                delete current_test;
                current_test = test_menu;
            }
            current_test->onImGuiRender();
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Save the current frame using the function above.
        // Ensure 'w_width' and 'w_height' are your viewport dimensions.
        if(record)
            saveFrame(test_index, frame_index, w_width, w_height);
        ++frame_index;

        glfwSwapBuffers(c_window);
        glfwPollEvents();

        terminate_program = glfwWindowShouldClose(c_window) || terminate_program;
    }


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
