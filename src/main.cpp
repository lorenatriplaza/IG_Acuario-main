#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shaders.h"
#include "Model.h"

// Tamaño de la ventana 
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Centro del acuario 
const glm::vec3 AQUARIUM_CENTER(0.0f, -1.7f, -12.0f);

// Parámetros de la cámara 
float camAlpha = 0.0f;
float camBeta  = 0.0f;
float camDist  = 14.0f;
glm::vec3 camOffset = AQUARIUM_CENTER;

// Control del ratón 
bool rotating = false;
bool panning = false;
bool movingLight = false;  
double lastX = 0.0;
double lastY = 0.0;

float t_global = 0.0f;


// Luz ambiental global
glm::vec3 ambientLight(0.4f, 0.4f, 0.45f);  

// Luz direccional fija (desde arriba)
glm::vec3 dirLightDir(0.2f, -1.0f, -0.3f);
glm::vec3 dirLightColor(0.5f, 0.5f, 0.6f);  

// Luz móvil 
glm::vec3 movingLightPos(0.0f, -1.7f, -12.0f);  
glm::vec3 movingLightColor(3.0f, 2.8f, 2.5f);  
bool movingLightEnabled = false;  

// Estados
bool peces_pausados = false;
int peces_visibles = 5;

// Shaders y modelos globales
Shaders shader;
Model   cubeModel;
Model   fishModel;
Model   sphereModel;
Model   tableModel;
Model   coralModel;
Model   coneModel;

// Texturas (GLuint)
GLuint sandTexture;
GLuint coralTexture;
GLuint ventiladorTexture;
GLuint backgroundVAO = 0;
GLuint backgroundVBO = 0;
GLuint backgroundTexture = 0;
GLuint roomBackTexture = 0;

// Estructura pez 
struct Pez {
    glm::vec3 posicion;
    glm::vec3 velocidad;
    glm::vec3 velocidadOriginal;
    glm::vec4 color;
    float fase;
    float escala;
    float anguloDireccion;
    float anguloObjetivo;
    float tiempoOndulacion;
    float amplitudOndulacion;
    float frecuenciaOndulacion;
    float tiempoCambio;
    float alturaObjetivo;
    int modeloTipo;
    bool persigiendoComida;
};

const int NUM_PECES = 9;
Pez peces[NUM_PECES];

// Estructura comida
struct Comida {
    glm::vec3 posicion;
    bool activa;
    glm::vec4 color;
    float escala; 
};

const int MAX_COMIDA = 300;
Comida comidas[MAX_COMIDA];

// Estructura burbuja
struct Burbuja {
    glm::vec3 posicion;
    bool activa;
    float escala;
    float velocidadSubida;
    float oscilacionX;
    float oscilacionZ;
    float fase;  
};

const int MAX_BURBUJAS = 120;
Burbuja burbujas[MAX_BURBUJAS];
float tiempoUltimaBurbuja = 0.0f;

// Estructura ventilador
struct Ventilador {
    glm::vec3 posicion;
    float anguloAspas;
    float velocidadRotacion;
    float anguloMovimiento; 
    float radio;
    float anguloRotacionPalo;  // Rotación del palo sobre sí mismo
};

Ventilador ventilador;

// Declaraciones de funciones
void actualizarPeces(float dt);
void actualizarComida(float dt);
void echarComida();
void actualizarBurbujas(float dt);
void generarBurbuja();
void actualizarVentilador(float dt);
void drawVentilador(glm::mat4 P, glm::mat4 V);

// Creación del plano de fondo 
void createBackgroundPlane() {
    float vertices[] = {
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);

    glBindVertexArray(backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}
 
// Cargar texturas 
GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Error al cargar textura: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    (void)window;
    glViewport(0, 0, w, h);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    // Rotar cámara
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rotating = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            rotating = false;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    if (!rotating && !panning) return;

    double dx = xpos - lastX;
    double dy = ypos - lastY;

    if (rotating) {
        camAlpha += static_cast<float>(dx) * 0.3f;
        camBeta  += static_cast<float>(dy) * 0.3f;

        if (camBeta > 89.0f)  camBeta = 89.0f;
        if (camBeta < -89.0f) camBeta = -89.0f;
    } else if (panning) {
        camOffset.x -= static_cast<float>(dx) * 0.01f;
        camOffset.y += static_cast<float>(dy) * 0.01f;
    }

    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    // Acercar/Alejar 
    camDist -= static_cast<float>(yoffset) * 0.5f;
    if (camDist < 0.5f) camDist = 0.5f;
    if (camDist > 60.0f) camDist = 60.0f;
}

