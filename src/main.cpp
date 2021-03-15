#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <SDL.h>
#include <SDL_opengl.h>
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
GLuint gVBO = 0;
GLuint gVAO = 0;

GLuint modelLoc = 0;
GLuint colorLoc = 0;

SDL_Window* gWindow;
SDL_GLContext gContext;

float gAngle = 90;
float gFov = 60;
float gAngleInc = glm::radians(gFov / (float)SCREEN_WIDTH);

bool init();
bool initGL();
void renderMap();
void render();
void printShaderLog(GLuint shader);
void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color);
void drawRay(float angle, int segment);

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
            gAngle += 1;
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
        "layout (location = 0) in vec2 position;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
            "gl_Position = projection * model * vec4(position.xy, 0.0, 1.0);\n"
            "//gl_Position = vec4(position.x, position.y, 0, 1);\n"
        "}"
    };

    const GLchar* fragmentSource[] =
    {
        "#version 450\n"
        "\n"
        "uniform vec3 color;\n"
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
        printf("Unable to link program: %d\n", gProgramID);
        return false;
    }

    glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);

    glUseProgram(gProgramID);

    GLuint projLoc = glGetUniformLocation(gProgramID, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    modelLoc = glGetUniformLocation(gProgramID, "model");
    colorLoc = glGetUniformLocation(gProgramID, "color");


    glClearColor(0.f, 0.f, 0.f, 1.f);

    glCreateBuffers(1, &gVBO);

    glNamedBufferStorage(gVBO, 12 * sizeof(GLfloat), quad, GL_DYNAMIC_STORAGE_BIT);

    glCreateVertexArrays(1, &gVAO);

    glVertexArrayVertexBuffer(gVAO, 0, gVBO, 0, 2 * sizeof(GLfloat));

    glEnableVertexArrayAttrib(gVAO, 0);

    glVertexArrayAttribFormat(gVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(gVAO, 0, 0);

    return true;
}

void renderMap()
{
    glViewport(0, 0, 512, 512);
    int quadWidth  = SCREEN_WIDTH / MAP_WIDTH;
    int quadHeight = SCREEN_HEIGHT / MAP_HEIGHT;

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

    float angle = glm::radians(gAngle - (gFov / 2));
    
    for(int i = 0; i <= SCREEN_WIDTH; ++i)
    {
        drawRay(angle, i);
        angle += gAngleInc;
    }
}

void drawRay(float angle, int segment)
{
    int quadWidth  = SCREEN_WIDTH / MAP_WIDTH;
    int quadHeight = SCREEN_HEIGHT / MAP_HEIGHT;

    float posX = 2.0f;
    float posY = 5.0f;

    glViewport(0, 0, 512, 512);
    drawQuad(glm::vec2(posX * quadWidth, posY * quadHeight), glm::vec2(10.0f, 5.0f), glm::vec3(1.0, 1.0, 1.0));

    for(float r = 0; r < 20; r += 0.1f)
    {
        float rayX = posX + r * glm::cos(angle);
        float rayY = posY + r * glm::sin(angle);

        if(mapLayout[(int)rayY * MAP_HEIGHT + (int)rayX] != ' ')
        {
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

            glViewport(512, 0, 512, 512);

            float height = (float)SCREEN_HEIGHT / r;
            drawQuad(glm::vec2(segment, (float)SCREEN_HEIGHT / 2 - (height / 2)), glm::vec2(1, height), color); 

            break;
        }
        else
        {
            glViewport(0, 0, 512, 512);
            drawQuad(glm::vec2(rayX * quadWidth, rayY * quadHeight), glm::vec2(5.0f, 2.5f), glm::vec3(1.0, 0.0, 0.0));
        }
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    renderMap();
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

void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color)
{
    glUseProgram(gProgramID);

    glBindVertexArray(gVAO);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(pos, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(0);
}

