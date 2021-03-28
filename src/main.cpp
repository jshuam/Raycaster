#include "SDL_keycode.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <vector>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 512;
const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 16;

const char mapLayout[] = "0000000000000000"
                         "0              0"
                         "0    111111111 0"
                         "0            1 0"
                         "0            1 0"
                         "0    1111111 1 0"
                         "0            1 0"
                         "0            1 0"
                         "0    111111111 0"
                         "0            1 0"
                         "0            1 0"
                         "2  11111111111 0"
                         "2   11111111   0"
                         "2    1    1    0"
                         "2              0"
                         "0333000000000000";

const float quad[] = {
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
};

GLuint gProgramID = 0;
GLuint gVBOp = 0;
GLuint gVBOc = 0;
GLuint gVAO = 0;

GLuint modelLoc = 0;
GLuint colorLoc = 0;

SDL_Window* gWindow;
SDL_GLContext gContext;

float gPosX = 2.0f;
float gPosY = 5.0f;
float gAngle = 90;
float gFov = 60;
float gAngleInc = glm::radians(gFov / (float)SCREEN_WIDTH);

std::vector<glm::vec4> *activeVertexBuffer;
std::vector<glm::vec3> *activeColorBuffer;

std::vector<glm::vec4> rays;
std::vector<glm::vec3> rayColors;

std::vector<glm::vec4> walls;
std::vector<glm::vec3> wallColors;

std::vector<glm::vec4> minimap;
std::vector<glm::vec3> miniMapColors;

bool init();
bool initGL();
void renderMap();
void render();
void printShaderLog(GLuint shader);
void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color);
void drawLine(glm::vec2 &&start, glm::vec2 &&end, glm::vec3 color);
void flushBatch();
void flushBuffers();
void drawRays();

int main(int argc, char* argv[])
{
    if(init())
    {
        bool quit = false;
        SDL_Event e;

        while(!quit)
        {
            while(SDL_PollEvent(&e))
            {
                if(e.type == SDL_QUIT)
                {
                    quit = true;
                }
            }

            render();
            SDL_GL_SwapWindow(gWindow);
            gAngle += 0.5;
        }
    }

    return 0;
}

bool init()
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not be initialized: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    gWindow = SDL_CreateWindow("Raycaster",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    gContext = SDL_GL_CreateContext(gWindow);
    if(gContext == NULL)
    {
        printf("Unable to create OpenGL context: %s\n", SDL_GetError());
        return false;
    }

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        printf("GLAD could not be initialized\n");
        return false;
    }

    if(SDL_GL_SetSwapInterval(1) < 0)
    {
        printf("Unable to set V-Sync: %s\n", SDL_GetError());
    }

    if(!initGL())
    {
        printf("Unable to initialize OpenGL\n");
        return false;
    }

    return true;
}

