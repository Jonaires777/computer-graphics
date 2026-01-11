#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include <Eigen/Dense>
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

    Camera camera(
        Point(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::radians(60.0f),
        float(MAX_WIDHT) / float(MAX_HEIGHT),
        0.3f
    );

    glfwSetWindowUserPointer(window, &camera);

    // SCENE DEFINITION
    std::vector<std::unique_ptr<Object>> objects;

    {
        std::vector<Triangle> floorTris;
        float size = 7.0f;
        float height = -1.5f;

        glm::vec3 v0(-size, height, size);
        glm::vec3 v1(size, height, size);
        glm::vec3 v2(size, height, -size);
        glm::vec3 v3(-size, height, -size);

        // Coordenadas UV: O repeatFactor define quantas vezes a madeira se repete
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

    // 1. Parede Direita (X = 5.0)
    {
        std::vector<Triangle> wallTris;
        float wallX = 3.0f; 
        float zSize = 10.0f;

        Point p1(wallX, -wallHeight, zSize), p2(wallX, -wallHeight, -zSize);
        Point p3(wallX, wallHeight, -zSize), p4(wallX, wallHeight, zSize);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // 2. Parede Esquerda (X = -5.0)
    {
        std::vector<Triangle> wallTris;
        float wallX = -3.0f;
        float zSize = 10.0f;

        Point p1(wallX, -wallHeight, -zSize), p2(wallX, -wallHeight, zSize);
        Point p3(wallX, wallHeight, zSize), p4(wallX, wallHeight, -zSize);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // 3. Parede de Fundo (Z = -10.0)
    {
        std::vector<Triangle> wallTris;
        float posZ = -7.0f;
        float limitX = 3.0f;

        Point p1(-limitX, -wallHeight, posZ), p2(limitX, -wallHeight, posZ);
        Point p3(limitX, wallHeight, posZ), p4(-limitX, wallHeight, posZ);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p2, p3, uv1, uv2, uv3, white, white, white, 32.0f));
        wallTris.push_back(Triangle(p1, p3, p4, uv1, uv3, uv4, white, white, white, 32.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    // 4. Parede de Trás (Z = 10.0)
    {
        std::vector<Triangle> wallTris;
        float posZ = 7.0f;
        float limitX = 3.0f;

        Point p1(-limitX, -wallHeight, posZ), p2(limitX, -wallHeight, posZ);
        Point p3(limitX, wallHeight, posZ), p4(-limitX, wallHeight, posZ);

        float rf = 3.0f;
        glm::vec2 uv1(0, 0), uv2(rf, 0), uv3(rf, rf), uv4(0, rf);

        wallTris.push_back(Triangle(p1, p3, p2, uv1, uv3, uv2, white, white, white, 1.0f));
        wallTris.push_back(Triangle(p1, p4, p3, uv1, uv4, uv3, white, white, white, 1.0f));

        auto wallMesh = std::make_unique<Mesh>(wallTris);
        wallMesh->loadTexture(RESOURCES_PATH "textures/parede.jpg");
        objects.push_back(std::move(wallMesh));
    }

    {
        std::vector<Triangle> roofTris;
        float limitX = 3.0f;
        float limitZ = 7.0f;
        float h = 1.5f;

        Point p1(-limitX, h, limitZ), p2(limitX, h, limitZ);
        Point p3(limitX, h, -limitZ), p4(-limitX, h, -limitZ);

        glm::vec2 uv(0, 0);
        glm::vec3 roofColor(0.933f);

        roofTris.push_back(Triangle(p1, p3, p2, uv, uv, uv, roofColor, roofColor, roofColor, 1.0f));
        roofTris.push_back(Triangle(p1, p4, p3, uv, uv, uv, roofColor, roofColor, roofColor, 1.0f));

        objects.push_back(std::make_unique<Mesh>(roofTris));
    }

    // --- ÁRVORE (TRONCO) ---
    objects.push_back(std::make_unique<Cilinder>(
        Point(-2.0f, -1.5f, -4.0f, 1.0f), 
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

    // --- ÁRVORE (FOLHAS/CONE)
    objects.push_back(std::make_unique<Cone>(
        Point(-2.0f, -0.6f, -4.0f, 1.0f), 
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

    // --- ESTRELA (ESFERA NO TOPO) ---
    objects.push_back(std::make_unique<Sphere>(
        Point(-2.0f, 0.95f, -4.0f, 1.0f),
        0.05f,
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        10.0f
    ));

    {
        float a = 0.4f;
        glm::vec3 center(-2.0f, -1.5f + a / 2.0f, -3.6f);
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

        objects.push_back(std::make_unique<Mesh>(cubeTris));
    }

    Mesh* giftMesh = static_cast<Mesh*>(objects.back().get());
    giftMesh->loadTexture(RESOURCES_PATH "textures/presente.jpg");

    Object* interruptorPtr = nullptr;
    {
        float sWidth = 0.15f;  
        float sHeight = 0.28f; 
        float sDepth = 0.02f; 

        glm::vec3 p(-2.98f, 0.0f, 2.0f);

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

    Object* remoteControlPtr = nullptr;
    bool tvOn = true;

    {
        std::vector<Triangle> remoteTris;
        glm::vec3 white(0.0f);

        float rcX = -0.9f;
        float rcY = -0.9f;
        float rcZ = 1.9f;

        auto addRCBox = [&](glm::vec3 min, glm::vec3 max) {
            glm::vec3 v[8] = {
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}
            };

            // Lateral: Usaremos um pixel neutro da textura (preto)
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

            addQ(0, 1, 2, 3, false); // Frente
            addQ(1, 5, 6, 2, false); // Direita
            addQ(5, 4, 7, 6, false); // Fundo
            addQ(4, 0, 3, 7, false); // Esquerda
            addQ(3, 2, 6, 7, true);  // TOPO corrigido
            addQ(4, 5, 1, 0, false); // Baixo
            };

        addRCBox(glm::vec3(rcX, rcY, rcZ), glm::vec3(rcX + 0.2f, rcY + 0.018f, rcZ + 0.08f));

        auto remoteMesh = std::make_unique<Mesh>(remoteTris);

        remoteMesh->loadTexture(RESOURCES_PATH "textures/controle.jpg");

        remoteControlPtr = remoteMesh.get();
        objects.push_back(std::move(remoteMesh));
    }

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

            addTriangles(0, 1, 2, 3, { 1.0f, 1.0f });        // Frente (X, Y)
            addTriangles(1, 5, 6, 2, { texRepeatZ, 1.0f }); // Direita (Z, Y)
            addTriangles(5, 4, 7, 6, { 1.0f, 1.0f });        // Fundo (X, Y)
            addTriangles(4, 0, 3, 7, { texRepeatZ, 1.0f }); // Esquerda (Z, Y)
            addTriangles(3, 2, 6, 7, { 1.0f, texRepeatZ });
            addTriangles(4, 5, 1, 0, { 1.0f, texRepeatZ }); // Baixo (X, Z)
            };

        float ground = -1.5f;
        float sofaZStart = 0.0f;
        float sofaLength = 2.0f;
        float sofaWidth = 0.8f;

        float repeatZ = sofaLength;

        // 1. Assento (Base)
        addBox(glm::vec3(-1.5f, ground, sofaZStart), glm::vec3(-1.5f + sofaWidth, ground + 0.4f, sofaZStart + sofaLength), 1.0f, repeatZ);

        // 2. Encosto
        addBox(glm::vec3(-1.5f, ground + 0.4f, sofaZStart), glm::vec3(-1.35f, ground + 0.9f, sofaZStart + sofaLength), 1.0f, repeatZ);

        // 3. Braços
        addBox(glm::vec3(-1.5f, ground + 0.4f, sofaZStart), glm::vec3(-1.5f + sofaWidth, ground + 0.6f, sofaZStart + 0.15f), 1.0f, 1.0f);
        addBox(glm::vec3(-1.5f, ground + 0.4f, sofaZStart + sofaLength - 0.15f), glm::vec3(-1.5f + sofaWidth, ground + 0.6f, sofaZStart + sofaLength), 1.0f, 1.0f);

        auto sofaMesh = std::make_unique<Mesh>(sofaTris);
        sofaMesh->loadTexture(RESOURCES_PATH "textures/sofa.jpg");
        objects.push_back(std::move(sofaMesh));
    }

    Mesh* tvMeshPtr = nullptr;
    {
        std::vector<Triangle> tvTris;
        glm::vec3 frameColor(1.0f);
        glm::vec3 screenColor(1.0f);

        float wallX = 3.0f;
        float tvWidth = 2.2f;  
        float tvHeight = 1.25f;
        float tvDepth = 0.04f; 

        float centerZ = 1.0f;  
        float centerY = 0.1f;

        // 1. MOLDURA
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

        // 2. TELA
        float screenX = wallX - tvDepth - 0.001f;
        float border = 0.03f;
        Point s1(screenX, centerY - tvHeight / 2 + border, centerZ + tvWidth / 2 - border);
        Point s2(screenX, centerY - tvHeight / 2 + border, centerZ - tvWidth / 2 + border);
        Point s3(screenX, centerY + tvHeight / 2 - border, centerZ - tvWidth / 2 + border);
        Point s4(screenX, centerY + tvHeight / 2 - border, centerZ + tvWidth / 2 - border);

        glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);


        glm::vec3 selfLuminous(1.0f, 1.0f, 1.0f);
        tvTris.push_back(Triangle(s1, s2, s3, uv0, uv1, uv2, selfLuminous, screenColor, screenColor, 10.0f));
        tvTris.push_back(Triangle(s1, s3, s4, uv0, uv2, uv3, selfLuminous, screenColor, screenColor, 10.0f));

        auto tvMesh = std::make_unique<Mesh>(tvTris);
        tvMeshPtr = tvMesh.get();
        tvMesh->loadTexture(RESOURCES_PATH "textures/tv.jpg");
        objects.push_back(std::move(tvMesh));
    }

    // --- DEFINIÇÃO DA JANELA
    {
        std::vector<Triangle> windowTris;
        glm::vec3 frameColor(1.0f);
        glm::vec3 glassColor(1.0f);

        // Parâmetros de posição
        float wallZ = 7.0f;
        float winWidth = 2.0f;  
        float winHeight = 1.2f;
        float winDepth = 0.05f; 
        float centerX = 0.0f;   
        float centerY = 0.2f;

        // 1. MOLDURA (Bloco recuado para dentro da sala a partir de Z=7.0)
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

        // Moldura encostada em Z=7.0 vindo para Z=6.95
        addFrame(glm::vec3(centerX - winWidth / 2, centerY - winHeight / 2, wallZ - winDepth),
            glm::vec3(centerX + winWidth / 2, centerY + winHeight / 2, wallZ));

        float glassZ = wallZ - winDepth - 0.001f;
        Point p1(centerX - winWidth / 2 + 0.05f, centerY - winHeight / 2 + 0.05f, glassZ);
        Point p2(centerX + winWidth / 2 - 0.05f, centerY - winHeight / 2 + 0.05f, glassZ);
        Point p3(centerX + winWidth / 2 - 0.05f, centerY + winHeight / 2 - 0.05f, glassZ);
        Point p4(centerX - winWidth / 2 + 0.05f, centerY + winHeight / 2 - 0.05f, glassZ);

        glm::vec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);

        glm::vec3 dayLight(1.0f);
        windowTris.push_back(Triangle(p1, p2, p3, uv0, uv1, uv2, dayLight, glassColor, glassColor, 10.0f));
        windowTris.push_back(Triangle(p1, p3, p4, uv0, uv2, uv3, dayLight, glassColor, glassColor, 10.0f));

        auto windowMesh = std::make_unique<Mesh>(windowTris);
        windowMesh->loadTexture(RESOURCES_PATH "textures/janela.jpg");
        objects.push_back(std::move(windowMesh));
    }

    // --- DEFINIÇÃO DO TETO COM TEXTURA ---
    {
        std::vector<Triangle> roofTris;
        float limitX = 3.0f;
        float limitZ = 7.0f;
        float h = 1.4f;

        // Vértices do teto (H é a altura)
        Point p1(-limitX, h, limitZ);
        Point p2(limitX, h, limitZ);
        Point p3(limitX, h, -limitZ);
        Point p4(-limitX, h, -limitZ);

        // Coordenadas UV com repetição (Tiling)
        // Como o teto é comprido (7 unidades), repetimos a textura 2x na largura e 4x no comprimento
        float rfX = 2.0f;
        float rfZ = 4.0f;
        glm::vec2 uv1(0.0f, 0.0f);
        glm::vec2 uv2(rfX, 0.0f);
        glm::vec2 uv3(rfX, rfZ);
        glm::vec2 uv4(0.0f, rfZ);

        glm::vec3 white(1.0f);

        // Triângulos configurados para olhar para BAIXO (ordem p1, p3, p2)
        roofTris.push_back(Triangle(p1, p3, p2, uv1, uv3, uv2, white, white, white, 32.0f));
        roofTris.push_back(Triangle(p1, p4, p3, uv1, uv4, uv3, white, white, white, 32.0f));

        auto roofMesh = std::make_unique<Mesh>(roofTris);
        roofMesh->loadTexture(RESOURCES_PATH "textures/teto_2.jpg");
        objects.push_back(std::move(roofMesh));
    }

    // --- DEFINIÇÃO DA MESA ---
    {
        std::vector<Triangle> tableTris;
        glm::vec3 tableColor(1.0f);

        // Dimensões aumentadas
        float tableHeight = -0.7f;
        float tableThickness = 0.05f;
        float tableWidthZ = 2.2f;  
        float tableDepthX = 1.2f;

        float wallRightX = 3.0f;
        float wallBackZ = -7.0f;

        // 1. TAMPO DA MESA
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

        // 2. PÉS DA MESA 
        glm::vec3 legColor(0.15f);
        float legRadius = 0.04f;
        float legHeight = tableHeight - (-1.5f);

        float offset = 0.1f;
        std::vector<glm::vec3> legPos = {
            {wallRightX - offset, -1.5f, wallBackZ + offset},               
            {wallRightX - tableDepthX + offset, -1.5f, wallBackZ + offset},    
            {wallRightX - offset, -1.5f, wallBackZ + tableWidthZ - offset},   
            {wallRightX - tableDepthX + offset, -1.5f, wallBackZ + tableWidthZ - offset} 
        };

        for (const auto& lp : legPos) {
            objects.push_back(std::make_unique<Cilinder>(
                Point(lp.x, lp.y, lp.z, 1.0f), legRadius, legHeight, glm::vec4(0, 1, 0, 0),
                true, true, legColor, legColor, legColor, 10.0f, legColor, legColor, legColor, 10.0f
            ));
        }
    }

    // --- CONFIGURAÇÃO DE ILUMINAÇÃO ---
    std::vector<Light*> lights;

    // 1. Luz do Canto (Árvore e Presente)
    SpotLight* spotTree = new SpotLight(
        glm::vec3(0.8f, 0.9f, 0.8f),     
        Point(-2.0f, 1.1f, -4.0f, 1.0f),   
        glm::vec3(0.0f, -1.0f, 0.0f),      
        50.0f                             
    );

    // 3. Luz do Sofá
    SpotLight* spotSofa = new SpotLight(
        glm::vec3(1.0f, 0.9f, 0.7f),      
        Point(-1.0f, 1.1f, 1.0f, 1.0f),    
        glm::vec3(0.0f, -1.0f, 0.0f),
        70.0f
    );

    // --- LUZ EMITIDA PELA TELA DA TV ---
    SpotLight* spotTVLight = new SpotLight(
        glm::vec3(0.8f, 0.8f, 1.0f),
        Point(2.8f, 0.1f, 1.0f, 1.0f), 
        glm::vec3(-1.0f, 0.0f, 0.0f),  
        60.0f                          
    );

    lights.push_back(spotTree);
    lights.push_back(spotSofa);
    lights.push_back(spotTVLight);

    PointLight* windowLight = new PointLight(
        glm::vec3(0.4f, 0.4f, 0.4f),
        Point(0.0f, 0.2f, 6.0f, 1.0f)
    );

    lights.push_back(windowLight);

    std::vector<Sphere*> ledSpheres;

    // --- LUMINÁRIAS DE LED NO TETO ---
    float ledRadius = 0.08f;
    glm::vec3 ledOnColor(1.0f); 

    std::vector<Point> spotPositions = {
        Point(-2.0f, 1.3f, -4.0f, 1.0f), 
        Point(-1.0f, 1.3f,  1.0f, 1.0f),
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

        float speed = 0.1f;
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
            cache.box = obj->getAABB();
            cache.isPlane = (dynamic_cast<Plane*>(cache.ptr) != nullptr);
            cache.isMesh = (dynamic_cast<Mesh*>(cache.ptr) != nullptr);
            sceneCache.push_back(cache);
        }

        if (triggerPick) {
            Object* clicked = pickObject(camera, sceneCache);
            if (clicked == interruptorPtr) {
                lightOn = !lightOn;
                spotSofa->intensity = lightOn ? glm::vec3(1.0f) : glm::vec3(0.0f);
                spotTree->intensity = lightOn ? glm::vec3(1.0f) : glm::vec3(0.0f);

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

            triggerPick = false;
        }

        if (cameraDirty) {
            cameraDirty = false;

            float wJanela = 0.6f;
            float hJanela = 0.6f;
            int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;

            std::fill(framebuffer.begin(), framebuffer.end(), I_A);

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
