#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>

struct Target {
    glm::vec3 position;
};

std::random_device rd;
std::mt19937 gen(rd());

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

std::vector<Target> spawnTargets(int count, float minDistance) {
    std::vector<Target> targets;
    
    for (int i = 0; i < count; i++) {
        glm::vec3 newPos;
        bool validPosition = false;
        int attempts = 0;
        
        while (!validPosition && attempts < 100) {
            newPos = glm::vec3(
                randomFloat(-3.0f, 3.0f),  
                randomFloat(-2.5f, 2.5f),   
                -8.0f                       
            );
            
            validPosition = true;
            
            for (const auto& target : targets) {
                float distance = glm::length(newPos - target.position);
                if (distance < minDistance) {
                    validPosition = false;
                    break;
                }
            }
            attempts++;
        }
        
        targets.push_back({newPos});
    }
    
    return targets;
}

const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;    
layout(location = 1) in vec3 aColor; 
out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);  
    vertexColor = aColor;            
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

const char* crosshairVertexSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec3 color;
out vec3 vertexColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = color;
}
)";

const char* crosshairFragmentSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

float deltaTime = 0.0f;  
float lastFrame = 0.0f;
int score = 0;
std::vector<Target>* targetsPtr = nullptr;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.042f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    
    if (pitch < -89.0f) 
        pitch = -89.0f;
    
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glm::vec3 rayOrigin = cameraPos;
        glm::vec3 rayDirection = glm::normalize(cameraFront);

        for (size_t i = 0; i < targetsPtr->size(); i++) {
            Target& target = (*targetsPtr)[i];
            
            glm::vec3 boxMin = target.position - glm::vec3(0.5f);
            glm::vec3 boxMax = target.position + glm::vec3(0.5f);

            float tMin = 0.0f;
            float tMax = 100.0f;

            bool hit = true;
            for (int axis = 0; axis < 3; axis++) {
                float origin = rayOrigin[axis]; 
                float dir = rayDirection[axis];          
                float bMin = boxMin[axis];       
                float bMax = boxMax[axis];        
                
                if (std::abs(dir) < 0.0001f) {
                    if (origin < bMin || origin > bMax) {
                        hit = false;
                        break;
                    }
                } else {
                    float t1 = (bMin - origin) / dir; 
                    float t2 = (bMax - origin) / dir;
                    
                    if (t1 > t2) std::swap(t1, t2);
                    
                    tMin = std::max(tMin, t1);  
                    tMax = std::min(tMax, t2); 
                    
                    if (tMin > tMax) {
                        hit = false;
                        break;
                    }
                }
            }
            
            if (hit && tMin > 0) {
                score++;
                std::cout << "HIT! Score: " << score << std::endl;
                
                glm::vec3 newPos;
                bool validPosition = false;
                int attempts = 0;
                while (!validPosition && attempts < 100) {
                    newPos = glm::vec3(
                        randomFloat(-3.0f, 3.0f),  
                        randomFloat(-2.5f, 2.5f),   
                        -8.0f                       
                    );
                    
                    validPosition = true;
                    
                    for (size_t j = 0; j < targetsPtr->size(); j++) {
                        if (j == i) continue; 
                        float distance = glm::length(newPos - (*targetsPtr)[j].position);
                        if (distance < 2.0f) { 
                            validPosition = false;
                            break;
                        }
                    }
                    attempts++;
                }
                target.position = newPos;
                break; 
            }
        }
    }
}

