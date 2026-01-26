#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "model/Point.h"
#include "model/Ray.h"
#include "model/Light.h"
#include "model/PointLight.h"
#include "model/SpotLight.h"
#include "model/Objects/Sphere.h"
#include "model/Objects/Plane.h"
#include "model/Objects/Cilinder.h"
#include "model/Objects/Cone.h"
#include "model/Objects/Triangle.h"
#include "model/Objects/Mesh.h"
#include "operations/Shading.h"
#include "model/Objects/Object.h"
#include "operations/Operations.h"
#include "model/ObjectCache.h"
#include "model/Tile.h"
#include "model/Camera.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace Operations;

#define MAX_HEIGHT 500
#define MAX_WIDHT 500

#define USE_GPU_ENGINE 0

bool lightOn = true;
bool triggerPick = false;
bool cameraDirty = true;
bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;
double lastX = 250, lastY = 250;
bool firstMouse = true;


extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key) {
        case GLFW_KEY_W: moveForward = true; break;
        case GLFW_KEY_S: moveBackward = true; break;
        case GLFW_KEY_A: moveLeft = true; break;
        case GLFW_KEY_D: moveRight = true; break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }

    if (action == GLFW_RELEASE)
    {
        switch (key) {
        case GLFW_KEY_W: moveForward = false; break;
        case GLFW_KEY_S: moveBackward = false; break;
        case GLFW_KEY_A: moveLeft = false; break;
        case GLFW_KEY_D: moveRight = false; break;
        }
    }
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);

    static double lastX = xpos;
    static double lastY = ypos;

    float sensitivity = 0.002f;

    float dx = (xpos - lastX) * sensitivity;
    float dy = (lastY - ypos) * sensitivity;

    lastX = xpos;
    lastY = ypos;

    camera->rotate(dx, dy);
    cameraDirty = true;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        triggerPick = true;
    }
}

void renderTiles(
    const Tile& tile,
    int nCol, int nLin,
    float wJanela, float hJanela,
    Camera& camera,
    const std::vector<ObjectCache>& sceneCache,
    const std::vector<Light*>& lights,
    const glm::vec3& I_A,
    std::vector<glm::vec3>& framebuffer
) {
    float Dx = wJanela / nCol;
    float Dy = hJanela / nLin;

    for (int l = tile.y; l < tile.y + tile.h; ++l) {
        float y = hJanela / 2.0f - Dy / 2.0f - l * Dy;
        for (int c = tile.x; c < tile.x + tile.w; ++c) {
            float x = -wJanela / 2 + Dx / 2.0f + c * Dx;
            float px = x / (wJanela / 2.0f);
            float py = y / (hJanela / 2.0f);

            Ray ray = camera.generateRay(px, py);

            glm::vec3 color = I_A;
            float t_min = FLT_MAX;
            Object* hitObject = nullptr;
            HitRecord bestMeshHit;

            for (const auto& item : sceneCache) {

                if (!item.isPlane) {
                    float tNear, tFar;
                    if (!item.box.intersect(ray, tNear, tFar)) {
                        continue;
                    }
                }

                if (item.isMesh) {
                    Mesh* meshPtr = static_cast<Mesh*>(item.ptr);
                    HitRecord currentHit;
                    if (meshPtr->intersectWithHitRecord(ray, currentHit) && currentHit.t < t_min) {
                        t_min = currentHit.t;
                        hitObject = meshPtr;
                        bestMeshHit = currentHit;
                    }
                }
                else {
                    float t;
                    if (item.ptr->intersect(ray, t) && t < t_min) {
                        t_min = t;
                        hitObject = item.ptr;
                        bestMeshHit.hitTriangle = nullptr;
                        bestMeshHit.t = t;
                    }
                }
            }

            if (hitObject) {
                glm::vec3 Pi_world = glm::vec3(ray.origin.position) + t_min * glm::normalize(glm::vec3(ray.direction));
                glm::vec3 viewDir = glm::normalize(glm::vec3(ray.direction));
                glm::vec3 Pi_local = glm::vec3(hitObject->invModel * glm::vec4(Pi_world, 1.0f));
                glm::vec3 n_world;

                Mesh* meshPtr = dynamic_cast<Mesh*>(hitObject);
                if (meshPtr) {
                    n_world = meshPtr->getNormalFromHit(bestMeshHit, Pi_local);
                    const Triangle* tri = bestMeshHit.hitTriangle;
                    if (tri) {
                        meshPtr->K_ambient = tri->K_ambient;
                        meshPtr->K_diffuse = tri->K_diffuse;
                        meshPtr->K_specular = tri->K_specular;
                        meshPtr->shininess = tri->shininess;
                    }
                }
                else {
                    n_world = hitObject->getNormal(Pi_local, viewDir);
                }

                color = shade(Pi_world, n_world, ray, lights, I_A, *hitObject, sceneCache, bestMeshHit);
            }

            framebuffer[l * nCol + c] = color;
        }
    }
}

Object* pickObject(Camera& camera, const std::vector<ObjectCache>& sceneCache) {
    Ray ray = camera.generateRay(0.0f, 0.0f); // Centro da tela

    float t_min = FLT_MAX;
    Object* hitObject = nullptr;

    for (const auto& item : sceneCache) {
        float t;
        if (!item.isPlane) {
            float tNear, tFar;
            if (!item.box.intersect(ray, tNear, tFar)) continue;
        }

        if (item.isMesh) {
            Mesh* meshPtr = static_cast<Mesh*>(item.ptr);
            HitRecord hr;
            if (meshPtr->intersectWithHitRecord(ray, hr)) {
                if (hr.t < t_min) {
                    t_min = hr.t;
                    hitObject = item.ptr;
                }
            }
        }
        else {
            if (item.ptr->intersect(ray, t)) {
                if (t < t_min) {
                    t_min = t;
                    hitObject = item.ptr;
                }
            }
        }
    }
    return hitObject;
}

bool isAheadOfCamera(const AABB& box, const Camera& camera) {
    glm::vec3 camPos = glm::vec3(camera.position.position);
    glm::vec3 camDir = camera.forward;

    glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}
    };

    for (int i = 0; i < 8; i++) {
        glm::vec3 dirToPoint = corners[i] - camPos;
        if (glm::dot(dirToPoint, camDir) > -1.0f) {
            return true;
        }
    }
    return false;
}