bool initGL()
{
    gProgramID = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    const GLchar* vertexSource[] =
    {
        "#version 450\n"
        "\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec3 vColor;\n"
        "\n"
        "out vec3 color;\n"
        "\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
            "gl_Position = projection * position;\n"
            "color = vColor;\n"
        "}"
    };

    const GLchar* fragmentSource[] =
    {
        "#version 450\n"
        "\n"
        "in vec3 color;\n"
        "\n"
        "out vec4 fragment;\n"
        "\n"
        "void main()\n"
        "{\n"
            "fragment = vec4(color, 1.0);\n"
        "}"
    };

    glShaderSource(vertexShader, 1, vertexSource, NULL);
    glCompileShader(vertexShader);
    GLint vShaderCompiled = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
    if(vShaderCompiled != GL_TRUE)
    {
        printf("Unable to compile vertex shader: %d\n", vertexShader);
        return false;
    }

    glShaderSource(fragmentShader, 1, fragmentSource, NULL);
    glCompileShader(fragmentShader);
    GLint fShaderCompiled = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
    if(fShaderCompiled != GL_TRUE)
    {
        printf("Unable to compile fragment shader: %d\n", fragmentShader);
        printShaderLog(fragmentShader);
        return false;
    }

    glAttachShader(gProgramID, vertexShader);
    glAttachShader(gProgramID, fragmentShader);
    glLinkProgram(gProgramID);

    GLint programSuccess = GL_FALSE;
    glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
    if(programSuccess != GL_TRUE)
    {
        char infoLog[512];
        glGetProgramInfoLog(gProgramID, 512, NULL, infoLog);	
        printf("Unable to link program: %d\n", gProgramID);
        printf("%s\n", infoLog);
        return false;
    }

    glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);

    glUseProgram(gProgramID);

    GLuint projLoc = glGetUniformLocation(gProgramID, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glClearColor(0.f, 0.f, 0.f, 1.f);

    glCreateBuffers(1, &gVBOp);
    glCreateBuffers(1, &gVBOc);

    // Positions: 4 floats per vertex, 6 vertices per quad, 10,000 quads per draw call
    glNamedBufferStorage(gVBOp, 4 * 6 * 10000 * sizeof(GLfloat), nullptr, GL_DYNAMIC_STORAGE_BIT);
 
    // 1536 quads: map size 16 * 16 = 256, 256 * 6 vertices per quad
    minimap.reserve(1536);

    // 120 vertices: 60 rays, 2 vertices per ray for 1 line
    rays.reserve(2050);
    
    // 10,000 quads: 60,000 vertices total
    walls.reserve(60000);
 
    // Colors: 3 floats per vertex, 6 vertices per quad, 10,000 quads per draw call
    glNamedBufferStorage(gVBOc, 3 * 6 * 10000 * sizeof(GLfloat), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // 1536 quads: map size 16 * 16 = 256, 256 * 6 vertices per quad
    miniMapColors.reserve(1536);

    // 120 vertices: 60 rays, 2 vertices per ray for 1 line
    rayColors.reserve(2050);

    // 10,000 quads: 60,000 vertices total, 1 color per vertex
    wallColors.reserve(60000);

    glCreateVertexArrays(1, &gVAO);

    glVertexArrayVertexBuffer(gVAO, 0, gVBOp, 0, 4 * sizeof(GLfloat));
    glVertexArrayVertexBuffer(gVAO, 1, gVBOc, 0, 3 * sizeof(GLfloat));

    glEnableVertexArrayAttrib(gVAO, 0);
    glEnableVertexArrayAttrib(gVAO, 1);

    glVertexArrayAttribFormat(gVAO, 0, 4,  GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(gVAO, 1, 3,  GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(gVAO, 0, 0);
    glVertexArrayAttribBinding(gVAO, 1, 1);

    return true;
}

void renderMap()
{
    glViewport(0, 0, 512, 512);
    int quadWidth  = SCREEN_WIDTH / MAP_WIDTH;
    int quadHeight = SCREEN_HEIGHT / MAP_HEIGHT;

    activeVertexBuffer = &minimap;
    activeColorBuffer  = &miniMapColors;
    for(int i = 0; i < MAP_HEIGHT; ++i)
    {
        for(int j = 0; j < MAP_WIDTH; ++j)
        {
            if(mapLayout[i * MAP_HEIGHT + j] == ' ') continue;
            glm::vec2 pos(j * quadWidth, i * quadHeight);
            glm::vec2 size(quadWidth, quadHeight);
            drawQuad(pos, size, glm::vec3(1.0f, 0.5f, 0.5f));
        }
    }

    drawRays();
}

void drawRays()
{
    int quadWidth  = SCREEN_WIDTH / MAP_WIDTH;
    int quadHeight = SCREEN_HEIGHT / MAP_HEIGHT;

    float playerAngle = glm::radians(gAngle);
    float angle = glm::radians(gAngle - (gFov / 2));

    glViewport(0, 0, 512, 512);
    drawQuad(glm::vec2(gPosX * quadWidth, gPosY * quadHeight), glm::vec2(10.0f, 5.0f), glm::vec3(1.0, 1.0, 1.0));
    glViewport(512, 0, 512, 512);


    for(int i = 0; i <= SCREEN_WIDTH; ++i)
    {
        for(float r = 0; r < 20; r += 0.01f)
        {
            float rayX = gPosX + r * glm::cos(angle);
            float rayY = gPosY + r * glm::sin(angle);

            if(mapLayout[(int)rayY * MAP_HEIGHT + (int)rayX] != ' ')
            {
                // Store ray on hit
                activeVertexBuffer = &rays;
                activeColorBuffer = &rayColors;
                glm::vec2 start(gPosX * quadWidth, gPosY * quadHeight);
                glm::vec2 end(rayX * quadWidth, rayY * quadHeight);
                drawLine(std::move(start), std::move(end), glm::vec3(1.0, 0.0, 0.0));

                int wallColor = mapLayout[(int)rayY * MAP_HEIGHT + (int)rayX] - '0';
                glm::vec3 color;

                switch(wallColor)
                {
                    case 0:
                        color = glm::vec3(1.0f, 0.5f, 0.5f);
                        break;
                    case 1:
                        color = glm::vec3(0.7f, 0.3f, 0.5f);
                        break;
                    case 2:
                        color = glm::vec3(0.4f, 0.3f, 0.7f);
                        break;
                    case 3:
                        color = glm::vec3(0.8f, 1.0f, 0.7f);
                        break;
                }

                activeVertexBuffer = &walls;
                activeColorBuffer  = &wallColors;

                float height = (float)SCREEN_HEIGHT / r;
                drawQuad(glm::vec2(i, (float)SCREEN_HEIGHT / 2 - (height / 2)), glm::vec2(1, height), color); 

                break;
            }
        }
        angle += gAngleInc;
    }
    flushBatch();
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    renderMap();
    flushBuffers();
}

void printShaderLog(GLuint shader)
{
    if (glIsShader(shader))
    {
        int infoLogLength, maxLength = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        char *infoLog = new char[maxLength];

        glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
        if (infoLogLength > 0)
        {
            printf("%s\n", infoLog);
        }

        delete[] infoLog;
    }
    else
    {
        printf("Shader %d is not a shader\n", shader);
    }
}

void flushBuffers()
{
    glUseProgram(gProgramID);

    glBindVertexArray(gVAO);

    /******************* MAP ************************/

    glViewport(0, 0, 512, 512);
    activeVertexBuffer = &minimap;
    activeColorBuffer = &miniMapColors;

    glNamedBufferSubData(gVBOp, 0, activeVertexBuffer->size() * sizeof(glm::vec4), activeVertexBuffer->data());
    glNamedBufferSubData(gVBOc, 0, activeColorBuffer->size()  * sizeof(glm::vec3), activeColorBuffer->data());

    glDrawArrays(GL_TRIANGLES, 0, activeVertexBuffer->size());

    //printf("Number of map tiles: %lu\n", minimap.size());

    activeVertexBuffer->clear();
    activeColorBuffer->clear();

    /******************* RAYS ************************/

    activeVertexBuffer = &rays;
    activeColorBuffer = &rayColors;

    glNamedBufferSubData(gVBOp, 0, activeVertexBuffer->size() * sizeof(glm::vec4), activeVertexBuffer->data());
    glNamedBufferSubData(gVBOc, 0, activeColorBuffer->size()  * sizeof(glm::vec3), activeColorBuffer->data());

    glDrawArrays(GL_LINES, 0, activeVertexBuffer->size());

    //printf("Number of rays: %lu\n", rays.size());
    //
    activeVertexBuffer->clear();
    activeColorBuffer->clear();

    /******************* WALLS ************************/

    glViewport(512, 0, 512, 512);
    activeVertexBuffer = &walls;
    activeColorBuffer = &wallColors;

    glNamedBufferSubData(gVBOp, 0, activeVertexBuffer->size() * sizeof(glm::vec4), activeVertexBuffer->data());
    glNamedBufferSubData(gVBOc, 0, activeColorBuffer->size()  * sizeof(glm::vec3), activeColorBuffer->data());

    glDrawArrays(GL_TRIANGLES, 0, activeVertexBuffer->size());

    //printf("Number of walls: %lu\n", walls.size());

    activeVertexBuffer->clear();
    activeColorBuffer->clear();

    glUseProgram(0);
}

void flushBatch()
{
    glUseProgram(gProgramID);

    glBindVertexArray(gVAO);

    glNamedBufferSubData(gVBOp, 0, activeVertexBuffer->size() * sizeof(glm::vec4), activeVertexBuffer->data());
    glNamedBufferSubData(gVBOc, 0, activeColorBuffer->size()  * sizeof(glm::vec3), activeColorBuffer->data());

    glDrawArrays(GL_TRIANGLES, 0, activeVertexBuffer->size());

    activeVertexBuffer->clear();
    activeColorBuffer->clear();

    glUseProgram(0);
}

void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(pos, 1.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));

    activeVertexBuffer->push_back((model * glm::vec4(quad[0], quad[1], 0.0f, 1.0f)));
    activeVertexBuffer->push_back((model * glm::vec4(quad[2], quad[3], 0.0f, 1.0f)));
    activeVertexBuffer->push_back((model * glm::vec4(quad[4], quad[5], 0.0f, 1.0f)));
    activeVertexBuffer->push_back((model * glm::vec4(quad[6], quad[7], 0.0f, 1.0f)));
    activeVertexBuffer->push_back((model * glm::vec4(quad[8], quad[9], 0.0f, 1.0f)));
    activeVertexBuffer->push_back((model * glm::vec4(quad[10], quad[11], 0.0f, 1.0f)));

    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);

    // 10,000 quads: 6 vertices per quad, 60,000 vertices total
    if(activeVertexBuffer->size() >= 60000)
    {
        flushBatch();
    }
}

void drawLine(glm::vec2 &&start, glm::vec2 &&end, glm::vec3 color)
{
    glm::mat4 startModel(1.0f);
    glm::mat4 endModel(1.0f);
    startModel = glm::translate(startModel, glm::vec3(start, 1.0f));
    endModel   = glm::translate(endModel, glm::vec3(end, 1.0f));

    activeVertexBuffer->push_back((startModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
    activeVertexBuffer->push_back((endModel * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));

    activeColorBuffer->push_back(color);
    activeColorBuffer->push_back(color);
}