// Entrada por teclado (Los diferentes controles se encuentran en la documentación del proyecto)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Pausar/reanudar peces
    static bool p_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !p_pressed) {
        peces_pausados = !peces_pausados;
        p_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        p_pressed = false;
    }

    // WASD: Desplazamiento lateral y profundidad de la cámara
    const float camMoveSpeed = 0.1f;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Adelante 
        camOffset.z -= camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // Atrás 
        camOffset.z += camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        // Izquierda 
        camOffset.x -= camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Derecha
        camOffset.x += camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        // Bajar
        camOffset.y -= camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        // Subir 
        camOffset.y += camMoveSpeed;
    }
    
    // Flechas del teclado: Mover cámara
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        // Flecha Izquierda: Mover cámara hacia la izquierda
        camOffset.x -= camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        // Flecha Derecha: Mover cámara hacia la derecha
        camOffset.x += camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        // Flecha Arriba: Mover cámara hacia arriba
        camOffset.y += camMoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        // Flecha Abajo: Mover cámara hacia abajo
        camOffset.y -= camMoveSpeed;
    }
    
    // IJKLUO: Controles luz móvil
    const float lightMoveSpeed = 0.15f;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        movingLightPos.x -= lightMoveSpeed;  // J: Izquierda
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        movingLightPos.x += lightMoveSpeed;  // L: Derecha
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        movingLightPos.y += lightMoveSpeed;  // I: Arriba
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        movingLightPos.y -= lightMoveSpeed;  // K: Abajo
    }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        movingLightPos.z += lightMoveSpeed;  // U: Adelante 
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        movingLightPos.z -= lightMoveSpeed;  // O: Atrás 
    }
    
    // Límites luz móvil pecera
    float minX = -4.5f;   
    float maxX = 4.5f;
    float minY = -4.8f;   
    float maxY = 1.0f;
    float minZ = -14.0f;  
    float maxZ = -10.0f;
    
    movingLightPos.x = glm::clamp(movingLightPos.x, minX, maxX);
    movingLightPos.y = glm::clamp(movingLightPos.y, minY, maxY);
    movingLightPos.z = glm::clamp(movingLightPos.z, minZ, maxZ);
    
    
    // Encender/Apagar luz móvil con la tecla T
    static bool t_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !t_pressed) {
        movingLightEnabled = !movingLightEnabled;
        std::cout << "Luz móvil: " << (movingLightEnabled ? "ENCENDIDA" : "APAGADA") << std::endl;
        t_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        t_pressed = false;
    }

    // Echar comida con la tecla C
    static bool c_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !c_pressed) {
        echarComida();
        c_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        c_pressed = false;
    }

    // Mover ventilador con teclas B (sentido horario) y R (sentido antihorario)
    const float velocidadAngular = 0.3f; 
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        ventilador.anguloMovimiento += velocidadAngular;  
        if (ventilador.anguloMovimiento >= 360.0f) ventilador.anguloMovimiento -= 360.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        ventilador.anguloMovimiento -= velocidadAngular;  
        if (ventilador.anguloMovimiento < 0.0f) ventilador.anguloMovimiento += 360.0f;
    }
    
    // Actualizar posición del ventilador 
    float anguloRad = glm::radians(ventilador.anguloMovimiento);
    ventilador.posicion.x = cos(anguloRad) * ventilador.radio;
    ventilador.posicion.z = -11.5f + sin(anguloRad) * ventilador.radio;

    // Control de número de peces visibles
    static bool num_pressed = false;
    if (!num_pressed) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) { peces_visibles = 1; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) { peces_visibles = 2; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) { peces_visibles = 3; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) { peces_visibles = 4; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) { peces_visibles = 5; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) { peces_visibles = 6; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) { peces_visibles = 7; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) { peces_visibles = 8; num_pressed = true; }
        else if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) { peces_visibles = 9; num_pressed = true; }
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_7) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_8) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_9) == GLFW_RELEASE) {
        num_pressed = false;
    }


    // Resetear vista
    static bool spaceKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spaceKeyPressed) {
        camAlpha = 0.0f;
        camBeta = 0.0f;
        camDist = 18.0f;
        camOffset = AQUARIUM_CENTER;
        movingLightPos = glm::vec3(0.0f, 2.0f, -10.0f);  
        
        for (int i = 0; i < MAX_COMIDA; i++) {
            comidas[i].activa = false;
        }
        
        spaceKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spaceKeyPressed = false;
    }
}

glm::vec3 computeCameraPosition()
{
    const float alpha = glm::radians(camAlpha);
    const float beta  = glm::radians(camBeta);
    const float cosBeta = cosf(beta);
    glm::vec3 basePos = glm::vec3(
        camDist * cosBeta * sinf(alpha),
        camDist * sinf(beta),
        camDist * cosBeta * cosf(alpha)
    );
    return basePos + camOffset;
}

// Dibujar cubo
void drawCube(glm::mat4 P, glm::mat4 V, glm::mat4 M, glm::vec4 color, bool enableLighting = true)
{
    shader.setMat4("uPVM", P * V * M);
    shader.setMat4("uModel", M);
    shader.setMat4("uView", V);
    shader.setBool("uAnimateTail", false);
    shader.setVec4("uColor", color);
    shader.setBool("useTexture", false);
    shader.setBool("uEnableLighting", enableLighting);
    
    if (enableLighting) {
        shader.setVec3("uAmbientLight", ambientLight);
        shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
        shader.setVec3("uDirLightColor", dirLightColor);
        shader.setVec3("uMovingLightPos", movingLightPos);
        shader.setVec3("uMovingLightColor", movingLightColor);
        shader.setBool("uMovingLightEnabled", movingLightEnabled);
    }
    
    cubeModel.renderModel(GL_TRIANGLES);
}

// Dibujar Pez
void drawPez(glm::mat4 P, glm::mat4 V, Pez &pez)
{
    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, pez.posicion);
    M = glm::rotate(M, pez.anguloDireccion, glm::vec3(0, 1, 0));
    M = glm::scale(M, glm::vec3(pez.escala * 0.5f));

    float velocidadNado = glm::length(pez.velocidad);
    float velocidadAnimacion = velocidadNado * 2.2f;
    
    if (pez.persigiendoComida) {
        velocidadAnimacion *= 6.0f; 
    }
    
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-4.0f, -4.0f);

    shader.useShaders();
    shader.setMat4("uPVM", P * V * M);
    shader.setMat4("uModel", M);
    shader.setMat4("uView", V);
    shader.setFloat("uTime", t_global * velocidadAnimacion + pez.fase * 2.0f);
    shader.setBool("uAnimateTail", true);
    shader.setBool("useTexture", false);
    shader.setVec4("uColor", pez.color);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    
    fishModel.renderModel(GL_TRIANGLES);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
}

