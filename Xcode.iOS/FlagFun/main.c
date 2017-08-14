
#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>  // TODO: Would use glad; yet we get unresolved symbols at link
#else // iOS, OS X, Linux
#include "glad/glad.h"
#endif

#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "matrix4.h"

// emcc 06_rainbow.c matrix4.c --preload-file 06_rainbow.vertex --preload-file 06_rainbow.fragment -O2 -s USE_SDL=2 -s FULL_ES3=1 -s WASM=1 -o 06_rainbow.html

// Linux
// gcc -std=c99 -I. $(sdl2-config --cflags --libs) gl.c matrix4.c glad/glad.c -ldl -lm -lGL -DVERTEX_SHADER='"06_flag.vertex"' -DFRAGMENT_SHADER='"06_flag.fragment"' -o 06_flag

// macOS
// gcc -I. $(sdl2-config --cflags --libs) -framework OpenGL gl.c glad/glad.c matrix4.c -DVERTEX_SHADER='"06_flag.vertex"' -DFRAGMENT_SHADER='"06_flag.fragment"' -o 06_flag

Matrix4 *ModelMatrix;
GLuint u_ModelMatrix;
GLuint u_Ticks;
GLuint u_Resolution;
GLuint u_ClientMouse;
GLuint u_DeviceOrientation;

GLuint VertexBuffer;

GLfloat Resolution[] = {
#if __IPHONEOS__
    320.0,
#else
    640.0,
#endif
    480.0
};

GLfloat Vertices[] = {
    -1.0,  1.0,
    -1.0, -1.0,
     1.0, -1.0,

    -1.0,  1.0,
     1.0, -1.0,
     1.0,  1.0
};

#define TICK_INTERVAL 30
Uint32 NextTime;

void animate();
void draw();
void compile_shader(const int shader_type, GLuint *shader_number, const char *shader_file, char *msg, size_t sz_msg);

// emcc gl.c matrix4.c -DVERTEX_SHADER='"06_rainbow.vertex"' -DFRAGMENT_SHADER='"06_rainbow.fragment"' --preload-file 06_rainbow.vertex --preload-file 06_rainbow.fragment -O2 -s USE_SDL=2 -s FULL_ES3=1 -s WASM=1 -o 06_rainbow.html

void
setup(int width, int height, char *argv[])
{
    GLuint vertex_shader, fragment_shader;
    char msg[512];

    NextTime = SDL_GetTicks();

    ModelMatrix = CreateMatrix4();
    iMatrix4.Identity(ModelMatrix);

    GLuint GlProgram;

    glClearColor(1.0, 1.0, 1.0, 1.0);

    /* Compile the vertex shader */
    compile_shader(GL_VERTEX_SHADER, &vertex_shader, VERTEX_SHADER, msg, sizeof(msg));
    if (strlen(msg)) printf("vertex shader info: %s\n", msg);
    
    /* Compile the fragment shader */
    compile_shader(GL_FRAGMENT_SHADER, &fragment_shader, FRAGMENT_SHADER, msg, sizeof(msg));
    if (strlen(msg)) printf("fragment shader info: %s\n", msg);
    
    /* Create and link the shader program */
    GlProgram = glCreateProgram();
    glAttachShader(GlProgram, vertex_shader);
    glAttachShader(GlProgram, fragment_shader);
   
    // TODO: GL_LINK_STATUS
    glLinkProgram(GlProgram);
    glGetProgramInfoLog(GlProgram, sizeof msg, NULL, msg);
    if (strlen(msg)) printf("info: %s\n", msg);
    
    /* Enable the shaders */
    glUseProgram(GlProgram);

    // Vertices
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), &Vertices[0], GL_STATIC_DRAW);

    GLint a_Position = glGetAttribLocation(GlProgram, "a_Position");
    glVertexAttribPointer(a_Position, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_Position);
    
    u_ModelMatrix = glGetUniformLocation(GlProgram, "u_ModelMatrix");
    u_Ticks = glGetUniformLocation(GlProgram, "u_Ticks");
    u_Resolution = glGetUniformLocation(GlProgram, "u_Resolution");
    u_ClientMouse = glGetUniformLocation(GlProgram, "u_ClientMouse");
    u_DeviceOrientation = glGetUniformLocation(GlProgram, "u_DeviceOrientation");

    glViewport(0, 0, (GLint) width, (GLint) height);
}