int main() {
    if (!glfwInit()) { 
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        800, 
        600, 
        "Aim Trainer", 
        NULL, 
        NULL
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    float vertices[] = {
        -0.5f, -0.5f,  0.5f,   1.0f, 0.2f, 0.2f,  
         0.5f, -0.5f,  0.5f,   1.0f, 0.2f, 0.2f,  
         0.5f,  0.5f,  0.5f,   1.0f, 0.2f, 0.2f, 
         0.5f,  0.5f,  0.5f,   1.0f, 0.2f, 0.2f, 
        -0.5f,  0.5f,  0.5f,   1.0f, 0.2f, 0.2f, 
        -0.5f, -0.5f,  0.5f,   1.0f, 0.2f, 0.2f,
        -0.5f, -0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
         0.5f, -0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
         0.5f,  0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
         0.5f,  0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
        -0.5f,  0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
        -0.5f, -0.5f, -0.5f,   0.8f, 0.1f, 0.1f,
        -0.5f,  0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f,  0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f, -0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f, -0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f, -0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f,  0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
         0.5f,  0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
         0.5f,  0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
         0.5f, -0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
         0.5f, -0.5f, -0.5f,   0.9f, 0.15f, 0.15f,
         0.5f, -0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
         0.5f,  0.5f,  0.5f,   0.9f, 0.15f, 0.15f,
        -0.5f,  0.5f, -0.5f,   1.0f, 0.3f, 0.3f,
         0.5f,  0.5f, -0.5f,   1.0f, 0.3f, 0.3f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.3f, 0.3f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.3f, 0.3f,
        -0.5f,  0.5f,  0.5f,   1.0f, 0.3f, 0.3f,
        -0.5f,  0.5f, -0.5f,   1.0f, 0.3f, 0.3f,
        -0.5f, -0.5f, -0.5f,   0.7f, 0.1f, 0.1f,
         0.5f, -0.5f, -0.5f,   0.7f, 0.1f, 0.1f,
         0.5f, -0.5f,  0.5f,   0.7f, 0.1f, 0.1f,
         0.5f, -0.5f,  0.5f,   0.7f, 0.1f, 0.1f,
        -0.5f, -0.5f,  0.5f,   0.7f, 0.1f, 0.1f,
        -0.5f, -0.5f, -0.5f,   0.7f, 0.1f, 0.1f,
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,                     
        3,                     
        GL_FLOAT,              
        GL_FALSE,             
        6 * sizeof(float),     
        (void*)0               
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,                    
        3,                  
        GL_FLOAT,              
        GL_FALSE,            
        6 * sizeof(float),    
        (void*)(3 * sizeof(float))  
    );
    glEnableVertexAttribArray(1);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error: " << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error: " << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float crosshairSize = 0.02f;
    float crosshairVertices[] = {

        -crosshairSize, 0.0f,
         crosshairSize, 0.0f,
       
        0.0f, -crosshairSize * (800.0f/600.0f),
        0.0f,  crosshairSize * (800.0f/600.0f),
    };

    unsigned int crosshairVAO, crosshairVBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int chVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(chVertexShader, 1, &crosshairVertexSource, NULL);
    glCompileShader(chVertexShader);

    unsigned int chFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(chFragmentShader, 1, &crosshairFragmentSource, NULL);
    glCompileShader(chFragmentShader);

    unsigned int crosshairShader = glCreateProgram();
    glAttachShader(crosshairShader, chVertexShader);
    glAttachShader(crosshairShader, chFragmentShader);
    glLinkProgram(crosshairShader);

    glDeleteShader(chVertexShader);
    glDeleteShader(chFragmentShader);
    std::vector<Target> targets = spawnTargets(3, 2.0f);
    targetsPtr = &targets;

    glEnable(GL_DEPTH_TEST);
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        float cameraSpeed = 2.5f * deltaTime;  
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;          
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;          
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );
        
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 
            800.0f / 600.0f,     
            0.1f,                
            100.0f               
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "view"),
            1, GL_FALSE, glm::value_ptr(view)
        );
        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection)
        );

        glBindVertexArray(VAO);
        for (const auto& target : targets) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, target.position);
            glUniformMatrix4fv(
                glGetUniformLocation(shaderProgram, "model"), 
                1, 
                GL_FALSE, 
                glm::value_ptr(model)
            );
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glDisable(GL_DEPTH_TEST); 
        glUseProgram(crosshairShader);
        glUniform3f(glGetUniformLocation(crosshairShader, "color"), 1.0f, 0.4f, 0.7f);
        glBindVertexArray(crosshairVAO);
        glLineWidth(2.0f);  
        glDrawArrays(GL_LINES, 0, 4);
        glEnable(GL_DEPTH_TEST);  
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &crosshairVAO);
    glDeleteBuffers(1, &crosshairVBO);
    glDeleteProgram(crosshairShader);

    glfwTerminate();
    return 0;
}