void actualizarPeces(float dt)
{
    if (peces_pausados) return;

    const float limiteX = 2.6f;
    const float limiteY_min = -3.8f;
    const float limiteY_max = 0.3f;
    const float limiteZ_min = -13.5f;
    const float limiteZ_max = -10.5f;
    const float distanciaEvitacion = 1.5f;
    const float margenGiro = 0.5f;

    for (int i = 0; i < NUM_PECES; i++) {
        peces[i].tiempoOndulacion += dt;
        
        float velocidadBase = glm::length(peces[i].velocidadOriginal);
        
        // Buscar comida cercana
        int comidaCercana = -1;
        float distanciaMin = 999999.0f;
        for (int c = 0; c < MAX_COMIDA; c++) {
            if (comidas[c].activa) {
                float dist = glm::distance(peces[i].posicion, comidas[c].posicion);
                if (dist < distanciaMin) {
                    distanciaMin = dist;
                    comidaCercana = c;
                }
            }
        }

        // perseguir comida 
        if (comidaCercana != -1 && distanciaMin < 8.0f) {
            peces[i].persigiendoComida = true;  
            
            glm::vec3 direccion = glm::normalize(comidas[comidaCercana].posicion - peces[i].posicion);
            peces[i].anguloObjetivo = atan2(direccion.x, direccion.z);
            peces[i].alturaObjetivo = comidas[comidaCercana].posicion.y;
            
            if (distanciaMin > 1.5f) {
                velocidadBase = velocidadBase * (1.8f + (8.0f - distanciaMin) * 0.2f);
            } else if (distanciaMin > 0.5f) {
                velocidadBase = velocidadBase * 1.6f;
            } else {
                velocidadBase = velocidadBase * (0.5f + distanciaMin);
            }
            
            // Comer comida cuando están cerca
            if (distanciaMin < 0.4f) {
                comidas[comidaCercana].escala -= dt * 1.5f;  
                if (comidas[comidaCercana].escala <= 0.0f) {
                    comidas[comidaCercana].activa = false;  
                }
            }
        }
        else {
            peces[i].persigiendoComida = false;  // No hay comida cerca
            
            // nadar hacia adelante
            peces[i].tiempoCambio -= dt;
            if (peces[i].tiempoCambio <= 0.0f) {
                peces[i].tiempoCambio = 3.0f + (float)rand() / RAND_MAX * 4.0f;
                
                // Cambiar altura para explorar todo el volumen vertical
                float random = (float)rand() / RAND_MAX;
                if (random < 0.33f) {
                    peces[i].alturaObjetivo = limiteY_min + ((float)rand() / RAND_MAX) * 1.2f;
                } else if (random < 0.66f) {
                    peces[i].alturaObjetivo = limiteY_min + 1.2f + ((float)rand() / RAND_MAX) * 1.5f;
                } else {
                    peces[i].alturaObjetivo = limiteY_min + 2.7f + ((float)rand() / RAND_MAX) * 1.3f;
                }
                float ajusteDireccion = ((float)rand() / RAND_MAX - 0.5f) * M_PI / 2.5f;
                peces[i].anguloObjetivo = peces[i].anguloDireccion + ajusteDireccion;
            }

            // Evitar paredes
            glm::vec3 direccionGiro(0.0f);
            bool necesitaGirar = false;
            
            glm::vec3 dirActual(sin(peces[i].anguloDireccion), 0.0f, cos(peces[i].anguloDireccion));
            glm::vec3 posFutura = peces[i].posicion + dirActual * velocidadBase * 1.2f;
            
            if (posFutura.x > limiteX - margenGiro) {
                direccionGiro.x = -1.0f;
                necesitaGirar = true;
            }
            if (posFutura.x < -limiteX + margenGiro) {
                direccionGiro.x = 1.0f;
                necesitaGirar = true;
            }
            if (posFutura.y > limiteY_max - margenGiro) {
                direccionGiro.y = -0.5f;
                necesitaGirar = true;
            }
            if (posFutura.y < limiteY_min + margenGiro) {
                direccionGiro.y = 0.5f;
                necesitaGirar = true;
            }
            if (posFutura.z > limiteZ_max - margenGiro) {
                direccionGiro.z = -1.0f;
                necesitaGirar = true;
            }
            if (posFutura.z < limiteZ_min + margenGiro) {
                direccionGiro.z = 1.0f;
                necesitaGirar = true;
            }
            
            if (necesitaGirar) {
                glm::vec3 nuevaDireccion = glm::normalize(dirActual + direccionGiro * 2.0f);
                peces[i].anguloObjetivo = atan2(nuevaDireccion.x, nuevaDireccion.z);
            }
        }

        // Evitar colisiones entre peces 
        glm::vec3 fuerzaEvitacion(0.0f);
        bool colisionDetectada = false;
        const float radioColision = 0.4f;
        
        for (int j = 0; j < NUM_PECES; j++) {
            if (i != j) {
                glm::vec3 diferencia = peces[i].posicion - peces[j].posicion;
                float dist = glm::length(diferencia);
                
                if (dist < radioColision && dist > 0.01f) {
                    glm::vec3 direccionSeparacion = glm::normalize(diferencia);
                    float solapamiento = radioColision - dist;
                    peces[i].posicion += direccionSeparacion * solapamiento * 0.5f;
                    colisionDetectada = true;
                    peces[i].anguloObjetivo = atan2(direccionSeparacion.x, direccionSeparacion.z);
                }
                else if (dist < distanciaEvitacion && dist > 0.01f) {
                    glm::vec3 direccionEvitacion = glm::normalize(diferencia);
                    float fuerza = (distanciaEvitacion - dist) / distanciaEvitacion;
                    fuerzaEvitacion += direccionEvitacion * fuerza * 2.0f;
                }
            }
        }
        
        if (!colisionDetectada && glm::length(fuerzaEvitacion) > 0.01f) {
            glm::vec3 dirActual(sin(peces[i].anguloDireccion), 0.0f, cos(peces[i].anguloDireccion));
            glm::vec3 nuevaDireccion = glm::normalize(dirActual + fuerzaEvitacion);
            peces[i].anguloObjetivo = atan2(nuevaDireccion.x, nuevaDireccion.z);
        }

        float difAngulo = peces[i].anguloObjetivo - peces[i].anguloDireccion;
        while (difAngulo > M_PI) difAngulo -= 2.0f * M_PI;
        while (difAngulo < -M_PI) difAngulo += 2.0f * M_PI;
        
        float velocidadGiro = 1.2f * dt;
        peces[i].anguloDireccion += glm::clamp(difAngulo, -velocidadGiro, velocidadGiro);
        
        // Calcular dirección de nado
        glm::vec3 direccionNado;
        direccionNado.x = sin(peces[i].anguloDireccion);
        direccionNado.z = cos(peces[i].anguloDireccion);
        direccionNado.y = 0.0f;
        
        // Movimiento vertical suave
        float difAltura = peces[i].alturaObjetivo - peces[i].posicion.y;
        float velocidadVertical = glm::clamp(difAltura * 0.5f, -0.3f, 0.3f);
        direccionNado.y = velocidadVertical;
        
        glm::vec3 dirHorizontal = glm::normalize(glm::vec3(direccionNado.x, 0.0f, direccionNado.z));
        peces[i].velocidad = dirHorizontal * velocidadBase + glm::vec3(0.0f, velocidadVertical, 0.0f);

        // Actualizar posición
        peces[i].posicion += peces[i].velocidad * dt;

        // Limitar posición
        peces[i].posicion.x = glm::clamp(peces[i].posicion.x, -limiteX, limiteX);
        peces[i].posicion.y = glm::clamp(peces[i].posicion.y, limiteY_min, limiteY_max);
        peces[i].posicion.z = glm::clamp(peces[i].posicion.z, limiteZ_min, limiteZ_max);
    }
}

