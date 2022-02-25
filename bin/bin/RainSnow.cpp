/*
Es05a: shadow rendering with shadow mapping technique
- swapping (pressing keys from 1 to 3) between basic shadow mapping (with a lot of aliasing/shadow "acne"), adaptive bias to avoid shadow "acne", and PCF to smooth shadow borders

N.B. 1)
In this example we use Shaders Subroutines to do shader swapping:
http://www.geeks3d.com/20140701/opengl-4-shader-subroutines-introduction-3d-programming-tutorial/
https://www.lighthouse3d.com/tutorials/glsl-tutorial/subroutines/
https://www.khronos.org/opengl/wiki/Shader_Subroutine

In other cases, an alternative could be to consider Separate Shader Objects:
https://www.informit.com/articles/article.aspx?p=2731929&seqNum=7
https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs
https://riptutorial.com/opengl/example/26979/load-separable-shader-in-cplusplus

N.B. 2) the application considers only a directional light. In case of more lights, and/or of different nature, the code must be modifies

N.B. 3)
see :
https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#basic-shadowmap
https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/common-techniques-to-improve-shadow-depth-maps
for further details


author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/


// Std. Includes
#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
#include <utils/shader_v1.h>
#include <utils/model_v1.h>
#include <utils/camera.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>



#define MAX_PARTICLES 1000
#define WCX		640
#define WCY		480
#define RAIN	0
#define SNOW	1
#define	HAIL	2

float slowdown = 2.0;
float velocity = 0.0;
float zoom = -40.0;
float pan = 0.0;
float tilt = 0.0;
float hailsize = 0.1;

int loop;
int fall;

//floor colors
float r = 0.0;
float g = 1.0;
float b = 0.0;
float ground_points[21][21][3];
float ground_colors[21][21][4];
float accum = -10.0;


// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// the rendering steps used in the application
enum render_passes{ SHADOWMAP, RENDER};

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
vector<std::string> shaders;

// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);

// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);

// in this application, we have isolated the models rendering using a function, which will be called in each rendering step
void RenderObjects(Shader &shader, Model &planeModel, Model &benchModel, Model &lampModel, Model &treeModel, GLint render_pass, GLuint depthMap);

// load image from disk and create an OpenGL texture
GLint LoadTexture(const char* path);

// we initialize an array of booleans for each keybord key
bool keys[1024];

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// View matrix: the camera moves, so we just set to indentity now
glm::mat4 view = glm::mat4(1.0f);

// Model and Normal transformation matrices for the objects in the scene: we set to identity
glm::mat4 lampModelMatrix = glm::mat4(1.0f);
glm::mat3 lampNormalMatrix = glm::mat3(1.0f);
glm::mat4 benchModelMatrix = glm::mat4(1.0f);
glm::mat3 benchNormalMatrix = glm::mat3(1.0f);
glm::mat4 treeModelMatrix = glm::mat4(1.0f);
glm::mat3 treeNormalMatrix = glm::mat3(1.0f);
glm::mat4 planeModelMatrix = glm::mat4(1.0f);
glm::mat3 planeNormalMatrix = glm::mat3(1.0f);

// we create a camera. We pass the initial position as a paramenter to the constructor. The last boolean tells if we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_TRUE);

// in this example, we consider a directional light. We pass the direction of incoming light as an uniform to the shaders
glm::vec3 lightDir0 = glm::vec3(1.0f, 1.0f, 1.0f);

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// vector for the textures IDs
vector<GLint> textureID;

// UV repetitions
GLfloat repeat = 1.0;

/////////////////// MAIN function ///////////////////////
int main()
{
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_lecture05a", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // we create the Shader Program for the creation of the shadow map
    Shader shadow_shader("19_shadowmap.vert", "20_shadowmap.frag");
    // we create the Shader Program used for objects (which presents different subroutines we can switch)
    Shader illumination_shader = Shader("21_ggx_tex_shadow.vert", "22_ggx_tex_shadow.frag");

    // we parse the Shader Program to search for the number and names of the subroutines.
    // the names are placed in the shaders vector
    SetupShader(illumination_shader.Program);
    // we print on console the name of the first subroutine used
    PrintCurrentShader(current_subroutine);

    // we load the images and store them in a vector
    textureID.push_back(LoadTexture("../../textures/UV_Grid_Sm.png"));
    textureID.push_back(LoadTexture("../../textures/SoilCracked.png"));
    textureID.push_back(LoadTexture("../../textures/bark_0021.jpg"));
    // textureID.push_back(LoadTexture("../../textures/DB2X2_L01_Nor.png"));
    // textureID.push_back(LoadTexture("../../textures/DB2X2_L01.png"));

    // we load the model(s) (code of Model class is in include/utils/model_v2.h)
    Model benchModel("../../models/bench.obj");
    Model lampModel("../../models/Lamp.obj");
    Model treeModel("../../models/Tree.obj");
    Model planeModel("../../models/plane.obj");

    /////////////////// CREATION OF BUFFER FOR THE  DEPTH MAP /////////////////////////////////////////
    // buffer dimension: too large -> performance may slow down if we have many lights; too small -> strong aliasing
    const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    GLuint depthMapFBO;
    // we create a Frame Buffer Object: the first rendering step will render to this buffer, and not to the real frame buffer
    glGenFramebuffers(1, &depthMapFBO);
    // we create a texture for the depth map
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    // in the texture, we will save only the depth data of the fragments. Thus, we specify that we need to render only depth in the first rendering step
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // we set to clamp the uv coordinates outside [0,1] to the color of the border
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // outside the area covered by the light frustum, everything is rendered in shadow (because we set GL_CLAMP_TO_BORDER)
    // thus, we set the texture border to white, so to render correctly everything not involved by the shadow map
    //*************
    GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // we bind the depth map FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // we set that we are not calculating nor saving color data
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ///////////////////////////////////////////////////////////////////


    // Projection matrix of the camera: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();
        // we apply FPS camera movements
        apply_camera_movements();

        /////////////////// STEP 1 - SHADOW MAP: RENDERING OF SCENE FROM LIGHT POINT OF VIEW ////////////////////////////////////////////////
        // we set view and projection matrix for the rendering using light as a camera
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        GLfloat near_plane = -10.0f, far_plane = 10.0f;
        GLfloat frustumSize = 5.0f;
        // for a directional light, the projection is orthographic. For point lights, we should use a perspective projection
        lightProjection = glm::ortho(-frustumSize, frustumSize, -frustumSize, frustumSize, near_plane, far_plane);
        // the light is directional, so technically it has no position. We need a view matrix, so we consider a position on the the direction vector of the light
        lightView = glm::lookAt(lightDir0, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // transformation matrix for the light
        lightSpaceMatrix = lightProjection * lightView;
        /// We "install" the  Shader Program for the shadow mapping creation
        shadow_shader.Use();
        // we pass the transformation matrix as uniform
        glUniformMatrix4fv(glGetUniformLocation(shadow_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        // we set the viewport for the first rendering step = dimensions of the depth texture
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        // we activate the FBO for the depth map rendering
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // we render the scene, using the shadow shader
        RenderObjects(shadow_shader, planeModel, benchModel, lampModel, treeModel, SHADOWMAP, depthMap);

        /////////////////// STEP 2 - SCENE RENDERING FROM CAMERA ////////////////////////////////////////////////

        // we get the view matrix from the Camera class
        view = camera.GetViewMatrix();

        // we activate back the standard Frame Buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        

        // we set the viewport for the final rendering step
        glViewport(0, 0, width, height);

        // We "install" the selected Shader Program as part of the current rendering process. We pass to the shader the light transformation matrix, and the depth map rendered in the first rendering step
        illumination_shader.Use();
         // we search inside the Shader Program the name of the subroutine currently selected, and we get the numerical index
        GLuint index = glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, shaders[current_subroutine].c_str());
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // we determine the position in the Shader Program of the uniform variables
        GLint lightDirLocation = glGetUniformLocation(illumination_shader.Program, "lightVector");
        GLint kdLocation = glGetUniformLocation(illumination_shader.Program, "Kd");
        GLint alphaLocation = glGetUniformLocation(illumination_shader.Program, "alpha");
        GLint f0Location = glGetUniformLocation(illumination_shader.Program, "F0");

        // we assign the value to the uniform variables
        glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir0));
        glUniform1f(kdLocation, Kd);
        glUniform1f(alphaLocation, alpha);
        glUniform1f(f0Location, F0);

        // we render the scene
        RenderObjects(illumination_shader, planeModel, benchModel, lampModel, treeModel, RENDER, depthMap);

        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    illumination_shader.Delete();
    shadow_shader.Delete();
    // chiudo e cancello il contesto creato
    glfwTerminate();
    return 0;
}


//////////////////////////////////////////
// we render the objects. We pass also the current rendering step, and the depth map generated in the first step, which is used by the shaders of the second step
void RenderObjects(Shader &shader, Model &planeModel, Model &benchModel, Model &lampModel, Model &treeModel, GLint render_pass, GLuint depthMap)
{
    // For the second rendering step -> we pass the shadow map to the shaders
    if (render_pass==RENDER)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        GLint shadowLocation = glGetUniformLocation(shader.Program, "shadowMap");
        glUniform1i(shadowLocation, 2);
    }
    // we pass the needed uniforms
    GLint textureLocation = glGetUniformLocation(shader.Program, "tex");
    GLint repeatLocation = glGetUniformLocation(shader.Program, "repeat");

    // PLANE
    // we activate the texture of the plane
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureID[1]);
    glUniform1i(textureLocation, 1);
    glUniform1f(repeatLocation, 80.0);

    /*
      we create the transformation matrix

      N.B.) the last defined is the first applied

      We need also the matrix for normals transformation, which is the inverse of the transpose of the 3x3 submatrix (upper left) of the modelview. We do not consider the 4th column because we do not need translations for normals.
      An explanation (where XT means the transpose of X, etc):
        "Two column vectors X and Y are perpendicular if and only if XT.Y=0. If We're going to transform X by a matrix M, we need to transform Y by some matrix N so that (M.X)T.(N.Y)=0. Using the identity (A.B)T=BT.AT, this becomes (XT.MT).(N.Y)=0 => XT.(MT.N).Y=0. If MT.N is the identity matrix then this reduces to XT.Y=0. And MT.N is the identity matrix if and only if N=(MT)-1, i.e. N is the inverse of the transpose of M.
    */
    // we reset to identity at each frame
    planeModelMatrix = glm::mat4(1.0f);
    planeNormalMatrix = glm::mat3(1.0f);
    planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
    planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
    planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));
    // we render the plane
    planeModel.Draw();

    // lamp
    // we activate the texture of the object
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID[0]);
    glUniform1i(textureLocation, 0);
    glUniform1f(repeatLocation, repeat);

    // we reset to identity at each frame
    lampModelMatrix = glm::mat4(1.0f);
    lampNormalMatrix = glm::mat3(1.0f);
    lampModelMatrix = glm::translate(lampModelMatrix, glm::vec3(-3.0f, -1.0f, 3.0f));
    lampModelMatrix = glm::rotate(lampModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
    lampModelMatrix = glm::scale(lampModelMatrix, glm::vec3(0.25f, 0.25f, 0.25f));
    lampNormalMatrix = glm::inverseTranspose(glm::mat3(view*lampModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(lampModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(lampNormalMatrix));

    // we render the lamp
    lampModel.Draw();
    

    // bench

        
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID[0]);
    glUniform1i(textureLocation, 0);
    glUniform1f(repeatLocation, repeat);
    // we reset to identity at each frame
    benchModelMatrix = glm::mat4(1.0f);
    benchNormalMatrix = glm::mat3(1.0f);
    benchModelMatrix = glm::translate(benchModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
    benchModelMatrix = glm::rotate(benchModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
    benchModelMatrix = glm::scale(benchModelMatrix, glm::vec3(0.01f, 0.01f, 0.01f));
    benchNormalMatrix = glm::inverseTranspose(glm::mat3(view*benchModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(benchModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(benchNormalMatrix));

    // we render the bench
    benchModel.Draw();

    // tree

        
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textureID[2]);
    glUniform1i(textureLocation, 3);
    glUniform1f(repeatLocation, repeat);

    // we reset to identity at each frame
    treeModelMatrix = glm::mat4(1.0f);
    treeNormalMatrix = glm::mat3(1.0f);
    treeModelMatrix = glm::translate(treeModelMatrix, glm::vec3(5.0f, -1.0f, 5.0f));
    treeModelMatrix = glm::rotate(treeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
    treeModelMatrix = glm::scale(treeModelMatrix, glm::vec3(1.5f, 1.5f, 1.5f));
    treeNormalMatrix = glm::inverseTranspose(glm::mat3(view*treeModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(treeModelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(treeNormalMatrix));
    
    // we render the tree
    treeModel.Draw();

}

//////////////////////////////////////////
// we load the image from disk and we create an OpenGL texture
GLint LoadTexture(const char* path)
{
    GLuint textureImage;
    int w, h, channels;
    unsigned char* image;
    image = stbi_load(path, &w, &h, &channels, STBI_rgb);

    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;

    glGenTextures(1, &textureImage);
    glBindTexture(GL_TEXTURE_2D, textureImage);
    // 3 channels = RGB ; 4 channel = RGBA
    if (channels==3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels==4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    // we set how to consider UVs outside [0,1] range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);

    // we set the binding to 0 once we have finished
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureImage;
}

///////////////////////////////////////////
// The function parses the content of the Shader Program, searches for the Subroutine type names,
// the subroutines implemented for each type, print the names of the subroutines on the terminal, and add the names of
// the subroutines to the shaders vector, which is used for the shaders swapping
void SetupShader(int program)
{
    int maxSub,maxSubU,countActiveSU;
    GLchar name[256];
    int len, numCompS;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++) {

        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int *s =  new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j=0; j < numCompS; ++j) {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            std::cout << "\t" << s[j] << " - " << name << "\n";
            shaders.push_back(name);
        }
        std::cout << std::endl;

        delete[] s;
    }
}

/////////////////////////////////////////
// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    std::cout << "Current shader subroutine: " << shaders[subroutine]  << std::endl;
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    GLuint new_subroutine;

    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // pressing a key number, we change the shader applied to the models
    // if the key is between 1 and 9, we proceed and check if the pressed key corresponds to
    // a valid subroutine
    if((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) && action == GLFW_PRESS)
    {
        // "1" to "9" -> ASCII codes from 49 to 59
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 9
        // we subtract 1 to have indices from 0 to 8
        new_subroutine = (key-'0'-1);
        // if the new index is valid ( = there is a subroutine with that index in the shaders vector),
        // we change the value of the current_subroutine variable
        // NB: we can just check if the new index is in the range between 0 and the size of the shaders vector,
        // avoiding to use the std::find function on the vector
        if (new_subroutine<shaders.size())
        {
            current_subroutine = new_subroutine;
            PrintCurrentShader(current_subroutine);
        }
    }

    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
      // we move the camera view following the mouse cursor
      // we calculate the offset of the mouse cursor from the position in the last frame
      // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
      if(firstMouse)
      {
          lastX = xpos;
          lastY = ypos;
          firstMouse = false;
      }

      // offset of mouse cursor position
      GLfloat xoffset = xpos - lastX;
      GLfloat yoffset = lastY - ypos;

      // the new position will be the previous one for the next frame
      lastX = xpos;
      lastY = ypos;

      // we pass the offset to the Camera class instance in order to update the rendering
      camera.ProcessMouseMovement(xoffset, yoffset);

}

//---------------------------------------------------------------------------


typedef struct {
  // Life
  bool alive;	// is the particle alive?
  float life;	// particle lifespan
  float fade; // decay
  // color
  float red;
  float green;
  float blue;
  // Position/direction
  float xpos;
  float ypos;
  float zpos;
  // Velocity/Direction, only goes down in y dir
  float vel;
  // Gravity
  float gravity;
}particles;

// Paticle System
particles par_sys[MAX_PARTICLES];

void normal_keys(unsigned char key, int x, int y) {
  if (key == 'r') { // Rain
    fall = RAIN;
   // glutPostRedisplay();
  }
  if (key == 'h') { // Hail
    fall = HAIL;
   // glutPostRedisplay();
  }
  if (key == 's') { // Snow
    fall = SNOW;
   // glutPostRedisplay();
  }
  if (key == '=') { //really '+' - make hail bigger
    hailsize += 0.01;
  }
  if (key == '-') { // make hail smaller
    if (hailsize > 0.1) hailsize -= 0.01;
  }
  if (key == ',') { // really '<' slow down
    if (slowdown > 4.0) slowdown += 0.01;
  }
  if (key == '.') { // really '>' speed up
    if (slowdown > 1.0) slowdown -= 0.01;
  }
  if (key == 'q') { // QUIT
    exit(0);
  }
}

void special_keys(int key, int x, int y) {

}


// Initialize/Reset Particles - give them their attributes
void initParticles(int i) {
    par_sys[i].alive = true;
    par_sys[i].life = 1.0;
    par_sys[i].fade = float(rand()%100)/1000.0f+0.003f;

    par_sys[i].xpos = (float) (rand() % 21) - 10;
    par_sys[i].ypos = 10.0;
    par_sys[i].zpos = (float) (rand() % 21) - 10;

    par_sys[i].red = 0.5;
    par_sys[i].green = 0.5;
    par_sys[i].blue = 1.0;

    par_sys[i].vel = velocity;
    par_sys[i].gravity = -0.8;//-0.8;

}

// void init( ) {
//   int x, z;

//     glShadeModel(GL_SMOOTH);
//     glClearColor(0.0, 0.0, 0.0, 0.0);
//     glClearDepth(1.0);
//     glEnable(GL_DEPTH_TEST);

//   // Ground Verticies
//     // Ground Colors
//     for (z = 0; z < 21; z++) {
//       for (x = 0; x < 21; x++) {
//         ground_points[x][z][0] = x - 10.0;
//         ground_points[x][z][1] = accum;
//         ground_points[x][z][2] = z - 10.0;

//         ground_colors[z][x][0] = r; // red value
//         ground_colors[z][x][1] = g; // green value
//         ground_colors[z][x][2] = b; // blue value
//         ground_colors[z][x][3] = 0.0; // acummulation factor
//       }
//     }

//     // Initialize particles
//     for (loop = 0; loop < MAX_PARTICLES; loop++) {
//         initParticles(loop);
//     }
// }

// // For Rain
// void drawRain() {
//   float x, y, z;
//   for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
//     if (par_sys[loop].alive == true) {
//       x = par_sys[loop].xpos;
//       y = par_sys[loop].ypos;
//       z = par_sys[loop].zpos + zoom;

//       // Draw particles
//       glColor3f(0.5, 0.5, 1.0);
//       glBegin(GL_LINES);
//         glVertex3f(x, y, z);
//         glVertex3f(x, y+0.5, z);
//       glEnd();

//       // Update values
//       //Move
//       // Adjust slowdown for speed!
//       par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000);
//       par_sys[loop].vel += par_sys[loop].gravity;
//       // Decay
//       par_sys[loop].life -= par_sys[loop].fade;

//       if (par_sys[loop].ypos <= -10) {
//         par_sys[loop].life = -1.0;
//       }
//       //Revive
//       if (par_sys[loop].life < 0.0) {
//         initParticles(loop);
//       }
//     }
//   }
// }

// // For Hail
// void drawHail() {
//   float x, y, z;

//   for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
//     if (par_sys[loop].alive == true) {
//       x = par_sys[loop].xpos;
//       y = par_sys[loop].ypos;
//       z = par_sys[loop].zpos + zoom;

//       // Draw particles
//       glColor3f(0.8, 0.8, 0.9);
//       glBegin(GL_QUADS);
//         // Front
//         glVertex3f(x-hailsize, y-hailsize, z+hailsize); // lower left
//         glVertex3f(x-hailsize, y+hailsize, z+hailsize); // upper left
//         glVertex3f(x+hailsize, y+hailsize, z+hailsize); // upper right
//         glVertex3f(x+hailsize, y-hailsize, z+hailsize); // lower left
//         //Left
//         glVertex3f(x-hailsize, y-hailsize, z+hailsize);
//         glVertex3f(x-hailsize, y-hailsize, z-hailsize);
//         glVertex3f(x-hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x-hailsize, y+hailsize, z+hailsize);
//         // Back
//         glVertex3f(x-hailsize, y-hailsize, z-hailsize);
//         glVertex3f(x-hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y-hailsize, z-hailsize);
//         //Right
//         glVertex3f(x+hailsize, y+hailsize, z+hailsize);
//         glVertex3f(x+hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y-hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y-hailsize, z+hailsize);
//         //Top
//         glVertex3f(x-hailsize, y+hailsize, z+hailsize);
//         glVertex3f(x-hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y+hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y+hailsize, z+hailsize);
//         //Bottom
//         glVertex3f(x-hailsize, y-hailsize, z+hailsize);
//         glVertex3f(x-hailsize, y-hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y-hailsize, z-hailsize);
//         glVertex3f(x+hailsize, y-hailsize, z+hailsize);
//       glEnd();

//       // Update values
//       //Move
//       if (par_sys[loop].ypos <= -10) {
//         par_sys[loop].vel = par_sys[loop].vel * -1.0;
//       }
//       par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000); // * 1000
//       par_sys[loop].vel += par_sys[loop].gravity;

//       // Decay
//       par_sys[loop].life -= par_sys[loop].fade;

//       //Revive
//       if (par_sys[loop].life < 0.0) {
//         initParticles(loop);
//       }
//     }
//   }
// }

// // For Snow
// void drawSnow() {
//   float x, y, z;
//   for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
//     if (par_sys[loop].alive == true) {
//       x = par_sys[loop].xpos;
//       y = par_sys[loop].ypos;
//       z = par_sys[loop].zpos + zoom;

//       // Draw particles
//       glColor3f(1.0, 1.0, 1.0);
//       glPushMatrix();
//       glTranslatef(x, y, z);
//    //   glutSolidSphere(0.2, 16, 16);
//       glPopMatrix();

//       // Update values
//       //Move
//       par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000);
//       par_sys[loop].vel += par_sys[loop].gravity;
//       // Decay
//       par_sys[loop].life -= par_sys[loop].fade;

//       if (par_sys[loop].ypos <= -10) {
//         int zi = z - zoom + 10;
//         int xi = x + 10;
//         ground_colors[zi][xi][0] = 1.0;
//         ground_colors[zi][xi][2] = 1.0;
//         ground_colors[zi][xi][3] += 1.0;
//         if (ground_colors[zi][xi][3] > 1.0) {
//           ground_points[xi][zi][1] += 0.1;
//         }
//         par_sys[loop].life = -1.0;
//       }

//       //Revive
//       if (par_sys[loop].life < 0.0) {
//         initParticles(loop);
//       }
//     }
//   }
// }

// // Draw Particles
// void drawScene( ) {
//   int i, j;
//   float x, y, z;

//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glMatrixMode(GL_MODELVIEW);

//   glLoadIdentity();


//   glRotatef(pan, 0.0, 1.0, 0.0);
//   glRotatef(tilt, 1.0, 0.0, 0.0);

//   // GROUND?!
//   glColor3f(r, g, b);
//   glBegin(GL_QUADS);
//     // along z - y const
//     for (i = -10; i+1 < 11; i++) {
//       // along x - y const
//       for (j = -10; j+1 < 11; j++) {
//         glColor3fv(ground_colors[i+10][j+10]);
//         glVertex3f(ground_points[j+10][i+10][0],
//               ground_points[j+10][i+10][1],
//               ground_points[j+10][i+10][2] + zoom);
//         glColor3fv(ground_colors[i+10][j+1+10]);
//         glVertex3f(ground_points[j+1+10][i+10][0],
//               ground_points[j+1+10][i+10][1],
//               ground_points[j+1+10][i+10][2] + zoom);
//         glColor3fv(ground_colors[i+1+10][j+1+10]);
//         glVertex3f(ground_points[j+1+10][i+1+10][0],
//               ground_points[j+1+10][i+1+10][1],
//               ground_points[j+1+10][i+1+10][2] + zoom);
//         glColor3fv(ground_colors[i+1+10][j+10]);
//         glVertex3f(ground_points[j+10][i+1+10][0],
//               ground_points[j+10][i+1+10][1],
//               ground_points[j+10][i+1+10][2] + zoom);
//       }

//     }
//   glEnd();
//   // Which Particles
//   if (fall == RAIN) {
//     drawRain();
//   }else if (fall == HAIL) {
//     drawHail();
//   }else if (fall == SNOW) {
//     drawSnow();
//   }

// }

// void reshape(int w, int h) {
//     if (h == 0) h = 1;

//     glViewport(0, 0, w, h);
//     glMatrixMode(GL_PROJECTION);
//     glLoadIdentity();

//     //gluPerspective(45, (float) w / (float) h, .1, 200);

//     glMatrixMode(GL_MODELVIEW);
//     glLoadIdentity();
// }