Uint32
time_left(void)
{
    Uint32 now;

    now = SDL_GetTicks();

    if (NextTime <= now) {
        return 0;
    } else {
        return NextTime - now;
    }
}

void
tick()
{
    draw();

    animate();
}

void
animate()
{
    Uint32 timeLeft = time_left();

    SDL_Delay(timeLeft);
    NextTime += TICK_INTERVAL;
}

void
draw()
{
    static GLfloat now;

    now = SDL_GetTicks();
    if (NULL != getenv("GLFUN_GetTicks")) printf("%f\n", now);

    glUniform1fv(u_Ticks, 1, &now);
    glUniform2fv(u_Resolution, 1, Resolution);
    glUniformMatrix4fv(u_ModelMatrix, 1, GL_FALSE, ModelMatrix->Elements);

    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

#ifdef EMSCRIPTEN
EM_BOOL
mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
#if 0
  printf("screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), canvas: (%ld,%ld)\n",
    e->screenX, e->screenY, e->clientX, e->clientY,
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->button, e->buttons, e->movementX, e->movementY, e->canvasX, e->canvasY);
#endif

  GLfloat mouseClient[] = { e->clientX, e->clientY };

  glUniform2fv(u_ClientMouse, 1, mouseClient);

  return 0;
}
#endif

#ifdef EMSCRIPTEN
EM_BOOL
orientation_callback(int eventType, const EmscriptenDeviceOrientationEvent *e, void *userData)
{
  GLfloat deviceOrientation[] = { e->alpha, e->beta, e->gamma };

  glUniform3fv(u_DeviceOrientation, 1, deviceOrientation);

  return 0;
}
#endif

void
compile_shader(const int shader_type, GLuint *shader_number, const char *shader_file, char *msg, size_t sz_msg)
{
    long numbytes;
    FILE *infile = NULL;
    GLchar *shader_buffer = NULL;

    *shader_number = glCreateShader(shader_type);

    /* open an existing file for reading */
    infile = fopen(shader_file, "rb");
    if (infile == NULL) {
        char *str = "fopen returned null";

        strcpy(msg, str);

        return;
    }
     
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    fseek(infile, 0L, SEEK_SET);	
     
    shader_buffer = (char*)calloc(numbytes, sizeof(char));	
    if (shader_buffer == NULL) {
        fclose(infile);

        return;
    }
     
    fread(shader_buffer, sizeof(char), numbytes, infile);
    // printf("%s\n", shader_buffer);

    // TODO: GL_COMPILE_STATUS
    *shader_number = glCreateShader(shader_type);
    glShaderSource(*shader_number, 1, (const GLchar * const *)&shader_buffer, NULL);
    glCompileShader(*shader_number);
    glGetShaderInfoLog(*shader_number, sz_msg, NULL, msg);

    fclose(infile);
    free(shader_buffer);
}

int
main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_GLContext glContext;

    bool initialized = SDL_Init(SDL_INIT_VIDEO) == 0;
    assert(initialized);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("sdlJoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)Resolution[0], (int)Resolution[1], SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    assert(window);

    glContext = SDL_GL_CreateContext(window);

#ifndef EMSCRIPTEN
    gladLoadGLLoader(SDL_GL_GetProcAddress);
#endif
   
#ifdef EMSCRIPTEN
    setup((int)Resolution[0], (int)Resolution[1], NULL);
#else
    setup((int)Resolution[0], (int)Resolution[1], argv + 1);
#endif

#ifdef EMSCRIPTEN
    emscripten_set_mousemove_callback(0, 0, 1, mouse_callback);
    emscripten_set_deviceorientation_callback(0, 0, orientation_callback);
    emscripten_set_main_loop(tick, 0, 1);
#else
    SDL_Event event;

    while(1) {
        draw();

        SDL_GL_SwapWindow(window);

        SDL_Delay(30);

        while(SDL_PollEvent(&event)) {
            if (SDL_QUIT == event.type) {
                SDL_DestroyWindow(window);

                window = NULL;

                // Cleanup
                glDeleteBuffers(1, &VertexBuffer);
                SDL_GL_DeleteContext(glContext);
                SDL_DestroyWindow(window);

                SDL_Quit();

                return EXIT_SUCCESS;
            } else if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_RESIZED == event.window.event) {
                Resolution[0] = event.window.data1;
                Resolution[1] = event.window.data2;

                glViewport(0, 0, (GLint) Resolution[0], (GLint) Resolution[1]);
            }
        }
    }
#endif

    return EXIT_SUCCESS;
}