void actualizarComida(float dt)
{
    for (int i = 0; i < MAX_COMIDA; i++) {
        if (comidas[i].activa) {
            comidas[i].posicion.y -= 0.12f * dt;  
            if (comidas[i].posicion.y < -3.5f) {
                comidas[i].activa = false;
            }
        }
    }
}

void actualizarBurbujas(float dt)
{
    const float limiteX = 2.6f;
    const float limiteZ_min = -13.5f;
    const float limiteZ_max = -10.5f;
    const float superficieY = 0.2f;
    
    for (int i = 0; i < MAX_BURBUJAS; i++) {
        if (burbujas[i].activa) {
            burbujas[i].posicion.y += burbujas[i].velocidadSubida * dt;
            
            burbujas[i].fase += dt * 2.0f;
            burbujas[i].posicion.x += sin(burbujas[i].fase) * burbujas[i].oscilacionX * dt;
            burbujas[i].posicion.z += cos(burbujas[i].fase * 0.7f) * burbujas[i].oscilacionZ * dt;
            
            burbujas[i].posicion.x = glm::clamp(burbujas[i].posicion.x, -limiteX + 0.2f, limiteX - 0.2f);
            burbujas[i].posicion.z = glm::clamp(burbujas[i].posicion.z, limiteZ_min + 0.2f, limiteZ_max - 0.2f);
            
            if (burbujas[i].posicion.y > superficieY) {
                burbujas[i].activa = false;
            }
        }
    }
}

void generarBurbuja()
{
    for (int i = 0; i < MAX_BURBUJAS; i++) {
        if (!burbujas[i].activa) {
            int pezIndex = rand() % peces_visibles;
            glm::vec3 posPez = peces[pezIndex].posicion;
            
            float offsetX = (rand() % 40 - 20) / 100.0f;  
            float offsetY = (rand() % 30 - 15) / 100.0f;  
            float offsetZ = 0.1f + (rand() % 20) / 100.0f;  
            
            float dirX = sin(peces[pezIndex].anguloDireccion);
            float dirZ = cos(peces[pezIndex].anguloDireccion);
            
            burbujas[i].posicion = glm::vec3(
                posPez.x + dirX * offsetZ + offsetX,
                posPez.y + offsetY,
                posPez.z + dirZ * offsetZ
            );
            
            burbujas[i].escala = 0.020f + (rand() % 60) / 3000.0f;  
            burbujas[i].velocidadSubida = 0.25f + (rand() % 80) / 400.0f;  
            burbujas[i].oscilacionX = 0.06f + (rand() % 40) / 500.0f;  
            burbujas[i].oscilacionZ = 0.06f + (rand() % 40) / 500.0f;
            burbujas[i].fase = (rand() % 628) / 100.0f;  
            burbujas[i].activa = true;
            break; 
        }
    }
}

// Dibujar comida 
void drawComida(glm::mat4 P, glm::mat4 V, Comida &comida)
{
    if (!comida.activa) return;

    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, comida.posicion);
    M = glm::scale(M, glm::vec3(0.04f * comida.escala));  

    shader.useShaders();
    shader.setMat4("uPVM", P * V * M);
    shader.setMat4("uModel", M);
    shader.setMat4("uView", V);
    shader.setVec4("uColor", comida.color);
    shader.setBool("useTexture", false);
    shader.setFloat("uTime", t_global);
    shader.setBool("uAnimateTail", false);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    sphereModel.renderModel(GL_TRIANGLES);
}