int main(void)
{

    if (!glfwInit())
        return -1;


#pragma region report opengl errors to std
    //enable opengl debugging output.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //you might want to do this when testing the game for shipping


    GLFWwindow* window = window = glfwCreateWindow(MAX_HEIGHT, MAX_WIDHT, "TASK_5", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MAX_WIDHT, MAX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#pragma region report opengl errors to std
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#pragma endregion

    //shader loading example
    Shader s;
    s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");
    s.bind();
    GLint texLocation = glGetUniformLocation(s.id, "u_Texture");
    glUniform1i(texLocation, 0);

    // CÂMERA 
    Camera camera(
        Point(3.0f, 1.5f, 7.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::radians(60.0f),
        float(MAX_WIDHT) / float(MAX_HEIGHT),
        0.3f
    );

    glfwSetWindowUserPointer(window, &camera);

    // SCENE DEFINITION 
    std::vector<std::unique_ptr<Object>> objects;

    // CHÃO 
    {
        std::vector<Triangle> floorTris;
        float size = 7.0f;
        float height = 0.0f;

        glm::vec3 v0(0.0f, height, 14.0f);
        glm::vec3 v1(10.0f, height, 14.0f);
        glm::vec3 v2(10.0f, height, 0.0f);
        glm::vec3 v3(0.0f, height, 0.0f);

        float repeatFactor = 2.0f;
        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(repeatFactor, 0.0f);
        glm::vec2 uv2(repeatFactor, repeatFactor);
        glm::vec2 uv3(0.0f, repeatFactor);

        glm::vec3 white(1.0f);

        floorTris.push_back(Triangle(
            Point(v0.x, v0.y, v0.z),
            Point(v1.x, v1.y, v1.z),
            Point(v2.x, v2.y, v2.z),
            uv0, uv1, uv2,
            white, white, white, 32.0f
        ));

        floorTris.push_back(Triangle(
            Point(v0.x, v0.y, v0.z),
            Point(v2.x, v2.y, v2.z),
            Point(v3.x, v3.y, v3.z),
            uv0, uv2, uv3,
            white, white, white, 32.0f
        ));

        auto floorMesh = std::make_unique<Mesh>(floorTris);
        floorMesh->loadTexture(RESOURCES_PATH "textures/chao_madeira.jpg");
        objects.push_back(std::move(floorMesh));
    }

    glm::vec3 white(1.0f);
    float wallSize = 10.0f;
    float wallHeight = 1.5f;

    // PAREDE DIREITA 
    {
        std::vector<Triangle> wallTris;
        float wallX = 6.0f;
        float zSize = 10.0f;

        Point p1(wallX, 0.0f, 17.0f), p2(wallX, 0.0f, 0.0f);
        Point p3(wallX, 3.0f, 0.0f), p4(wallX, 3.0f, 17.0f);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // PAREDE ESQUERDA 
    {
        std::vector<Triangle> wallTris;
        float wallX = 0.0f;
        float zSize = 10.0f;

        Point p1(wallX, 0.0f, 0.0f), p2(wallX, 0.0f, 17.0f);
        Point p3(wallX, 3.0f, 17.0f), p4(wallX, 3.0f, 0.0f);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // PAREDE DE FUNDO 
    {
        std::vector<Triangle> wallTris;
        float posZ = 0.0f;
        float limitX = 3.0f;

        Point p1(0.0f, 0.0f, posZ), p2(6.0f, 0.0f, posZ);
        Point p3(6.0f, 3.0f, posZ), p4(0.0f, 3.0f, posZ);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 32.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 32.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // PAREDE DE TRÁS 
    {
        std::vector<Triangle> wallTris;
        float posZ = 14.0f;
        float limitX = 3.0f;

        Point p1(0.0f, 0.0f, posZ), p2(6.0f, 0.0f, posZ);
        Point p3(6.0f, 3.0f, posZ), p4(0.0f, 3.0f, posZ);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p3, p2, uv1, uv3, uv2, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p4, p3, uv1, uv4, uv3, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // TETO SEM TEXTURA 
    {
        std::vector<Triangle> roofTris;
        float limitX = 3.0f;
        float limitZ = 7.0f;
        float h = 3.0f;

        Point p1(0.0f, h, 14.0f), p2(6.0f, h, 14.0f);
        Point p3(6.0f, h, 0.0f), p4(0.0f, h, 0.0f);

        glm::vec2 uv(0, 0);
        glm::vec3 roofColor(0.933f);

        roofTris.push_back(Triangle(p1, p3, p2, uv, uv, uv, roofColor, roofColor, roofColor, 1.0f));
        roofTris.push_back(Triangle(p1, p4, p3, uv, uv, uv, roofColor, roofColor, roofColor, 1.0f));

        objects.push_back(std::make_unique<Mesh>(roofTris));
    }

    // ÁRVORE (TRONCO)
    objects.push_back(std::make_unique<Cilinder>(
        Point(1.0f, 0.0f, 3.0f, 1.0f),
        0.05f,
        0.9f,
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        true, true,
        glm::vec3(0.824f, 0.706f, 0.549f),
        glm::vec3(0.824f, 0.706f, 0.549f),
        glm::vec3(0.824f, 0.706f, 0.549f),
        10.0f,
        glm::vec3(0.824f, 0.706f, 0.549f),
        glm::vec3(0.824f, 0.706f, 0.549f),
        glm::vec3(0.824f, 0.706f, 0.549f),
        10.0f
    ));

    // ÁRVORE (FOLHAS/CONE)
    objects.push_back(std::make_unique<Cone>(
        Point(1.0f, 0.9f, 3.0f, 1.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        1.5f,
        0.9f,
        true,
        glm::vec3(0.0f, 1.0f, 0.498f),
        glm::vec3(0.0f, 1.0f, 0.498f),
        glm::vec3(0.0f, 1.0f, 0.498f),
        10.0f,
        glm::vec3(0.0f, 1.0f, 0.498f),
        glm::vec3(0.0f, 1.0f, 0.498f),
        glm::vec3(0.0f, 1.0f, 0.498f),
        10.0f
    ));

    // ESTRELA (ESFERA NO TOPO)
    objects.push_back(std::make_unique<Sphere>(
        Point(1.0f, 2.45f, 3.0f, 1.0f),
        0.05f,
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        10.0f
    ));

    // PRESENTE (CUBO
    {
        float a = 0.4f;
        glm::vec3 center(1.0f, 0.2f, 3.4f);
        float h = a / 2.0f;

        std::vector<Triangle> cubeTris;
        glm::vec3 v[8] = {
            center + glm::vec3(-h, -h,  h), center + glm::vec3(h, -h,  h),
            center + glm::vec3(h,  h,  h), center + glm::vec3(-h,  h,  h),
            center + glm::vec3(-h, -h, -h), center + glm::vec3(h, -h, -h),
            center + glm::vec3(h,  h, -h), center + glm::vec3(-h,  h, -h)
        };

        glm::vec3 color(1.0f, 1.0f, 1.0f);
        auto addQuad = [&](int a, int b, int c, int d) {
            glm::vec2 uvA(0.0f, 0.0f); glm::vec2 uvB(1.0f, 0.0f);
            glm::vec2 uvC(1.0f, 1.0f); glm::vec2 uvD(0.0f, 1.0f);
            cubeTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uvA, uvB, uvC, color, color, color, 32.0f));
            cubeTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uvA, uvC, uvD, color, color, color, 32.0f));
            };

        addQuad(0, 1, 2, 3); addQuad(1, 5, 6, 2); addQuad(5, 4, 7, 6);
        addQuad(4, 0, 3, 7); addQuad(3, 2, 6, 7); addQuad(4, 5, 1, 0);

        auto giftMesh = std::make_unique<Mesh>(cubeTris);
        giftMesh->loadTexture(RESOURCES_PATH "textures/presente.jpg");

        float angle = glm::radians(30.0f); // Gira 30 graus

        glm::mat4 R = Operations::rotateY(angle);
        glm::mat4 T = Operations::translate(center);    // Volta para a posição original
        glm::mat4 invT = Operations::translate(-center); // Leva para a origem (0,0,0) para girar

        // Ordem: T * R * invT
        giftMesh->model = T * R * invT;
        giftMesh->invModel = glm::inverse(giftMesh->model);

        // Matriz de normais para a luz bater corretamente nas faces giradas
        giftMesh->normalMatrix = glm::transpose(glm::inverse(glm::mat3(giftMesh->model)));

        objects.push_back(std::move(giftMesh));
    }

    // INTERRUPTOR 
    Object* interruptorPtr = nullptr;
    {
        float sWidth = 0.15f;
        float sHeight = 0.28f;
        float sDepth = 0.02f;

        glm::vec3 p(0.02f, 1.5f, 9.0f);

        std::vector<Triangle> swTris;
        glm::vec3 white(1.0f);

        Point p1(p.x, p.y - sHeight / 2, p.z + sWidth / 2);
        Point p2(p.x, p.y - sHeight / 2, p.z - sWidth / 2);
        Point p3(p.x, p.y + sHeight / 2, p.z - sWidth / 2);
        Point p4(p.x, p.y + sHeight / 2, p.z + sWidth / 2);

        float cropX = 0.20f;
        float cropY = 0.05f;

        glm::vec2 uv0(cropX, cropY);
        glm::vec2 uv1(1.0f - cropX, cropY);
        glm::vec2 uv2(1.0f - cropX, 1.0f - cropY);
        glm::vec2 uv3(cropX, 1.0f - cropY);

        swTris.push_back(Triangle(p1, p2, p3, uv0, uv1, uv2, white, white, white, 32.0f));
        swTris.push_back(Triangle(p1, p3, p4, uv0, uv2, uv3, white, white, white, 32.0f));

        auto swMesh = std::make_unique<Mesh>(swTris);
        swMesh->loadTexture(RESOURCES_PATH "textures/interruptor.png");
        interruptorPtr = swMesh.get();
        objects.push_back(std::move(swMesh));
    }

    // --- PORTA (Ao lado do interruptor) ---
    {
        std::vector<Triangle> doorTris;
        glm::vec3 doorColor(1.0f); // Branco para não alterar a cor da textura

        float doorW = 0.05f; // Espessura
        float doorH = 2.10f; // Altura padrão de porta
        float doorL = 0.90f; // Largura padrão (no eixo Z)

        glm::vec3 center(0.01f, doorH / 2.0f, 11.5f);

        glm::vec3 min = center - glm::vec3(doorW / 2, doorH / 2, doorL / 2);
        glm::vec3 max = center + glm::vec3(doorW / 2, doorH / 2, doorL / 2);

        // Vértices da caixa
        glm::vec3 v[8] = {
            {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
            {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
        };

		float cropX = 0.3f;
        float cropY = 0.2f;
        glm::vec2 uvBL(cropX, cropY); 
        glm::vec2 uvBR(1 - cropX, cropY);
        glm::vec2 uvTR(1 - cropX, 1 - cropY);
        glm::vec2 uvTL(cropX, 1 - cropY);
        glm::vec2 noUV(cropX, cropY);

        auto addFace = [&](int a, int b, int c, int d, bool isFront) {
            glm::vec2 u0, u1, u2, u3;

            if (isFront) {
                u0 = uvBR; u1 = uvBL; u2 = uvTL; u3 = uvTR;
            }
            else {
                u0 = u1 = u2 = u3 = noUV;
            }

            doorTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), u0, u1, u2, doorColor, doorColor, doorColor, 32.0f));
            doorTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), u0, u2, u3, doorColor, doorColor, doorColor, 32.0f));
            };

        addFace(0, 1, 2, 3, false);
        addFace(1, 5, 6, 2, true);  // FACE DA FRENTE
        addFace(5, 4, 7, 6, false);
        addFace(4, 0, 3, 7, false);
        addFace(3, 2, 6, 7, false);
        addFace(4, 5, 1, 0, false);

        auto doorMesh = std::make_unique<Mesh>(doorTris);
        doorMesh->loadTexture(RESOURCES_PATH "textures/porta.jpg");
        objects.push_back(std::move(doorMesh));
    }

    // CONTROLE REMOTO 
    Object* remoteControlPtr = nullptr;
    bool tvOn = true;

    {
        std::vector<Triangle> remoteTris;
        glm::vec3 white(0.0f);

        float rcX = 2.1f;
        float rcY = 0.6f;
        float rcZ = 8.9f;

        auto addRCBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };

            glm::vec2 uvSide(0.1f, 0.9f);

            float uMin = 0.05f;
            float uMax = 0.9f;
            float vMin = 0.30f;
            float vMax = 0.70f;

            glm::vec2 t0(vMax, uMin);
            glm::vec2 t1(vMax, uMax);
            glm::vec2 t2(vMin, uMax);
            glm::vec2 t3(vMin, uMin);

            auto addQ = [&](int a, int b, int c, int d, bool isTop) {
                glm::vec2 u0 = isTop ? t0 : uvSide;
                glm::vec2 u1 = isTop ? t1 : uvSide;
                glm::vec2 u2 = isTop ? t2 : uvSide;
                glm::vec2 u3 = isTop ? t3 : uvSide;

                remoteTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), u0, u1, u2, white, white, white, 10.0f));
                remoteTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), u0, u2, u3, white, white, white, 10.0f));
                };

            addQ(0, 1, 2, 3, false);
            addQ(1, 5, 6, 2, false);
            addQ(5, 4, 7, 6, false);
            addQ(4, 0, 3, 7, false);
            addQ(3, 2, 6, 7, true);
            addQ(4, 5, 1, 0, false);
            };

        addRCBox(glm::vec3(rcX, rcY, rcZ), glm::vec3(rcX + 0.2f, rcY + 0.018f, rcZ + 0.08f));

        auto remoteMesh = std::make_unique<Mesh>(remoteTris);

        remoteMesh->loadTexture(RESOURCES_PATH "textures/controle.jpg");

        remoteControlPtr = remoteMesh.get();
        objects.push_back(std::move(remoteMesh));
    }

    // SOFÁ
    {
        std::vector<Triangle> sofaTris;
        glm::vec3 whiteColor(1.0f);

        auto addBox = [&](glm::vec3 min, glm::vec3 max, float texRepeatX = 1.0f, float texRepeatZ = 1.0f) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };

            auto addTriangles = [&](int a, int b, int c, int d, glm::vec2 scale) {
                glm::vec2 uv0(0.0f, 0.0f);
                glm::vec2 uv1(scale.x, 0.0f);
                glm::vec2 uv2(scale.x, scale.y);
                glm::vec2 uv3(0.0f, scale.y);

                sofaTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z),
                    uv0, uv1, uv2, whiteColor, whiteColor, whiteColor, 10.0f));
                sofaTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z),
                    uv0, uv2, uv3, whiteColor, whiteColor, whiteColor, 10.0f));
                };

            addTriangles(0, 1, 2, 3, { 1.0f, 1.0f });
            addTriangles(1, 5, 6, 2, { texRepeatZ, 1.0f });
            addTriangles(5, 4, 7, 6, { 1.0f, 1.0f });
            addTriangles(4, 0, 3, 7, { texRepeatZ, 1.0f });
            addTriangles(3, 2, 6, 7, { 1.0f, texRepeatZ });
            addTriangles(4, 5, 1, 0, { 1.0f, texRepeatZ });
            };

        float ground = 0.0f;
        float sofaZStart = 7.0f;
        float sofaLength = 2.0f;
        float sofaWidth = 0.8f;

        float repeatZ = sofaLength;

        addBox(glm::vec3(1.5f, ground, sofaZStart), glm::vec3(1.5f + sofaWidth, ground + 0.4f, sofaZStart + sofaLength), 1.0f, repeatZ);

        addBox(glm::vec3(1.5f, ground + 0.4f, sofaZStart), glm::vec3(1.65f, ground + 0.9f, sofaZStart + sofaLength), 1.0f, repeatZ);

        addBox(glm::vec3(1.5f, ground + 0.4f, sofaZStart), glm::vec3(1.5f + sofaWidth, ground + 0.6f, sofaZStart + 0.15f), 1.0f, 1.0f);
        addBox(glm::vec3(1.5f, ground + 0.4f, sofaZStart + sofaLength - 0.15f), glm::vec3(1.5f + sofaWidth, ground + 0.6f, sofaZStart + sofaLength), 1.0f, 1.0f);

        auto sofaMesh = std::make_unique<Mesh>(sofaTris);
        sofaMesh->loadTexture(RESOURCES_PATH "textures/sofa.jpg");
        objects.push_back(std::move(sofaMesh));
    }

    // MESINHA DE CENTRO
    {
        std::vector<Triangle> tableTris;

        float tableGlassRadius = 0.35f;
        float tableGlassHeight = 0.05f;
        float tableHeight = 0.4f;
        float legRadius = 0.025f;

        float tableX = 3.0f;
        float tableZ = 8.0f;
        glm::vec3 center(tableX, tableHeight, tableZ);

        // Cores
        glm::vec3 glassColor(2.5f);
        glm::vec3 legColor(0.1f);

        int segments = 32;
        for (int i = 0; i < segments; ++i) {
            float angle1 = (float)i / segments * 2.0f * glm::pi<float>();
            float angle2 = (float)(i + 1) / segments * 2.0f * glm::pi<float>();

            // Vértices na borda
            glm::vec3 v1 = center + glm::vec3(tableGlassRadius * cos(angle1), 0.0f, tableGlassRadius * sin(angle1));
            glm::vec3 v2 = center + glm::vec3(tableGlassRadius * cos(angle2), 0.0f, tableGlassRadius * sin(angle2));

            // UVs circulares
            glm::vec2 uvCenter(0.5f, 0.5f);
            glm::vec2 uv1(0.5f + 0.5f * cos(angle1), 0.5f + 0.5f * sin(angle1));
            glm::vec2 uv2(0.5f + 0.5f * cos(angle2), 0.5f + 0.5f * sin(angle2));

            // Topo
            tableTris.push_back(Triangle(Point(center.x, center.y, center.z), Point(v1.x, v1.y, v1.z), Point(v2.x, v2.y, v2.z), uvCenter, uv1, uv2, glassColor, glassColor, glassColor, 100.0f));

            // Fundo
            tableTris.push_back(Triangle(Point(center.x, center.y - tableGlassHeight, center.z), Point(v2.x, v2.y - tableGlassHeight, v2.z), Point(v1.x, v1.y - tableGlassHeight, v1.z), uvCenter, uv2, uv1, glassColor, glassColor, glassColor, 100.0f));

            // Lateral
            glm::vec3 v1b = v1 - glm::vec3(0, tableGlassHeight, 0);
            glm::vec3 v2b = v2 - glm::vec3(0, tableGlassHeight, 0);
            float u1 = (float)i / segments; float u2 = (float)(i + 1) / segments;

            tableTris.push_back(Triangle(Point(v1.x, v1.y, v1.z), Point(v2.x, v2.y, v2.z), Point(v2b.x, v2b.y, v2b.z), glm::vec2(u1, 0), glm::vec2(u2, 0), glm::vec2(u2, 1), glassColor, glassColor, glassColor, 100.0f));
            tableTris.push_back(Triangle(Point(v1.x, v1.y, v1.z), Point(v2b.x, v2b.y, v2b.z), Point(v1b.x, v1b.y, v1b.z), glm::vec2(u1, 0), glm::vec2(u2, 1), glm::vec2(u1, 1), glassColor, glassColor, glassColor, 100.0f));
        }

        auto addTriangulatedCylinder = [&](glm::vec3 pos, float r, float h) {
            int legSegs = 12; 
            glm::vec2 noUV(0, 0);

            for (int i = 0; i < legSegs; i++) {
                float a1 = (float)i / legSegs * 2.0f * glm::pi<float>();
                float a2 = (float)(i + 1) / legSegs * 2.0f * glm::pi<float>();

                float x1 = r * cos(a1); float z1 = r * sin(a1);
                float x2 = r * cos(a2); float z2 = r * sin(a2);

                glm::vec3 b1 = pos + glm::vec3(x1, 0.0f, z1);
                glm::vec3 b2 = pos + glm::vec3(x2, 0.0f, z2);
                glm::vec3 t1 = pos + glm::vec3(x1, h, z1);
                glm::vec3 t2 = pos + glm::vec3(x2, h, z2);

                tableTris.push_back(Triangle(Point(b1.x, b1.y, b1.z), Point(b2.x, b2.y, b2.z), Point(t1.x, t1.y, t1.z), noUV, noUV, noUV, legColor, legColor, legColor, 32.0f));
                tableTris.push_back(Triangle(Point(b2.x, b2.y, b2.z), Point(t2.x, t2.y, t2.z), Point(t1.x, t1.y, t1.z), noUV, noUV, noUV, legColor, legColor, legColor, 32.0f));
            }
            };

        float legHeight = tableHeight - tableGlassHeight;
        float legOffset = tableGlassRadius * 0.65f;

        addTriangulatedCylinder(glm::vec3(tableX + legOffset, 0.0f, tableZ + legOffset), legRadius, legHeight);
        addTriangulatedCylinder(glm::vec3(tableX - legOffset, 0.0f, tableZ + legOffset), legRadius, legHeight);
        addTriangulatedCylinder(glm::vec3(tableX + legOffset, 0.0f, tableZ - legOffset), legRadius, legHeight);
        addTriangulatedCylinder(glm::vec3(tableX - legOffset, 0.0f, tableZ - legOffset), legRadius, legHeight);

        // --- CRIAÇÃO FINAL ---
        auto tableMesh = std::make_unique<Mesh>(tableTris);
        
        tableMesh->loadTexture(RESOURCES_PATH "textures/vidro.jpg");

        // Importante: Calcula a caixa que engloba vidro E pés
        tableMesh->updateAABB();

        objects.push_back(std::move(tableMesh));
    }

    // TV 
    Mesh* tvMeshPtr = nullptr;
    {
        std::vector<Triangle> tvTris;
        glm::vec3 frameColor(1.0f);
        glm::vec3 screenColor(1.0f);

        float wallX = 6.0f;
        float tvWidth = 2.2f;
        float tvHeight = 1.25f;
        float tvDepth = 0.04f;

        float centerZ = 8.0f;
        float centerY = 1.6f;

        auto addFrame = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };
            auto addQ = [&](int a, int b, int c, int d) {
                glm::vec2 uv(0, 0);
                tvTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv, uv, uv, frameColor, frameColor, frameColor, 10.0f));
                tvTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv, uv, uv, frameColor, frameColor, frameColor, 10.0f));
                };
            addQ(0, 1, 2, 3); addQ(1, 5, 6, 2); addQ(5, 4, 7, 6);
            addQ(4, 0, 3, 7); addQ(3, 2, 6, 7); addQ(4, 5, 1, 0);
            };

        addFrame(glm::vec3(wallX - tvDepth, centerY - tvHeight / 2, centerZ - tvWidth / 2),
            glm::vec3(wallX, centerY + tvHeight / 2, centerZ + tvWidth / 2));

        float screenX = wallX - tvDepth - 0.001f;
        float border = 0.03f;
        Point s1(screenX, centerY - tvHeight / 2 + border, centerZ + tvWidth / 2 - border);
        Point s2(screenX, centerY - tvHeight / 2 + border, centerZ - tvWidth / 2 + border);
        Point s3(screenX, centerY + tvHeight / 2 - border, centerZ - tvWidth / 2 + border);
        Point s4(screenX, centerY + tvHeight / 2 - border, centerZ + tvWidth / 2 - border);

        glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

        glm::vec3 selfLuminous(2.5f, 2.5f, 2.5f);
        tvTris.push_back(Triangle(s1, s2, s3, uv0, uv1, uv2, selfLuminous, screenColor, screenColor, 10.0f));
        tvTris.push_back(Triangle(s1, s3, s4, uv0, uv2, uv3, selfLuminous, screenColor, screenColor, 10.0f));

        auto tvMesh = std::make_unique<Mesh>(tvTris);
        tvMeshPtr = tvMesh.get();
        tvMesh->loadTexture(RESOURCES_PATH "textures/tv.jpg");
        objects.push_back(std::move(tvMesh));
    }

    // JANELA
    {
        std::vector<Triangle> windowTris;
        glm::vec3 frameColor(1.0f);
        glm::vec3 glassColor(1.0f);

        float wallZ = 14.0f;
        float winWidth = 2.0f;
        float winHeight = 1.2f;
        float winDepth = 0.05f;
        float centerX = 3.0f;
        float centerY = 1.7f;

        auto addFrame = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };
            auto addQ = [&](int a, int b, int c, int d) {
                glm::vec2 uv(0, 0);
                windowTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv, uv, uv, frameColor, frameColor, frameColor, 10.0f));
                windowTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv, uv, uv, frameColor, frameColor, frameColor, 10.0f));
                };
            addQ(0, 1, 2, 3); addQ(1, 5, 6, 2); addQ(5, 4, 7, 6);
            addQ(4, 0, 3, 7); addQ(3, 2, 6, 7); addQ(4, 5, 1, 0);
            };

        addFrame(glm::vec3(centerX - winWidth / 2, centerY - winHeight / 2, wallZ - winDepth),
            glm::vec3(centerX + winWidth / 2, centerY + winHeight / 2, wallZ));

        float glassZ = wallZ - winDepth - 0.001f;
        Point p1(centerX - winWidth / 2 + 0.05f, centerY - winHeight / 2 + 0.05f, glassZ);
        Point p2(centerX + winWidth / 2 - 0.05f, centerY - winHeight / 2 + 0.05f, glassZ);
        Point p3(centerX + winWidth / 2 - 0.05f, centerY + winHeight / 2 - 0.05f, glassZ);
        Point p4(centerX - winWidth / 2 + 0.05f, centerY + winHeight / 2 - 0.05f, glassZ);

        glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

        glm::vec3 dayLight(2.5f, 2.5f, 2.5f);
        windowTris.push_back(Triangle(p1, p2, p3, uv0, uv1, uv2, dayLight, glassColor, glassColor, 10.0f));
        windowTris.push_back(Triangle(p1, p3, p4, uv0, uv2, uv3, dayLight, glassColor, glassColor, 10.0f));

        auto windowMesh = std::make_unique<Mesh>(windowTris);
        windowMesh->loadTexture(RESOURCES_PATH "textures/janela.jpg");
        objects.push_back(std::move(windowMesh));
    }

    // --- ESTANTE DE LIVROS ---
    {
        std::vector<Triangle> shelfTris;
        glm::vec3 woodColor(1.0f);

        // Dimensões da Estante
        float shelfHeight = 2.4f;
        float shelfWidth = 1.2f;
        float shelfDepth = 0.35f;
        float thickness = 0.04f;

        float startX = 0.0f;
        float startZ = 14.0f;

        auto addBoard = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };
            glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

            auto addQ = [&](int a, int b, int c, int d) {
                shelfTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv0, uv1, uv2, woodColor, woodColor, woodColor, 32.0f));
                shelfTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv0, uv2, uv3, woodColor, woodColor, woodColor, 32.0f));
                };
            addQ(0, 1, 2, 3); addQ(1, 5, 6, 2); addQ(5, 4, 7, 6);
            addQ(4, 0, 3, 7); addQ(3, 2, 6, 7); addQ(4, 5, 1, 0);
            };

        // 1. ESTRUTURA (Construindo da esquerda X=0 para a direita X+)

        // Lateral Esquerda (Encostada na parede X=0)
        addBoard(glm::vec3(startX, 0.0f, startZ - shelfDepth),
            glm::vec3(startX + thickness, shelfHeight, startZ));

        // Lateral Direita
        addBoard(glm::vec3(startX + shelfWidth - thickness, 0.0f, startZ - shelfDepth),
            glm::vec3(startX + shelfWidth, shelfHeight, startZ));

        // Topo
        addBoard(glm::vec3(startX, shelfHeight - thickness, startZ - shelfDepth),
            glm::vec3(startX + shelfWidth, shelfHeight, startZ));

        // Fundo (Encostado na parede Z=14)
        addBoard(glm::vec3(startX, 0.0f, startZ - thickness),
            glm::vec3(startX + shelfWidth, shelfHeight, startZ));

        // Prateleiras internas
        int numShelves = 5;
        float spacing = (shelfHeight - thickness) / numShelves;
        for (int i = 0; i < numShelves; i++) {
            float y = i * spacing + 0.1f;
            addBoard(glm::vec3(startX + thickness, y, startZ - shelfDepth),
                glm::vec3(startX + shelfWidth - thickness, y + thickness, startZ - thickness));
        }

        auto m = std::make_unique<Mesh>(shelfTris);
        m->loadTexture(RESOURCES_PATH "textures/mesa.jpg");
        objects.push_back(std::move(m));

        // 2. LIVROS
        std::vector<Triangle> bookTris;
        glm::vec3 red(0.8f, 0.2f, 0.2f), blue(0.2f, 0.3f, 0.8f), green(0.2f, 0.6f, 0.2f);

        auto addBook = [&](glm::vec3 pos, float h, float w, float d, glm::vec3 color) {
            glm::vec3 min = pos;
            glm::vec3 max = pos + glm::vec3(w, h, d);
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };
            glm::vec2 zuv(0, 0);
            auto addQ = [&](int a, int b, int c, int d) {
                bookTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), zuv, zuv, zuv, color, color, color, 10.0f));
                bookTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), zuv, zuv, zuv, color, color, color, 10.0f));
                };
            addQ(0, 1, 2, 3); addQ(1, 5, 6, 2); addQ(5, 4, 7, 6); addQ(4, 0, 3, 7); addQ(3, 2, 6, 7); addQ(4, 5, 1, 0);
            };

        // Livros na Prateleira 2
        float shelfY = 1.0f + thickness; 
        float bZ = startZ - 0.25f;      

        addBook(glm::vec3(0.2f, shelfY, bZ), 0.25f, 0.05f, 0.2f, red);
        addBook(glm::vec3(0.26f, shelfY, bZ), 0.22f, 0.04f, 0.19f, blue);
        addBook(glm::vec3(0.31f, shelfY, bZ), 0.26f, 0.06f, 0.21f, green);

        // Livro deitado
        addBook(glm::vec3(0.5f, shelfY, bZ), 0.05f, 0.25f, 0.2f, red);

        // Livros na Prateleira 3
        shelfY = 1.5f + thickness;
        addBook(glm::vec3(0.7f, shelfY, bZ), 0.24f, 0.05f, 0.2f, blue);
        addBook(glm::vec3(0.76f, shelfY, bZ), 0.24f, 0.05f, 0.2f, blue);

        auto books = std::make_unique<Mesh>(bookTris);
        objects.push_back(std::move(books));
    }

    // TETO COM TEXTURA 
    {
        std::vector<Triangle> roofTris;
        float limitX = 3.0f;
        float limitZ = 7.0f;
        float h = 2.9f;

        Point p1(0.0f, h, 14.0f);
        Point p2(6.0f, h, 14.0f);
        Point p3(6.0f, h, 0.0f);
        Point p4(0.0f, h, 0.0f);

        float rfX = 2.0f;
        float rfZ = 4.0f;
        glm::vec2 uv1(0.0f, 0.0f);
        glm::vec2 uv2(rfX, 0.0f);
        glm::vec2 uv3(rfX, rfZ);
        glm::vec2 uv4(0.0f, rfZ);

        glm::vec3 white(1.0f);

        roofTris.push_back(Triangle(p1, p3, p2, uv1, uv3, uv2, white, white, white, 32.0f));
        roofTris.push_back(Triangle(p1, p4, p3, uv1, uv4, uv3, white, white, white, 32.0f));

        auto roofMesh = std::make_unique<Mesh>(roofTris);
        roofMesh->loadTexture(RESOURCES_PATH "textures/teto_2.jpg");
        objects.push_back(std::move(roofMesh));
    }

    // MESA 
    {
        std::vector<Triangle> tableTris;
        glm::vec3 tableColor(1.0f);

        float tableHeight = 0.8f;
        float tableThickness = 0.05f;
        float tableWidthZ = 2.2f;
        float tableDepthX = 1.2f;

        float wallRightX = 6.0f;
        float wallBackZ = 0.0f;

        auto addTableBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };
            glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

            auto addQ = [&](int a, int b, int c, int d) {
                tableTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv0, uv1, uv2, tableColor, tableColor, tableColor, 32.0f));
                tableTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv0, uv2, uv3, tableColor, tableColor, tableColor, 32.0f));
                };
            addQ(0, 1, 2, 3); addQ(1, 5, 6, 2); addQ(5, 4, 7, 6);
            addQ(4, 0, 3, 7); addQ(3, 2, 6, 7); addQ(4, 5, 1, 0);
            };

        addTableBox(glm::vec3(wallRightX - tableDepthX, tableHeight - tableThickness, wallBackZ),
            glm::vec3(wallRightX, tableHeight, wallBackZ + tableWidthZ));

        auto tableMesh = std::make_unique<Mesh>(tableTris);
        tableMesh->loadTexture(RESOURCES_PATH "textures/mesa.jpg");
        objects.push_back(std::move(tableMesh));

        glm::vec3 legColor(0.15f);
        float legRadius = 0.04f;
        float legHeight = tableHeight - 0.0f;

        float offset = 0.1f;
        std::vector<glm::vec3> legPos = {
            {wallRightX - offset, 0.0f, wallBackZ + offset},
            {wallRightX - tableDepthX + offset, 0.0f, wallBackZ + offset},
            {wallRightX - offset, 0.0f, wallBackZ + tableWidthZ - offset},
            {wallRightX - tableDepthX + offset, 0.0f, wallBackZ + tableWidthZ - offset}
        };

        for (const auto& lp : legPos) {
            objects.push_back(std::make_unique<Cilinder>(
                Point(lp.x, lp.y, lp.z, 1.0f), legRadius, legHeight, glm::vec4(0, 1, 0, 0),
                true, true, legColor, legColor, legColor, 10.0f, legColor, legColor, legColor, 10.0f
            ));
        }
    }

    // --- PC GAMER ---
    {
        std::vector<Triangle> pcTris;
        glm::vec3 caseColor(0.1f, 0.1f, 0.1f); // Preto
        glm::vec3 ledColor(0.0f, 0.8f, 1.0f);  // Azul Ciano Neon

        glm::vec3 start(5.6f, 0.8f, 0.4f);

        float w = 0.22f; // Largura
        float h = 0.48f; // Altura
        float d = 0.50f; // Profundidade

        // Função auxiliar local para criar caixas do PC
        auto addPcBox = [&](glm::vec3 min, glm::vec3 max, glm::vec3 col, bool emissive) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };
            glm::vec2 uv(0.1f, 0.1f); // UV fixo (cor solida da textura)

            // Se for emissivo (LED), o K_ambient é muito alto para brilhar no escuro
            glm::vec3 ka = emissive ? col * 5.0f : col;

            auto q = [&](int a, int b, int c, int d) {
                pcTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv, uv, uv, ka, col, col, 32.0f));
                pcTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv, uv, uv, ka, col, col, 32.0f));
                };
            q(0, 1, 2, 3); q(1, 5, 6, 2); q(5, 4, 7, 6); q(4, 0, 3, 7); q(3, 2, 6, 7); q(4, 5, 1, 0);
            };

        // 1. Corpo do Gabinete
        addPcBox(start, start + glm::vec3(w, h, d), caseColor, false);

        // 2. Painel Frontal (Um pouco mais à frente no Z)
        glm::vec3 painelMin = start + glm::vec3(0.0f, 0.0f, d);
        glm::vec3 painelMax = start + glm::vec3(w, h, d + 0.02f);
        addPcBox(painelMin, painelMax, glm::vec3(0.15f), false); // Cinza bem escuro

        // 3. Botão Power (LED) na frente
        addPcBox(start + glm::vec3(0.08f, h - 0.06f, d + 0.02f),
            start + glm::vec3(0.14f, h - 0.04f, d + 0.025f), ledColor, true);

        // 4. Faixa de LED lateral (decorativa)
        addPcBox(start + glm::vec3(-0.001f, 0.1f, 0.1f),
            start + glm::vec3(0.0f, h - 0.1f, 0.12f), ledColor, true);

        auto pcMesh = std::make_unique<Mesh>(pcTris);
        // Usamos a textura do controle pois ela é escura e rugosa (parece plástico/metal)
        pcMesh->loadTexture(RESOURCES_PATH "textures/controle.jpg");
        objects.push_back(std::move(pcMesh));
    }

    // --- MONITOR DO PC (Em cima da mesa de canto) ---
    Mesh* monitorMeshPtr = nullptr;
    bool monitorOn = true;
    {
        std::vector<Triangle> baseTris;
        std::vector<Triangle> screenTris;

        glm::vec3 plasticColor(0.1f); // Cor do plástico (sem textura)
        glm::vec3 screenColor(1.0f);  // Cor base da tela (branca para a textura)

        glm::vec3 basePos(5.6f, 0.8f, 1.2f);

        float monW = 0.02f;
        float monH = 0.35f;
        float monL = 0.60f;

        // Função auxiliar genérica para adicionar caixas à lista desejada
        auto addGenericBox = [&](glm::vec3 min, glm::vec3 max, glm::vec3 col, std::vector<Triangle>& targetTris) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };
            glm::vec2 uv(0, 0); // UV zerado, pois estas partes não terão textura
            auto q = [&](int a, int b, int c, int d) {
                targetTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv, uv, uv, col, col, col, 32.f));
                targetTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv, uv, uv, col, col, col, 32.f));
                };
            q(0, 1, 2, 3); q(1, 5, 6, 2); q(5, 4, 7, 6); q(4, 0, 3, 7); q(3, 2, 6, 7); q(4, 5, 1, 0);
            };

        // 1. Base e Haste (Vão para baseTris)
        addGenericBox(basePos + glm::vec3(-0.1f, 0.0f, -0.1f), basePos + glm::vec3(0.1f, 0.02f, 0.1f), plasticColor, baseTris);
        addGenericBox(basePos + glm::vec3(-0.02f, 0.02f, -0.02f), basePos + glm::vec3(0.02f, 0.15f, 0.02f), plasticColor, baseTris);

        // 2. Carcaça da Tela (Vai para baseTris)
        glm::vec3 screenCenter = basePos + glm::vec3(-0.05f, 0.30f, 0.0f);
        addGenericBox(screenCenter + glm::vec3(0, -monH / 2, -monL / 2), screenCenter + glm::vec3(monW, monH / 2, monL / 2), plasticColor, baseTris);

        // 3. Display (Apenas a face frontal vai para screenTris)
        float border = 0.015f;
        glm::vec3 min = screenCenter + glm::vec3(-0.001f, -monH / 2 + border, -monL / 2 + border);
        glm::vec3 max = screenCenter + glm::vec3(0.0f, monH / 2 - border, monL / 2 - border);

        // Vértices da tela (olhando para -X)
        Point p1(min.x, min.y, max.z); // Inferior Direito
        Point p2(min.x, max.y, max.z); // Superior Direito
        Point p3(min.x, max.y, min.z); // Superior Esquerdo
        Point p4(min.x, min.y, min.z); // Inferior Esquerdo

        // UVs para mapear a textura corretamente
        glm::vec2 uv0(1, 0), uv1(1, 1), uv2(0, 1), uv3(0, 0);

        // K_ambient alto para a tela brilhar quando ligada
        glm::vec3 ka_screen(1.5f);

        screenTris.push_back(Triangle(p1, p2, p3, uv0, uv1, uv2, ka_screen, screenColor, screenColor, 32.f));
        screenTris.push_back(Triangle(p1, p3, p4, uv0, uv2, uv3, ka_screen, screenColor, screenColor, 32.f));

        auto monitorBase = std::make_unique<Mesh>(baseTris);
        objects.push_back(std::move(monitorBase));

        auto monitorScreen = std::make_unique<Mesh>(screenTris);
        monitorScreen->loadTexture(RESOURCES_PATH "textures/monitor.jpg");

        monitorMeshPtr = monitorScreen.get();
        objects.push_back(std::move(monitorScreen));
    }

    // --- TECLADO (Em cima da mesa de canto) ---
    {
        std::vector<Triangle> keyboardTris;
        glm::vec3 kbColor(1.0f); // Cor base branca para multiplicar pela textura

        glm::vec3 kbStart(5.2f - 0.25f, 0.805f, 1.0f - 0.1f);

        float kW = 0.50f; // Largura do teclado
        float kH = 0.02f; // Altura/Espessura
        float kD = 0.22f; // Profundidade

        // Função auxiliar para criar a caixa do teclado com textura no topo
        auto addKbBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };

            float crop = 0.2f;
            glm::vec2 uv0(0, crop), uv1(1, crop), uv2(1, 1 - crop), uv3(0, 1 - crop);
            glm::vec2 uvSide(0.1f, 0.1f); 

            auto addQ = [&](int a, int b, int c, int d, bool isTop) {
                glm::vec2 u0 = isTop ? uv0 : uvSide;
                glm::vec2 u1 = isTop ? uv1 : uvSide;
                glm::vec2 u2 = isTop ? uv2 : uvSide;
                glm::vec2 u3 = isTop ? uv3 : uvSide;

                keyboardTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), u0, u1, u2, kbColor, kbColor, kbColor, 10.0f));
                keyboardTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), u0, u2, u3, kbColor, kbColor, kbColor, 10.0f));
                };

            addQ(0, 1, 2, 3, false); // Frente
            addQ(1, 5, 6, 2, false); // Direita
            addQ(5, 4, 7, 6, false); // Trás
            addQ(4, 0, 3, 7, false); // Esquerda
            addQ(3, 2, 6, 7, true);  // TOPO (Teclas)
            addQ(4, 5, 1, 0, false); // Baixo
            };

        addKbBox(kbStart, kbStart + glm::vec3(kW, kH, kD));

        auto kbMesh = std::make_unique<Mesh>(keyboardTris);
        kbMesh->loadTexture(RESOURCES_PATH "textures/teclado.jpg");

        glm::vec3 pivot = kbStart + glm::vec3(kW / 2, 0, kD / 2);
        glm::mat4 R = Operations::rotateY(glm::radians(-90.0f)); 
        glm::mat4 T = Operations::translate(pivot);
        glm::mat4 invT = Operations::translate(-pivot);

        kbMesh->model = T * R * invT;
        kbMesh->invModel = glm::inverse(kbMesh->model);
        kbMesh->normalMatrix = glm::transpose(glm::inverse(glm::mat3(kbMesh->model)));

        objects.push_back(std::move(kbMesh));
    }

    // --- MOUSE ---
    {
        std::vector<Triangle> mouseTris;
        glm::vec3 mouseColor(0.1f); // Preto fosco

        glm::vec3 startPos(5.1f, 0.8f, 1.40f);

        // Dimensões do mouse
        float mW = 0.06f; // Largura
        float mH = 0.03f; // Altura
        float mL = 0.11f; // Comprimento

        float cropX = 0.4f;
        float cropY = 0.2f;
        glm::vec2 uv0(cropX, cropY);
        glm::vec2 uv1(1.0f - cropX, cropY);
        glm::vec2 uv2(1.0f - cropX, 1.0f - cropY);
        glm::vec2 uv3(cropX, 1.0f - cropY);
        glm::vec2 uvSide(0.1f, 0.1f);

        auto addBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };
            glm::vec2 uv(0, 0);

            auto addQ = [&](int a, int b, int c, int d, bool isTop) {
                glm::vec2 u0 = isTop ? uv0 : uvSide;
                glm::vec2 u1 = isTop ? uv1 : uvSide;
                glm::vec2 u2 = isTop ? uv2 : uvSide;
                glm::vec2 u3 = isTop ? uv3 : uvSide;

                mouseTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), u0, u1, u2, mouseColor, mouseColor, mouseColor, 32.0f));
                mouseTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), u0, u2, u3, mouseColor, mouseColor, mouseColor, 32.0f));
                };

            addQ(0, 1, 2, 3, false); // Frente
            addQ(1, 5, 6, 2, false); // Direita
            addQ(5, 4, 7, 6, false); // Trás
            addQ(4, 0, 3, 7, false); // Esquerda
            addQ(3, 2, 6, 7, true);  // TOPO
            addQ(4, 5, 1, 0, false); // Baixo

			};

        addBox(startPos, startPos + glm::vec3(mW, mH, mL));

        auto mouseMesh = std::make_unique<Mesh>(mouseTris);
        mouseMesh->loadTexture(RESOURCES_PATH "textures/mouse.jpg");

        glm::vec3 pivot = startPos + glm::vec3(mW / 2, 0, mL / 2);
        glm::mat4 R = Operations::rotateY(glm::radians(-90.0f));
        glm::mat4 T = Operations::translate(pivot);
        glm::mat4 invT = Operations::translate(-pivot);

        mouseMesh->model = T * R * invT;
        mouseMesh->invModel = glm::inverse(mouseMesh->model);
        mouseMesh->normalMatrix = glm::transpose(glm::inverse(glm::mat3(mouseMesh->model)));

        objects.push_back(std::move(mouseMesh));
    }

    // --- CADEIRA GAMER (Frente à mesa do PC) ---
    {
        std::vector<Triangle> chairTris;
        glm::vec3 chairColor(1.0f);

        glm::vec3 chairPos(4.3f, 0.0f, 1.1f);

        // Dimensões
        float seatH = 0.50f; // Altura do assento do chão
        float seatW = 0.50f; // Largura do assento
        float seatD = 0.50f; // Profundidade do assento
        float backH = 0.80f; // Altura do encosto (a partir do assento)
        float backTh = 0.10f; // Espessura do encosto

        auto addBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };

            glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

            auto q = [&](int a, int b, int c, int d) {
                chairTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv0, uv1, uv2, chairColor, chairColor, chairColor, 10.0f));
                chairTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv0, uv2, uv3, chairColor, chairColor, chairColor, 10.0f));
                };
            q(0, 1, 2, 3); q(1, 5, 6, 2); q(5, 4, 7, 6); q(4, 0, 3, 7); q(3, 2, 6, 7); q(4, 5, 1, 0);
            };

        // 1. Pé Central (Cilindro simplificado como caixa)
        addBox(chairPos + glm::vec3(-0.05f, 0.0f, -0.05f), chairPos + glm::vec3(0.05f, seatH - 0.05f, 0.05f));

        // 2. Base com Rodinhas (Caixa achatada no chão)
        addBox(chairPos + glm::vec3(-0.25f, 0.0f, -0.25f), chairPos + glm::vec3(0.25f, 0.05f, 0.25f));

        // 3. Assento
        glm::vec3 sMin = chairPos + glm::vec3(-seatW / 2, seatH - 0.05f, -seatD / 2);
        glm::vec3 sMax = chairPos + glm::vec3(seatW / 2, seatH, seatD / 2);
        addBox(sMin, sMax);

        // 4. Encosto (Atrás do assento, ou seja, maior Z na posição original)
        // Nota: O encosto fica na borda "traseira" (Z positivo local)
        glm::vec3 bMin = chairPos + glm::vec3(-seatW / 2, seatH, seatD / 2 - backTh);
        glm::vec3 bMax = chairPos + glm::vec3(seatW / 2, seatH + backH, seatD / 2);
        addBox(bMin, bMax);

        auto chairMesh = std::make_unique<Mesh>(chairTris);
        chairMesh->loadTexture(RESOURCES_PATH "textures/cadeira2.jpg");

        glm::vec3 pivot = chairPos; // Gira no próprio eixo central
        glm::mat4 R = Operations::rotateY(glm::radians(-90.0f));
        glm::mat4 T = Operations::translate(pivot);
        glm::mat4 invT = Operations::translate(-pivot);

        chairMesh->model = T * R * invT;
        chairMesh->invModel = glm::inverse(chairMesh->model);
        chairMesh->normalMatrix = glm::transpose(glm::inverse(glm::mat3(chairMesh->model)));

        objects.push_back(std::move(chairMesh));
    }

    // --- PAREDE DIVISÓRIA (Com vão de porta) ---
    {
        std::vector<Triangle> divWallTris;
        glm::vec3 wallColor(1.0f);

        float divZ = 5.0f;        // Posição Z da divisória
        float doorXStart = 2.0f;  // Começo do vão da porta
        float doorXEnd = 3.0f;    // Fim do vão da porta
        float doorHeight = 2.1f;  // Altura do vão
        float wallThick = 0.15f;  // Espessura da parede

        // Função auxiliar para criar blocos de parede
        auto addWallBlock = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };
            glm::vec2 uv(0, 0);
            // UVs ajustados para a textura da parede não esticar demais
            float repX = (max.x - min.x) / 2.0f;
            float repY = (max.y - min.y) / 2.0f;
            glm::vec2 u0(0, 0), u1(repX, 0), u2(repX, repY), u3(0, repY);

            auto q = [&](int a, int b, int c, int d) {
                divWallTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), u0, u1, u2, wallColor, wallColor, wallColor, 1.0f));
                divWallTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), u0, u2, u3, wallColor, wallColor, wallColor, 1.0f));
                };
            q(0, 1, 2, 3); q(1, 5, 6, 2); q(5, 4, 7, 6); q(4, 0, 3, 7); q(3, 2, 6, 7); q(4, 5, 1, 0);
            };

        // 1. Parede Esquerda (Do canto X=0 até a porta)
        addWallBlock(glm::vec3(0.0f, 0.0f, divZ), glm::vec3(doorXStart, 3.0f, divZ + wallThick));

        // 2. Parede Direita (Da porta até o canto X=6)
        addWallBlock(glm::vec3(doorXEnd, 0.0f, divZ), glm::vec3(6.0f, 3.0f, divZ + wallThick));

        // 3. Viga Superior (Acima da porta)
        addWallBlock(glm::vec3(doorXStart, doorHeight, divZ), glm::vec3(doorXEnd, 3.0f, divZ + wallThick));

        auto divWallMesh = std::make_unique<Mesh>(divWallTris);
        divWallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(divWallMesh));
    }

    // --- PORTA ENTREABERTA (No vão da divisória) ---
    {
        std::vector<Triangle> doorTris;
        glm::vec3 dColor(1.0f);

        float dW = 0.95f; // Largura da folha da porta
        float dH = 2.05f; // Altura
        float dT = 0.04f; // Espessura

        // Criamos a porta na origem para facilitar a rotação pelo pivô (dobradiça)
        // A dobradiça fica em X=0 local
        glm::vec3 min(0.0f, 0.0f, 0.0f);
        glm::vec3 max(dW, dH, dT);

        glm::vec3 v[8] = { {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z} };

        // UVs simples para a porta
        float cropX = 0.3f;
        float cropY = 0.2f;
        glm::vec2 uv0(cropX, cropY), uv1(1 - cropX, cropY), uv2(1 - cropX, 1 - cropY), uv3(cropX, 1 - cropY);
        auto q = [&](int a, int b, int c, int d) {
            doorTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[b].x, v[b].y, v[b].z), Point(v[c].x, v[c].y, v[c].z), uv0, uv1, uv2, dColor, dColor, dColor, 10.0f));
            doorTris.push_back(Triangle(Point(v[a].x, v[a].y, v[a].z), Point(v[c].x, v[c].y, v[c].z), Point(v[d].x, v[d].y, v[d].z), uv0, uv2, uv3, dColor, dColor, dColor, 10.0f));
            };
        q(0, 1, 2, 3); q(1, 5, 6, 2); q(5, 4, 7, 6); q(4, 0, 3, 7); q(3, 2, 6, 7); q(4, 5, 1, 0);

        // Maçaneta
        float kX = dW - 0.1f; float kY = 1.0f; float kZ = dT;
        glm::vec3 km(kX, kY, kZ); glm::vec3 kM(kX + 0.05f, kY + 0.05f, kZ + 0.08f);
        // ... (código simplificado de adicionar caixa da maçaneta aqui se quiser)

        auto openDoorMesh = std::make_unique<Mesh>(doorTris);
        openDoorMesh->loadTexture(RESOURCES_PATH "textures/porta.jpg");

        glm::vec3 hingePos(2.05f, 0.0f, 5.0f + 0.05f); // Pequeno ajuste para centralizar no batente

        // Rotação: Abre 90 graus para dentro da sala do PC (-Z)
        glm::mat4 R = Operations::rotateY(glm::radians(-90.0f));
        glm::mat4 T = Operations::translate(hingePos);

        // Como criamos a porta com a dobradiça em (0,0,0), não precisamos de invT para o pivô
        openDoorMesh->model = T * R;
        openDoorMesh->invModel = glm::inverse(openDoorMesh->model);
        openDoorMesh->normalMatrix = glm::transpose(glm::inverse(glm::mat3(openDoorMesh->model)));

        objects.push_back(std::move(openDoorMesh));
    }

    // ILUMINAÇÃO
    std::vector<Light*> lights;

    // Luz da Árvore
    SpotLight* spotTree = new SpotLight(
        glm::vec3(0.8f, 0.9f, 0.8f),
        Point(1.0f, 2.6f, 3.0f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        50.0f
    );

    // Luz do Sofá 
    SpotLight* spotSofa = new SpotLight(
        glm::vec3(1.0f, 0.9f, 0.7f),
        Point(2.0f, 2.6f, 8.0f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        70.0f
    );

    // Luz da TV 
    SpotLight* spotTVLight = new SpotLight(
        glm::vec3(0.8f, 0.8f, 1.0f),
        Point(5.8f, 1.6f, 8.0f, 1.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        60.0f
    );

    // Luz da mesa do PC
    SpotLight* spotPCLight = new SpotLight(
        glm::vec3(0.8f, 0.8f, 1.0f),
        Point(5.4f, 2.0f, 1.1f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        40.0f
    );

    lights.push_back(spotTree);
    lights.push_back(spotSofa);
    lights.push_back(spotTVLight);
	lights.push_back(spotPCLight);

    PointLight* pcLight = new PointLight(
        glm::vec3(0.0f, 0.2f, 0.4f), 
        Point(5.7f, 1.2f, 1.0f, 1.0f)
    );

    lights.push_back(pcLight);

    std::vector<Sphere*> ledSpheres;

    // LEDs no Teto - Transformados
    float ledRadius = 0.08f;
    glm::vec3 ledOnColor(1.0f);

    std::vector<Point> spotPositions = {
        Point(1.0f, 2.8f, 3.0f, 1.0f),
        Point(2.0f, 2.8f, 8.0f, 1.0f),
        Point(5.4f, 2.8f, 1.1f, 1.0f)
    };

    for (const auto& pos : spotPositions) {
        auto led = std::make_unique<Sphere>(pos, ledRadius, ledOnColor, ledOnColor, ledOnColor, 10.0f);
        ledSpheres.push_back(led.get());
        objects.push_back(std::move(led));
    }

    glm::vec3 I_A(0.2f, 0.2f, 0.2f);

    std::vector<glm::vec3> framebuffer(MAX_WIDHT * MAX_HEIGHT);
    static std::vector<unsigned char> pixelBuffer(MAX_WIDHT * MAX_HEIGHT * 3);

    float vertices[] = {
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Atributo de Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atributo de Textura
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        float speed = 0.4f;
        if (moveForward)  camera.moveForward(speed);
        if (moveBackward) camera.moveForward(-speed);
        if (moveLeft)     camera.moveRight(-speed);
        if (moveRight)    camera.moveRight(speed);

        bool moving = moveForward || moveBackward || moveLeft || moveRight;
        cameraDirty |= moving;

        std::vector<ObjectCache> sceneCache;
        sceneCache.reserve(objects.size());
        for (auto& obj : objects) {
            ObjectCache cache;
            cache.ptr = obj.get();
            Mesh* meshPtr = dynamic_cast<Mesh*>(obj.get());
            if (meshPtr) {
                cache.box = meshPtr->getWorldAABB();
            }
            else {
                cache.box = obj->getAABB();
            }

            cache.isPlane = (dynamic_cast<Plane*>(cache.ptr) != nullptr);
            cache.isMesh = (meshPtr != nullptr);
            sceneCache.push_back(cache);
        }

        if (triggerPick) {
            Object* clicked = pickObject(camera, sceneCache);
            if (clicked == interruptorPtr) {
                lightOn = !lightOn;
                spotSofa->intensity = lightOn ? glm::vec3(1.0f) : glm::vec3(0.0f);
                spotTree->intensity = lightOn ? glm::vec3(1.0f) : glm::vec3(0.0f);
				spotPCLight->intensity = lightOn ? glm::vec3(0.6f, 0.6f, 0.8f) : glm::vec3(0.0f);

                glm::vec3 ledColor = lightOn ? glm::vec3(1.0f) : glm::vec3(0.05f);
                for (Sphere* led : ledSpheres) {
                    led->K_ambient = ledColor;
                    led->K_diffuse = ledColor;
                }

                cameraDirty = true;
            }

            if (clicked == remoteControlPtr) {
                tvOn = !tvOn;
                if (tvOn) {
                    tvMeshPtr->loadTexture(RESOURCES_PATH "textures/tv.jpg");
                    spotTVLight->intensity = glm::vec3(0.4f, 0.4f, 0.6f);
                }
                else {
                    tvMeshPtr->loadTexture(RESOURCES_PATH "textures/fundo_preto.jpg");
                    spotTVLight->intensity = glm::vec3(0.0f);
                }
                cameraDirty = true;
            }

            if (clicked == monitorMeshPtr) {
                monitorOn = !monitorOn;

                if (monitorOn) {
                    monitorMeshPtr->loadTexture(RESOURCES_PATH "textures/monitor.jpg");
                }
                else {
                    monitorMeshPtr->loadTexture(RESOURCES_PATH "textures/fundo_preto.jpg");
                }
                cameraDirty = true;
            }

            triggerPick = false;
        }

        if (cameraDirty) {
            cameraDirty = false;

            float wJanela = 0.6f;
            float hJanela = 0.6f;
            int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;

            std::fill(framebuffer.begin(), framebuffer.end(), I_A);

            std::vector<ObjectCache> sceneCache;
            sceneCache.reserve(objects.size());

            for (auto& obj : objects) {
                ObjectCache cache;
                cache.ptr = obj.get();
                Mesh* meshPtr = dynamic_cast<Mesh*>(obj.get());

                if (meshPtr) {
                    cache.box = meshPtr->getWorldAABB();
                }
                else {
                    cache.box = obj->getAABB();
                }

                cache.isPlane = (dynamic_cast<Plane*>(cache.ptr) != nullptr);
                cache.isMesh = (meshPtr != nullptr);

                if (cache.isPlane || isAheadOfCamera(cache.box, camera)) {
                    sceneCache.push_back(cache);
                }
            }

            int tileSize = 32;
            std::vector<Tile> tiles;
            for (int y = 0; y < MAX_HEIGHT; y += tileSize) {
                for (int x = 0; x < MAX_WIDHT; x += tileSize) {
                    Tile t = { x, y, std::min(tileSize, MAX_WIDHT - x), std::min(tileSize, MAX_HEIGHT - y) };
                    tiles.push_back(t);
                }
            }

            std::atomic<int> nextTileIndex(0);
            int numThreads = std::max(1, (int)std::thread::hardware_concurrency());
            std::vector<std::thread> workers;

            for (int i = 0; i < numThreads; ++i) {
                workers.emplace_back([&]() {
                    while (true) {
                        int index = nextTileIndex.fetch_add(1);
                        if (index >= tiles.size()) break;
                        renderTiles(tiles[index], nCol, nLin, wJanela, hJanela, camera, sceneCache, lights, I_A, framebuffer);
                    }
                    });
            }
            for (auto& w : workers) w.join();

            for (int i = 0; i < MAX_WIDHT * MAX_HEIGHT; i++) {
                pixelBuffer[i * 3 + 0] = (unsigned char)(glm::clamp(framebuffer[i].r, 0.0f, 1.0f) * 255.0f);
                pixelBuffer[i * 3 + 1] = (unsigned char)(glm::clamp(framebuffer[i].g, 0.0f, 1.0f) * 255.0f);
                pixelBuffer[i * 3 + 2] = (unsigned char)(glm::clamp(framebuffer[i].b, 0.0f, 1.0f) * 255.0f);
            }

            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_WIDHT, MAX_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());
        }

        glViewport(0, 0, 500, 500);
        glClear(GL_COLOR_BUFFER_BIT);
        s.bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);

        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - lastTime >= 1.0) {
            double msPerFrame = 1000.0 / double(nbFrames);

            char title[256];
            sprintf(title, "FPS: %d  (%.2f ms/frame)", nbFrames, msPerFrame);

            glfwSetWindowTitle(window, title);

            nbFrames = 0;
            lastTime += 1.0;
        }
    }

    //there is no need to call the clear function for the libraries since the os will do that for us.
    //by calling this functions we are just wasting time.
    //glfwDestroyWindow(window);
    //glfwTerminate();
    return 0;
}
