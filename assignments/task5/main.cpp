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
        // Não precisamos de 'extern' aqui se ela estiver no mesmo arquivo
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

                // 2. Interseção específica (evita dynamic_cast a cada pixel)
                if (item.isMesh) {
                    Mesh* meshPtr = static_cast<Mesh*>(item.ptr); // Cast seguro e imediato
                    HitRecord currentHit;
                    if (meshPtr->intersect(ray, currentHit) && currentHit.t < t_min) {
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
                    }
                }
            }

            if (hitObject) {
                glm::vec3 Pi_world = glm::vec3(ray.origin.position) + t_min * glm::normalize(glm::vec3(ray.direction));
                glm::vec3 viewDir = glm::normalize(glm::vec3(ray.direction));
                glm::vec3 Pi_local = glm::vec3(hitObject->invModel * glm::vec4(Pi_world, 1.0f));
                glm::vec3 n_world;

                // Verificação rápida usando o boolean do cache se for o hitObject
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

                // Importante: Passar o Cache para o shade se for otimizá-lo também
                color = shade(Pi_world, n_world, ray, lights, I_A, *hitObject, sceneCache);
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
        // Teste AABB para performance no picking
        if (!item.isPlane) {
            float tNear, tFar;
            if (!item.box.intersect(ray, tNear, tFar)) continue;
        }

        // Teste de interseção real
        if (item.isMesh) {
            Mesh* meshPtr = static_cast<Mesh*>(item.ptr);
            HitRecord hr;
            if (meshPtr->intersect(ray, hr)) {
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

    objects.push_back(std::make_unique<Plane>(
        Point(0.0f, -1.5f, 0.0f, 1.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        glm::vec3(0.55f, 0.27f, 0.07f),
        glm::vec3(0.55f, 0.27f, 0.07f),
        glm::vec3(0.55f, 0.27f, 0.07f),
        10.0f
    ));

    objects.push_back(std::make_unique<Plane>(
        Point(2.0f, -1.5f, 0.0f, 1.0f),
        glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        10.0f
    ));

    objects.push_back(std::make_unique<Plane>(
        Point(0.0f, -1.5f, -4.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        10.0f
    ));

    objects.push_back(std::make_unique<Plane>(
        Point(-2.0f, -1.5f, 0.0f, 1.0f),
        glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        glm::vec3(0.686f, 0.933f, 0.933f),
        10.0f
    ));

    objects.push_back(std::make_unique<Plane>(
        Point(0.0f, 1.5f, 0.0f, 1.0f),
        glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
        glm::vec3(0.933f),
        glm::vec3(0.933f),
        glm::vec3(0.933f),
        10.0f
    ));

    objects.push_back(std::make_unique<Cilinder>(
        Point(0.0f, -1.5f, -2.0f, 1.0f),
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

    objects.push_back(std::make_unique<Cone>(
        Point(0.0f, -0.6f, -2.0f, 1.0f),
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

    {
        float a = 0.4f;
        glm::vec3 center(0.0f, -1.5f + a / 2.0f, -1.65f);
        float h = a / 2.0f;

        std::vector<Triangle> cubeTris;

        glm::vec3 v[8] = {
            center + glm::vec3(-h, -h,  h),
            center + glm::vec3(h, -h,  h),
            center + glm::vec3(h,  h,  h),
            center + glm::vec3(-h,  h,  h),
            center + glm::vec3(-h, -h, -h),
            center + glm::vec3(h, -h, -h),
            center + glm::vec3(h,  h, -h),
            center + glm::vec3(-h,  h, -h)
        };

        glm::vec3 color(1.0f, 0.078f, 0.576f);

        auto addQuad = [&](int a, int b, int c, int d) {
            cubeTris.push_back(Triangle(
                Point(v[a].x, v[a].y, v[a].z),
                Point(v[b].x, v[b].y, v[b].z),
                Point(v[c].x, v[c].y, v[c].z),
                color, color, color, 32.0f
            ));
            cubeTris.push_back(Triangle(
                Point(v[a].x, v[a].y, v[a].z),
                Point(v[c].x, v[c].y, v[c].z),
                Point(v[d].x, v[d].y, v[d].z),
                color, color, color, 32.0f
            ));
            };

        addQuad(0, 1, 2, 3);
        addQuad(1, 5, 6, 2);
        addQuad(5, 4, 7, 6);
        addQuad(4, 0, 3, 7);
        addQuad(3, 2, 6, 7);
        addQuad(4, 5, 1, 0);

        objects.push_back(std::make_unique<Mesh>(cubeTris));
    }

    objects.push_back(std::make_unique<Sphere>(
        Point(0.0f, 0.95f, -2.0f, 1.0f),
        0.05f,
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        glm::vec3(0.854f, 0.647f, 0.125f),
        10.0f
    ));

    Object* interruptorPtr = nullptr;
    {
        float s = 0.08f; // Metade do tamanho do lado do cubo
        // Parede esquerda está em x = -2.0. 
        // Posicionamos em x = -1.92 para que o cubo "saia" da parede em direção ao centro.
        glm::vec3 p(-1.92f, 0.0f, 0.0f);

        std::vector<Triangle> swTris;

        // Vértices do cubo (8 pontos) desempacotados para o seu construtor Point
        glm::vec3 v[8] = {
            p + glm::vec3(-s, -s,  s), p + glm::vec3(s, -s,  s), p + glm::vec3(s,  s,  s), p + glm::vec3(-s,  s,  s),
            p + glm::vec3(-s, -s, -s), p + glm::vec3(s, -s, -s), p + glm::vec3(s,  s, -s), p + glm::vec3(-s,  s, -s)
        };

        glm::vec3 blackColor(0.0f, 0.0f, 0.0f); // Preto

        auto addQ = [&](int a, int b, int c, int d) {
            // Criando os triângulos respeitando Point(x, y, z, w)
            swTris.push_back(Triangle(
                Point(v[a].x, v[a].y, v[a].z, 1.0f),
                Point(v[b].x, v[b].y, v[b].z, 1.0f),
                Point(v[c].x, v[c].y, v[c].z, 1.0f),
                blackColor, blackColor, blackColor, 32.0f));
            swTris.push_back(Triangle(
                Point(v[a].x, v[a].y, v[a].z, 1.0f),
                Point(v[c].x, v[c].y, v[c].z, 1.0f),
                Point(v[d].x, v[d].y, v[d].z, 1.0f),
                blackColor, blackColor, blackColor, 32.0f));
            };

        // Construindo as 6 faces do cubo
        addQ(0, 1, 2, 3); // Frente
        addQ(1, 5, 6, 2); // Direita
        addQ(5, 4, 7, 6); // Fundo
        addQ(4, 0, 3, 7); // Esquerda
        addQ(3, 2, 6, 7); // Cima
        addQ(4, 5, 1, 0); // Baixo

        auto swMesh = std::make_unique<Mesh>(swTris);
        swMesh->model = glm::mat4(1.0f); // Identidade para o Bake
        interruptorPtr = swMesh.get();
        objects.push_back(std::move(swMesh));
    }

    SpotLight* spot = new SpotLight(
        glm::vec3(1.0f),
        Point(-1.0f, 1.4f, -0.2f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        60.0f
    );

	PointLight point(glm::vec3(0.7f, 0.7f, 0.7f), Point(-1.0f, 1.4f, -0.2f, 1.0f));

    std::vector<Light*> lights;
    //lights.push_back(&point);                                                                                         
    lights.push_back(spot);

    glm::vec3 I_A(0.3f, 0.3f, 0.3f);

    std::vector<glm::vec3> framebuffer(MAX_WIDHT* MAX_HEIGHT);
    static std::vector<unsigned char> pixelBuffer(MAX_WIDHT* MAX_HEIGHT * 3);

    float vertices[] = {
        // posições          // coords textura
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f,   // top right
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f    // top left 
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

        // --- 1. MOVIMENTAÇÃO ---
        float speed = 0.1f;
        if (moveForward)  camera.moveForward(speed);
        if (moveBackward) camera.moveForward(-speed);
        if (moveLeft)     camera.moveRight(-speed);
        if (moveRight)    camera.moveRight(speed);

        bool moving = moveForward || moveBackward || moveLeft || moveRight;
        cameraDirty |= moving;

        // --- 2. GERAÇÃO DO CACHE (Apenas se necessário ou uma vez por frame) ---
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

        // --- 3. PROCESSO DE PICKING (INTERRUPTOR) ---
        if (triggerPick) {
            Object* clicked = pickObject(camera, sceneCache);
            if (clicked == interruptorPtr) {
                lightOn = !lightOn;
                // Liga/Desliga a luz
                spot->intensity = lightOn ? glm::vec3(1.0f) : glm::vec3(0.0f);

                // FEEDBACK VISUAL: Muda a cor do interruptor
                // Se for Mesh, você pode acessar os triângulos e mudar a cor deles, 
                // ou simplificar mudando o K_ambient do objeto se sua classe suportar.

                cameraDirty = true; // Força o redesenho após a interação
            }
            triggerPick = false;
        }

        // --- 4. RENDERIZAÇÃO (Só acontece se algo mudou) ---
        if (cameraDirty) {
            cameraDirty = false;

            float wJanela = 0.6f;
            float hJanela = 0.6f;
            int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;

            // Limpa buffers
            std::fill(framebuffer.begin(), framebuffer.end(), I_A);

            // Configuração de Tiles e Threads
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

            // Converte framebuffer para pixelBuffer (UNSIGNED BYTE)
            for (int i = 0; i < MAX_WIDHT * MAX_HEIGHT; i++) {
                pixelBuffer[i * 3 + 0] = (unsigned char)(glm::clamp(framebuffer[i].r, 0.0f, 1.0f) * 255.0f);
                pixelBuffer[i * 3 + 1] = (unsigned char)(glm::clamp(framebuffer[i].g, 0.0f, 1.0f) * 255.0f);
                pixelBuffer[i * 3 + 2] = (unsigned char)(glm::clamp(framebuffer[i].b, 0.0f, 1.0f) * 255.0f);
            }

            // Atualiza a textura na GPU
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_WIDHT, MAX_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());
        }

        // --- 5. DESENHO DA TELA (Sempre acontece para manter o FPS estável e mostrar a mira) ---
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