// Dibujar burbuja
void drawBurbuja(glm::mat4 P, glm::mat4 V, Burbuja &burbuja)
{
    if (!burbuja.activa) return;

    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, burbuja.posicion);
    M = glm::scale(M, glm::vec3(burbuja.escala));

    shader.useShaders();
    shader.setMat4("uPVM", P * V * M);
    shader.setMat4("uModel", M);
    shader.setMat4("uView", V);
    shader.setVec4("uColor", glm::vec4(0.8f, 0.9f, 1.0f, 0.5f));
    shader.setBool("useTexture", false);
    shader.setFloat("uTime", t_global);
    shader.setBool("uAnimateTail", false);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    sphereModel.renderModel(GL_TRIANGLES);
}

// Actualizar ventilador
void actualizarVentilador(float dt)
{
    ventilador.anguloAspas += ventilador.velocidadRotacion * dt;
    if (ventilador.anguloAspas > 360.0f) {
        ventilador.anguloAspas -= 360.0f;
    }
    
    // Rotación del palo (más lenta que las aspas)
    ventilador.anguloRotacionPalo += 90.0f * dt;  // 90 grados por segundo
    if (ventilador.anguloRotacionPalo > 360.0f) {
        ventilador.anguloRotacionPalo -= 360.0f;
    }
}

// Dibujar ventilador
void drawVentilador(glm::mat4 P, glm::mat4 V)
{
    shader.useShaders();
    shader.setBool("uAnimateTail", false);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    
    const float CUBE_HALF = 1.0f;

    const float paloScaleY = 3.0f;   
    const float baseScaleY = 0.15f;
    const float paloHeight = 2.0f * CUBE_HALF * paloScaleY; 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ventiladorTexture);
    shader.setInt("uTexture", 0);
    shader.setBool("useTexture", true);

    // Dibujar base (cubo achatado)
    glm::mat4 baseMatrix(1.0f);
    baseMatrix = glm::translate(baseMatrix, ventilador.posicion);
    baseMatrix = glm::translate(baseMatrix, glm::vec3(0.0f, -baseScaleY * CUBE_HALF, 0.0f)); // bajar "media base"
    baseMatrix = glm::scale(baseMatrix, glm::vec3(0.9f, baseScaleY, 0.9f));

    shader.setMat4("uPVM", P * V * baseMatrix);
    shader.setMat4("uModel", baseMatrix);
    shader.setMat4("uView", V);
    shader.setVec4("uColor", glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    cubeModel.renderModel(GL_TRIANGLES);

    // Dibujar palo (cubo alargado)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ventiladorTexture);
    shader.setInt("uTexture", 0);
    shader.setBool("useTexture", true);
    
    glm::mat4 paloMatrix(1.0f);
    paloMatrix = glm::translate(paloMatrix, ventilador.posicion);
    paloMatrix = glm::rotate(paloMatrix, glm::radians(ventilador.anguloRotacionPalo), glm::vec3(0.0f, 1.0f, 0.0f));
    paloMatrix = glm::translate(paloMatrix, glm::vec3(0.0f, paloScaleY * CUBE_HALF, 0.0f)); 
    paloMatrix = glm::scale(paloMatrix, glm::vec3(0.12f, paloScaleY, 0.12f));

    shader.setMat4("uPVM", P * V * paloMatrix);
    shader.setMat4("uModel", paloMatrix);
    shader.setMat4("uView", V);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    cubeModel.renderModel(GL_TRIANGLES);
    
    // Dibujar esfera (centro superior del ventilador)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ventiladorTexture);
    shader.setInt("uTexture", 0);
    shader.setBool("useTexture", true);
    
    glm::mat4 esferaMatrix = glm::mat4(1.0f);
    esferaMatrix = glm::translate(esferaMatrix, ventilador.posicion);
    esferaMatrix = glm::rotate(esferaMatrix, glm::radians(ventilador.anguloRotacionPalo), glm::vec3(0.0f, 1.0f, 0.0f));  // Girar con el palo
    esferaMatrix = glm::translate(esferaMatrix, glm::vec3(0.0f, paloHeight, 0.0f));
    esferaMatrix = glm::scale(esferaMatrix, glm::vec3(0.25f));
    shader.setMat4("uPVM", P * V * esferaMatrix);
    shader.setMat4("uModel", esferaMatrix);
    shader.setMat4("uView", V);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    sphereModel.renderModel(GL_TRIANGLES);
    
    // Dibujar aspas (5 conos rotando)
    const int numAspas = 5;
    for (int i = 0; i < numAspas; i++) {
        float anguloAspa = ventilador.anguloAspas + (i * 72.0f);  // 72 grados entre cada aspa (360/5)
        
        glm::mat4 aspaMatrix = glm::mat4(1.0f);
        aspaMatrix = glm::translate(aspaMatrix, ventilador.posicion);
        aspaMatrix = glm::rotate(aspaMatrix, glm::radians(ventilador.anguloRotacionPalo), glm::vec3(0.0f, 1.0f, 0.0f));  // Girar con el palo
        aspaMatrix = glm::translate(aspaMatrix, glm::vec3(0.0f, paloHeight, 0.0f));
    
        aspaMatrix = glm::rotate(aspaMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        aspaMatrix = glm::rotate(aspaMatrix, glm::radians(ventilador.anguloAspas), glm::vec3(0.0f, 1.0f, 0.0f));
        aspaMatrix = glm::rotate(aspaMatrix, glm::radians(anguloAspa), glm::vec3(0.0f, 1.0f, 0.0f));
        aspaMatrix = glm::translate(aspaMatrix, glm::vec3(0.6f, 0.0f, 0.0f));
        aspaMatrix = glm::rotate(aspaMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        aspaMatrix = glm::scale(aspaMatrix, glm::vec3(0.15f, 0.8f, 0.15f));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ventiladorTexture);
        shader.setInt("uTexture", 0);
        shader.setBool("useTexture", true);
        
        shader.setMat4("uPVM", P * V * aspaMatrix);
        shader.setMat4("uModel", aspaMatrix);
        shader.setMat4("uView", V);
        shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        coneModel.renderModel(GL_TRIANGLES);
    }
}

void echarComida()
{
    int contador = 0;
    for (int i = 0; i < MAX_COMIDA && contador < 50; i++) {
        if (!comidas[i].activa) {
            comidas[i].posicion = glm::vec3(
                (rand() % 400 - 200) / 100.0f,
                0.2f,
                (rand() % 200 - 1300) / 100.0f
            );

            int colorType = rand() % 6;
            switch(colorType) {
                case 0: comidas[i].color = glm::vec4(1.0f, 0.9f, 0.2f, 1.0f); break;
                case 1: comidas[i].color = glm::vec4(1.0f, 0.4f, 0.2f, 1.0f); break;
                case 2: comidas[i].color = glm::vec4(0.9f, 0.2f, 0.2f, 1.0f); break;
                case 3: comidas[i].color = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f); break;
                case 4: comidas[i].color = glm::vec4(0.8f, 0.6f, 0.3f, 1.0f); break;
                case 5: comidas[i].color = glm::vec4(0.9f, 0.5f, 0.7f, 1.0f); break;
            }

            comidas[i].escala = 1.0f; 
            comidas[i].activa = true;
            contador++;
        }
    }
}

