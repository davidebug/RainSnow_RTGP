/*
work03

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

// Loader estensions OpenGL
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

// classes developed during lab lectures to manage shaders and to load models
#include <utils/shader_v1.h>
#include <utils/model_v1.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
vector<std::string> shaders;

// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);

// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);

// parameters for time computation
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
GLboolean spinning = GL_TRUE;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// Uniforms to pass to shaders
// frequency and power parameters for noise generation (for all subroutines)
GLfloat frequency = 10.0;
GLfloat power = 1.0;
// number of harmonics (used in the turbulence-based subroutines)
GLfloat harmonics = 4.0;
// velocity for noise animation effect (for the animated subroutine)
GLfloat speed = 5.0;

// color to be passed as uniform to the shader of the plane
GLfloat planeColor[] = {0.0,0.5,0.0};

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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_work03", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);

    // we disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // we create the Shader Program used for the plane
    Shader plane_shader("00_basic.vert", "01_fullcolor.frag");

    // we create the Shader Program used for objects (which presents different subroutines we can switch)
    Shader noise_shader = Shader("06_procedural_base.vert", "08_random_patterns.frag");
    // we parse the Shader Program to search for the number and names of the subroutines. 
    // the names are placed in the shaders vector
    SetupShader(noise_shader.Program);
    // we print on console the name of the first subroutine used
    PrintCurrentShader(current_subroutine);
    
    // we load the model(s) (code of Model class is in include/utils/model_v1.h)
    Model cubeModel("../../models/cube.obj");
    Model sphereModel("../../models/sphere.obj");
    Model bunnyModel("../../models/bunny_lp.obj");
    Model planeModel("../../models/plane.obj");

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, -7.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Model and Normal transformation matrices for the objects in the scene: we set to identity
    glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
    glm::mat3 sphereNormalMatrix = glm::mat3(1.0f);
    glm::mat4 cubeModelMatrix = glm::mat4(1.0f);
    glm::mat3 cubeNormalMatrix = glm::mat3(1.0f);
    glm::mat4 bunnyModelMatrix = glm::mat4(1.0f);
    glm::mat3 bunnyNormalMatrix = glm::mat3(1.0f);
    glm::mat4 planeModelMatrix = glm::mat4(1.0f);

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate the time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
        if (spinning)
            orientationY+=(deltaTime*spin_speed);

        /////////////////// PLANE ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        plane_shader.Use();
        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint planeColorLocation = glGetUniformLocation(plane_shader.Program, "colorIn");
        // we assign the value to the uniform variables
        glUniform3fv(planeColorLocation, 1, planeColor);

        // we create the transformation matrix
        // we reset to identity at each frame
        planeModelMatrix = glm::mat4(1.0f);
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));

        // we render the plane
        planeModel.Draw();


        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the noise_shader Shader Program as part of the current rendering process
        noise_shader.Use();
        // we search inside the Shader Program the name of the subroutine currently selected, and we get the numerical index
        GLuint index = glGetSubroutineIndex(noise_shader.Program, GL_FRAGMENT_SHADER, shaders[current_subroutine].c_str());
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

        // we determine the position in the Shader Program of the uniform variables
        GLint frequencyLocation = glGetUniformLocation(noise_shader.Program, "frequency");
        GLint powerLocation = glGetUniformLocation(noise_shader.Program, "power");
        GLint timerLocation = glGetUniformLocation(noise_shader.Program, "timer");
        GLint harmonicsLocation = glGetUniformLocation(noise_shader.Program, "harmonics");

        // we assign the value to the uniform variable
        glUniform1f(frequencyLocation, frequency);
        glUniform1f(powerLocation, power);
        glUniform1f(timerLocation, currentFrame);
        glUniform1f(harmonicsLocation, harmonics);

        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(noise_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(noise_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // SPHERE
        /*
          we create the transformation matrix

          N.B.) the last defined is the first applied

          We need also the matrix for normals transformation, which is the inverse of the transpose of the 3x3 submatrix (upper left) of the modelview. We do not consider the 4th column because we do not need translations for normals.
          An explanation (where XT means the transpose of X, etc):
            "Two column vectors X and Y are perpendicular if and only if XT.Y=0. If We're going to transform X by a matrix M, we need to transform Y by some matrix N so that (M.X)T.(N.Y)=0. Using the identity (A.B)T=BT.AT, this becomes (XT.MT).(N.Y)=0 => XT.(MT.N).Y=0. If MT.N is the identity matrix then this reduces to XT.Y=0. And MT.N is the identity matrix if and only if N=(MT)-1, i.e. N is the inverse of the transpose of M.

        */
        // we reset to identity at each frame
        sphereModelMatrix = glm::mat4(1.0f);
        sphereNormalMatrix = glm::mat3(1.0f);
        sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 0.0f, 0.0f));
        sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
        // if we cast a mat4 to a mat3, we are automatically considering the upper left 3x3 submatrix
        sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(noise_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(noise_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));

        // we render the model
        sphereModel.Draw();

        //CUBE
        // we create the transformation matrix and the normals transformation matrix
        // we reset to identity at each frame
        cubeModelMatrix = glm::mat4(1.0f);
        cubeNormalMatrix = glm::mat3(1.0f);
        cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));	// It's a bit too big for our scene, so scale it down
        cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(noise_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(noise_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));

        // we render the cube
        cubeModel.Draw();

        //BUNNY
        // we create the transformation matrix and the normals transformation matrix
        // we reset to identity at each frame
        bunnyModelMatrix = glm::mat4(1.0f);
        bunnyNormalMatrix = glm::mat3(1.0f);
        bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(3.0f, 0.0f, 0.0f));
        bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));	// It's a bit too big for our scene, so scale it down
        bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(noise_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(noise_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

        // Swapping back and front buffers
        bunnyModel.Draw();

        // Faccio lo swap tra back e front buffer
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    plane_shader.Delete();
    noise_shader.Delete();
    // we close and delete the created context
    glfwTerminate();
    return 0;
}


//////////////////////////////////////////
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

//////////////////////////////////////////
// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    std::cout << "Current shader subroutine: " << shaders[subroutine]  << std::endl;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
  GLuint new_subroutine;
  
  // if ESC is pressed, we close the application
  if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GL_TRUE);

  // if P is pressed, we start/stop the animated rotation of models
  if(key == GLFW_KEY_P && action == GLFW_PRESS)
      spinning=!spinning;

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
}
