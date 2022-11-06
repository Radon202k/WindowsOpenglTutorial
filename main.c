#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")

unsigned int client_width = 0;
unsigned int client_height = 0;

LRESULT CALLBACK
window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_SIZE:
        {
            client_width = LOWORD(lparam);
            client_height = HIWORD(lparam);
        } break;
        
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
        
        default:
        {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

#include "win32_opengl.h"

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
    // declare variables
    int game_running = 1;
    HWND window;
    HDC device_context;
    HGLRC opengl_context;
    // call init_opengl()
    init_opengl(&window, &device_context, &opengl_context, 800, 600, "Test window");
    
    // create shader for drawing points
    char *pts_vertex_shader_source =
        "#version 450 core                      \n"
        "layout (location=0) in vec3 in_pos;    \n"
        "out gl_PerVertex {                     \n"
        "  vec4 gl_Position;                    \n"
        "};                                     \n"
        "void main()                            \n"
        "{                                      \n"
        "gl_Position = vec4(in_pos, 1);         \n"
        "}                                      \n";
    
    char *pts_fragment_shader_source =
        "#version 450 core                      \n"
        "layout (location=0)                    \n"
        "out vec4 gl_FragColor;                 \n"
        "void main()                            \n"
        "{                                      \n"
        "gl_FragColor = vec4(0,1,0,1);          \n"
        "}                                      \n";
    
    GLuint vertex_shader = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, (char **)&pts_vertex_shader_source);
    GLuint fragment_shader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, (char **)&pts_fragment_shader_source);
    
    GLint linked;
    glGetProgramiv(vertex_shader, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char message[1024];
        glGetProgramInfoLog(vertex_shader, sizeof(message), 0, message);
        OutputDebugStringA(message);
        assert(!"Failed to create vertex shader!");
    }
    glGetProgramiv(fragment_shader, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char message[1024];
        glGetProgramInfoLog(fragment_shader, sizeof(message), 0, message);
        OutputDebugStringA(message);
        assert(!"Failed to create fragment shader!");
    }
    
    GLuint shader_pipeline=(GLuint)-1;
    glGenProgramPipelines(1, &shader_pipeline);
    glUseProgramStages(shader_pipeline, GL_VERTEX_SHADER_BIT, vertex_shader);
    glUseProgramStages(shader_pipeline, GL_FRAGMENT_SHADER_BIT, fragment_shader);
    // set on init opengl context state
    wglSwapIntervalEXT(1); // enable vsync
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha blended
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPointSize(5);
    // create opengl buffers for drawing points (vao and vbo)
    GLuint pts_vao=(GLuint)-1;
    glCreateVertexArrays(1, &pts_vao);
    
    GLuint pts_vbo=(GLuint)-1;
    glCreateBuffers(1, &pts_vbo);
    
    // allocates gpu memory for vertex buffer (enough for a maximum number allowed of points)
    glNamedBufferStorage(pts_vbo, 4096*sizeof(float)*3, 0, GL_DYNAMIC_STORAGE_BIT);
    
    // bind vertex buffer to vertex array object
    GLint vbo_index = 0;
    glVertexArrayVertexBuffer(pts_vao, vbo_index, pts_vbo, 0, sizeof(float)*3);
    
    // set vertex buffer input layout (remember pts_3d.bo[0] is the vao [vertex array object])
    glVertexArrayAttribFormat(pts_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(pts_vao, 0, vbo_index);
    glEnableVertexArrayAttrib(pts_vao, 0);
    ShowWindow(window, SW_SHOWDEFAULT);
    while (game_running)
    {
        // process Windows OS events
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                game_running = 0;
                break;
            }
            
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        // set per frame opengl context state
        glClearColor(0.1f, 0.1f, 0.1f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0,0,client_width,client_height);
        glScissor(0,0,client_width,client_height);
        glBindVertexArray(pts_vao);
        // set data for opengl vertex buffer
        float pt_set[] =
        {
            -0.5f,-0.5f,-0.5f,  // (x,y,z) for a single point
            +0.5f,-0.5f,-0.5f,
            +0.5f,+0.5f,-0.5f,
            -0.5f,+0.5f,-0.5f,
        };
        
        unsigned int float_count = array_count(pt_set);
        unsigned int pt_count = float_count / 3; // (x,y,z)
        unsigned int pt_set_byte_size = sizeof(pt_set);
        
        // copy from stack memory to gpu memory that we reserved earlier for vertex buffer
        glNamedBufferSubData(pts_vbo, 0, pt_set_byte_size, pt_set);
        // draw to back buffer and swap
        glBindProgramPipeline(shader_pipeline);
        glDrawArrays(GL_POINTS, 0, pt_count);
        SwapBuffers(device_context);
    }
}