void renderScene(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& eye, float timeValue)
{
    // Fondo de habitación
    glDisable(GL_DEPTH_TEST);
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    
    glm::mat4 roomBackMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -15.0f));
    roomBackMatrix = glm::scale(roomBackMatrix, glm::vec3(20.0f, 15.0f, 1.0f));
    shader.setMat4("uPVM", projection * view * roomBackMatrix);
    shader.setMat4("uModel", roomBackMatrix);
    shader.setMat4("uView", view);
    shader.setBool("useTexture", true);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    shader.setBool("uEnableLighting", false);
    glBindTexture(GL_TEXTURE_2D, roomBackTexture);
    glBindVertexArray(backgroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Fondo del acuario
    glm::mat4 backgroundMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.7f, -14.5f));
    backgroundMatrix = glm::scale(backgroundMatrix, glm::vec3(5.0f, 3.5f, 1.0f));
    shader.setMat4("uPVM", projection * view * backgroundMatrix);
    shader.setMat4("uModel", backgroundMatrix);
    shader.setMat4("uView", view);
    shader.setBool("useTexture", true);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    shader.setBool("uEnableLighting", false);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glBindVertexArray(backgroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    // Mesa
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glm::mat4 tableMatrix(1.0f);
    tableMatrix = glm::translate(tableMatrix, glm::vec3(0.0f, -10.0f, -12.0f));
    tableMatrix = glm::rotate(tableMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    tableMatrix = glm::scale(tableMatrix, glm::vec3(12.0f, 12.0f, 12.0f));
    shader.setMat4("uPVM", projection * view * tableMatrix);
    shader.setMat4("uModel", tableMatrix);
    shader.setMat4("uView", view);
    shader.setFloat("uTime", timeValue);
    shader.setBool("uAnimateTail", false);
    shader.setVec4("uColor", glm::vec4(0.85f, 0.85f, 0.85f, 1.0f));
    shader.setBool("useTexture", false);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    glFrontFace(GL_CW);
    tableModel.renderModel(GL_FILL);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);

    // Arena del fondo
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sandTexture);
    glm::mat4 sandMatrix(1.0f);
    sandMatrix = glm::translate(sandMatrix, glm::vec3(0.0f, -5.19f, -12.0f));
    sandMatrix = glm::rotate(sandMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    sandMatrix = glm::scale(sandMatrix, glm::vec3(5.0f, 2.5f, 0.01f));
    shader.setMat4("uPVM", projection * view * sandMatrix);
    shader.setMat4("uModel", sandMatrix);
    shader.setMat4("uView", view);
    shader.setFloat("uTime", timeValue);
    shader.setBool("uAnimateTail", false);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    shader.setBool("useTexture", true);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    cubeModel.renderModel(GL_FILL);
    glEnable(GL_BLEND);

    // Agua del acuario 
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    glm::mat4 waterTableMatrix(1.0f);
    waterTableMatrix = glm::translate(waterTableMatrix, glm::vec3(0.0f, -1.7f, -12.0f));
    waterTableMatrix = glm::scale(waterTableMatrix, glm::vec3(5.0f, 3.5f, 2.5f));
    shader.setMat4("uPVM", projection * view * waterTableMatrix);
    shader.setMat4("uModel", waterTableMatrix);
    shader.setMat4("uView", view);
    shader.setFloat("uTime", timeValue);
    shader.setBool("uAnimateTail", false);
    shader.setVec4("uColor", glm::vec4(0.12f, 0.4f, 0.6f, 0.32f));
    shader.setBool("useTexture", false);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    glDepthMask(GL_FALSE);
    cubeModel.renderModel(GL_FILL);
    glDepthMask(GL_TRUE);

    // Corales en el fondo
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);
    
    // Corales 
    shader.setBool("uAnimateTail", false);
    shader.setBool("uEnableLighting", true);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, coralTexture);
    shader.setInt("uTexture", 0);
    shader.setBool("useTexture", true);
    shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    glm::vec3 coralPositions[11] = {
        glm::vec3(-2.3f, -5.2f, -10.2f),
        glm::vec3(1.8f, -5.2f, -11.3f),
        glm::vec3(-1.5f, -5.2f, -12.8f),
        glm::vec3(2.7f, -5.2f, -10.7f),
        glm::vec3(0.5f, -5.2f, -11.8f),
        glm::vec3(-0.8f, -5.2f, -13.2f),
        glm::vec3(-0.2f, -5.2f, -10.5f),
        glm::vec3(2.0f, -5.2f, -12.3f),
        glm::vec3(-1.8f, -5.2f, -11.7f),
        glm::vec3(0.9f, -5.2f, -10.8f),
        glm::vec3(-2.1f, -5.2f, -12.1f)
    };
    
    for (int i = 0; i < 11; i++) {
        glm::mat4 coralMatrix(1.0f);
        coralMatrix = glm::translate(coralMatrix, coralPositions[i]);
        coralMatrix = glm::rotate(coralMatrix, glm::radians(-90.0f), glm::vec3(1, 0, 0)); 
        
        float frequencia = 1.0f + (i * 0.1f);  
        float amplitudX = 0.8f + sin(i * 0.5f) * 0.3f;  
        float amplitudZ = 0.6f + cos(i * 0.7f) * 0.2f;  
        float faseX = i * 0.8f;  
        float faseZ = i * 1.2f;
        
        float anguloX = sin(timeValue * frequencia + faseX) * amplitudX;
        float anguloZ = cos(timeValue * frequencia * 0.8f + faseZ) * amplitudZ;
        
        coralMatrix = glm::rotate(coralMatrix, glm::radians(anguloX), glm::vec3(1, 0, 0));
        coralMatrix = glm::rotate(coralMatrix, glm::radians(anguloZ), glm::vec3(0, 0, 1));
        
        float escala;
        if (i < 6) escala = 0.025f;
        else if (i < 9) escala = 0.015f;
        else escala = 0.014f;
        coralMatrix = glm::scale(coralMatrix, glm::vec3(escala));
        
        shader.setMat4("uPVM", projection * view * coralMatrix);
        shader.setMat4("uModel", coralMatrix);
        shader.setMat4("uView", view);
        shader.setFloat("uTime", timeValue);
        
        coralModel.renderModel(GL_TRIANGLES);
    }

    // Ventilador
    drawVentilador(projection, view);

    // Peces
    shader.useShaders();
    shader.setVec3("uViewPos", eye);
    shader.setBool("uEnableLighting", true);
    shader.setVec3("uAmbientLight", ambientLight);
    shader.setVec3("uDirLightDir", glm::normalize(dirLightDir));
    shader.setVec3("uDirLightColor", dirLightColor);
    shader.setVec3("uMovingLightPos", movingLightPos);
    shader.setVec3("uMovingLightColor", movingLightColor);
    shader.setBool("uMovingLightEnabled", movingLightEnabled);

    for (int i = 0; i < peces_visibles; i++) {
        drawPez(projection, view, peces[i]);
    }

    // Comida
    for (int i = 0; i < MAX_COMIDA; i++) {
        drawComida(projection, view, comidas[i]);
    }
    
    // Burbujas 
    glDepthMask(GL_FALSE);  
    for (int i = 0; i < MAX_BURBUJAS; i++) {
        drawBurbuja(projection, view, burbujas[i]);
    }
    glDepthMask(GL_TRUE);  
}

