#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "model/Point.h"
#include "model/Ray.h"
#include "model/LightSource.h"
#include "model/Objects/Sphere.h"
#include "model/Objects/Plane.h"
#include "model/Objects/Cilinder.h"
#include "model/Objects/Cone.h"
#include "model/Objects/Triangle.h"
#include "model/Objects/Mesh.h"
#include "operations/Shading.h"
#include "model/Objects/Object.h"
#include <iostream>


#define MAX_HEIGHT 500
#define MAX_WIDHT 500

#define USE_GPU_ENGINE 0
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(void)
{

	if (!glfwInit())
		return -1;


#pragma region report opengl errors to std
	//enable opengl debugging output.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion


	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //you might want to do this when testing the game for shipping


	GLFWwindow* window = window = glfwCreateWindow(MAX_HEIGHT, MAX_WIDHT, "TASK_5", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);


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

	GLint pixelColorLocation = glGetUniformLocation(s.id, "u_PixelColor");

	while (!glfwWindowShouldClose(window))
	{
		float wJanela = 0.6f;
		float hJanela = 0.6f;
		float dJanela = 0.3f; 
		int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;

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

        LightSource light(glm::vec3(0.7f, 0.7f, 0.7f), Point(-1.0f, 1.4f, -0.2f, 1.0f));
        glm::vec3 I_A(0.3f, 0.3f, 0.3f);


		Point eye(0.0f, 0.0f, 0.0f, 1.0f);

		float Dx = wJanela / (float)nCol;
		float Dy = hJanela / (float)nLin;

		glViewport(0, 0, 500, 500);
		glClearColor(0.39f, 0.39f, 0.39f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		s.bind();

		float z = -dJanela;
		for (int l = 0; l < nLin; l++) {
			float y = hJanela / 2.0f - Dy / 2.0f - l * Dy;
			for (int c = 0; c < nCol; c++) {
				float x = -wJanela / 2 + Dx / 2.0f + c * Dx;
				Ray ray(eye, glm::vec4(x, y, z, 0.0f));

				glm::vec3 color(0.0f);
				float t_min = FLT_MAX;
				Object* hitObject = nullptr;

				for (auto& obj : objects) {
					float t;
					if (obj->intersect(ray, t) && t < t_min) {
						t_min = t;
						hitObject = obj.get();
					}
				}

				if (hitObject) {
					glm::vec3 Pi = glm::vec3(eye.position) + t_min * glm::normalize(glm::vec3(ray.direction));
					glm::vec3 viewDir = glm::normalize(glm::vec3(ray.direction));
					glm::vec3 n = hitObject->getNormal(Pi, viewDir);
					color = shade(Pi, n, ray, light, I_A, *hitObject, objects);
				}
				else {
					color = I_A;
				}

				glUniform3f(pixelColorLocation, color.r, color.g, color.b);
				glBegin(GL_POINTS);

				glVertex2f(x / (wJanela / 2.0f), y / (hJanela / 2.0f));
				glEnd();
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//there is no need to call the clear function for the libraries since the os will do that for us.
	//by calling this functions we are just wasting time.
	//glfwDestroyWindow(window);
	//glfwTerminate();
	return 0;
}