int main()
{
    srand((unsigned)time(nullptr));

    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pecera 3D - Proyecto Final", nullptr, nullptr);
    if (!window) {
        std::cerr << "No se pudo crear la ventana GLFW\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "ERROR: No se pudo iniciar GLEW\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.85f, 0.82f, 0.75f, 1.0f);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    shader.initShaders(
        "resources/shaders/vshader.glsl",
        "resources/shaders/fshader.glsl"
    );

    cubeModel.initModel("resources/models/cube.obj");
    fishModel.initModel("resources/models/pez.obj");
    sphereModel.initModel("resources/models/sphere.obj");
    tableModel.initModel("resources/models/Table.obj");
    coralModel.initModel("resources/models/coral.obj");
    coneModel.initModel("resources/models/cone.obj");

    sandTexture = loadTexture("resources/textures/arena.jpg");
    coralTexture = loadTexture("resources/textures/coral.jpg");
    ventiladorTexture = loadTexture("resources/textures/ventilador1.jpg");
    
    createBackgroundPlane();
    roomBackTexture = loadTexture("resources/textures/room_back.jpg");
    backgroundTexture = loadTexture("resources/textures/acuario.jpeg");

    // Inicializar peces
    srand(time(NULL));
    peces[0] = {glm::vec3(-2.0f, -0.3f, -11.2f), glm::vec3(0.6f, 0.1f, 0.25f), glm::vec3(0.6f, 0.1f, 0.25f), glm::vec4(0.2f, 0.6f, 0.8f, 1.0f), 0.0f, 0.37f};
    peces[1] = {glm::vec3(2.0f, -2.8f, -12.8f), glm::vec3(-0.4f, -0.15f, 0.35f), glm::vec3(-0.4f, -0.15f, 0.35f), glm::vec4(0.9f, 0.5f, 0.2f, 1.0f), 1.5f, 0.35f};
    peces[2] = {glm::vec3(-1.5f, -3.0f, -11.0f), glm::vec3(0.5f, 0.1f, -0.4f), glm::vec3(0.5f, 0.1f, -0.4f), glm::vec4(0.8f, 0.2f, 0.3f, 1.0f), 3.0f, 0.39f};
    peces[3] = {glm::vec3(1.8f, -0.8f, -12.5f), glm::vec3(0.35f, 0.15f, -0.4f), glm::vec3(0.35f, 0.15f, -0.4f), glm::vec4(0.9f, 0.9f, 0.3f, 1.0f), 4.5f, 0.36f};
    peces[4] = {glm::vec3(0.2f, -1.8f, -11.3f), glm::vec3(-0.55f, 0.0f, 0.3f), glm::vec3(-0.55f, 0.0f, 0.3f), glm::vec4(0.6f, 0.3f, 0.8f, 1.0f), 2.0f, 0.38f};
    peces[5] = {glm::vec3(-1.8f, -1.3f, -13.0f), glm::vec3(0.4f, -0.08f, 0.4f), glm::vec3(0.4f, -0.08f, 0.4f), glm::vec4(0.3f, 0.8f, 0.6f, 1.0f), 3.5f, 0.39f};
    peces[6] = {glm::vec3(1.2f, -2.5f, -11.8f), glm::vec3(-0.5f, 0.15f, -0.35f), glm::vec3(-0.5f, 0.15f, -0.35f), glm::vec4(0.9f, 0.3f, 0.5f, 1.0f), 5.0f, 0.37f};
    peces[7] = {glm::vec3(-0.8f, -0.5f, -12.4f), glm::vec3(0.5f, -0.1f, 0.35f), glm::vec3(0.5f, -0.1f, 0.35f), glm::vec4(0.4f, 0.5f, 0.9f, 1.0f), 1.0f, 0.38f};
    peces[8] = {glm::vec3(0.5f, -2.2f, -12.0f), glm::vec3(0.45f, 0.1f, 0.3f), glm::vec3(0.45f, 0.1f, 0.3f), glm::vec4(0.7f, 0.4f, 0.9f, 1.0f), 2.5f, 0.36f};
    
    for (int i = 0; i < NUM_PECES; i++) {
        glm::vec3 velNorm = glm::normalize(peces[i].velocidad);
        peces[i].anguloDireccion = atan2(velNorm.x, velNorm.z);
        peces[i].anguloObjetivo = peces[i].anguloDireccion;
        peces[i].tiempoOndulacion = (float)rand() / RAND_MAX * 6.28f;
        peces[i].amplitudOndulacion = 0.02f + (float)rand() / RAND_MAX * 0.015f;
        peces[i].frecuenciaOndulacion = 4.0f + (float)rand() / RAND_MAX * 1.0f;
        peces[i].tiempoCambio = (float)rand() / RAND_MAX * 3.0f;
        peces[i].alturaObjetivo = peces[i].posicion.y;
        peces[i].modeloTipo = 0;
        peces[i].persigiendoComida = false;
    }

    // Inicializar comida
    for (int i = 0; i < MAX_COMIDA; i++) {
        comidas[i].activa = false;
    }
    
    // Inicializar burbujas
    for (int i = 0; i < MAX_BURBUJAS; i++) {
        burbujas[i].activa = false;
    }
    
    // Inicializar ventilador
    ventilador.posicion = glm::vec3(10.0f, -8.8f, -11.5f);  
    ventilador.anguloAspas = 0.0f;
    ventilador.velocidadRotacion = 360.0f;  // Velocidad de rotación de las aspas (grados/segundo)
    ventilador.anguloMovimiento = 0.0f;     
    ventilador.radio = 10.0f;
    ventilador.anguloRotacionPalo = 0.0f;  // Inicializar rotación del palo               
    
    // Generar burbujas iniciales cerca de los peces
    for (int i = 0; i < 15; i++) {
        if (i < MAX_BURBUJAS) {
            int pezIndex = rand() % peces_visibles;
            glm::vec3 posPez = peces[pezIndex].posicion;
            
            float offsetX = (rand() % 60 - 30) / 100.0f;  
            float offsetY = (rand() % 100) / 100.0f;      
            float offsetZ = (rand() % 40 - 20) / 100.0f;  
            
            burbujas[i].posicion = glm::vec3(
                posPez.x + offsetX,
                posPez.y + offsetY,
                posPez.z + offsetZ
            );
            burbujas[i].escala = 0.020f + (rand() % 60) / 3000.0f;
            burbujas[i].velocidadSubida = 0.25f + (rand() % 80) / 400.0f;
            burbujas[i].oscilacionX = 0.06f + (rand() % 40) / 500.0f;
            burbujas[i].oscilacionZ = 0.06f + (rand() % 40) / 500.0f;
            burbujas[i].fase = (rand() % 628) / 100.0f;
            burbujas[i].activa = true;
        }
    }

    // Bucle principal
    float lastTime = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        t_global = currentTime;

        actualizarPeces(deltaTime);
        actualizarComida(deltaTime);
        actualizarBurbujas(deltaTime);
        actualizarVentilador(deltaTime);
        
        // Generar burbujas continuamente 
        tiempoUltimaBurbuja += deltaTime;
        if (tiempoUltimaBurbuja >= 0.25f + (rand() % 100) / 200.0f) {  
            generarBurbuja();
            tiempoUltimaBurbuja = 0.0f;
        }
        
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (height == 0) height = 1;

        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        const glm::vec3 eye = computeCameraPosition();
        const glm::mat4 view = glm::lookAt(eye, camOffset, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        renderScene(projection, view, eye, t_global);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}