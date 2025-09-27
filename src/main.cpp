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
#include <iostream>


#define MAX_HEIGHT 500
#define MAX_WIDHT 500

#define USE_GPU_ENGINE 0
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
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


	GLFWwindow *window = window = glfwCreateWindow(MAX_HEIGHT, MAX_WIDHT, "Sphere", NULL, NULL);
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
		
		// sphere definition
		float rEsfera = 0.4f;
		Sphere sphere(Point(0.0f, 0.0f, -1.0f, 1.0f), rEsfera);
		sphere.K_ambient = glm::vec3(0.7f, 0.2f, 0.2f);
		sphere.K_diffuse = glm::vec3(0.7f, 0.2f, 0.2f);
		sphere.K_specular = glm::vec3(0.7f, 0.2f, 0.2f);
		sphere.shininess = 10.0f;

		// planes definitions
		Plane floor(Point(0.0f, -rEsfera, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
		floor.K_ambient = glm::vec3(0.2f, 0.7f, 0.2f);
		floor.K_diffuse = glm::vec3(0.2f, 0.7f, 0.2f);
		floor.K_specular = glm::vec3(0.0f, 0.0f, 0.0f);
		floor.shininess = 1.0f;

		Plane background(Point(0.0f, 0.0f, -2.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		background.K_ambient = glm::vec3(0.3f, 0.3f, 0.7f);
		background.K_diffuse = glm::vec3(0.3f, 0.3f, 0.7f);
		background.K_specular = glm::vec3(0.0f, 0.0f, 0.0f);
		background.shininess = 1.0f;

		LightSource light(glm::vec3(0.7f, 0.7f, 0.7f), Point(0.0f, 0.6f, -0.3f, 1.0f));
		glm::vec3 I_A = glm::vec3(0.3f, 0.3f, 0.3f);

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

				float t_min = FLT_MAX;
				int object_id = -1; // -1: none, 0: sphere, 1: floor, 2: background
				glm::vec3 color(0.0f, 0.0f, 0.0f);

				float t_sphere;
				float t_floor;
				float t_background;

				if (sphere.intersect(ray, t_sphere) && t_sphere < t_min) {
					t_min = t_sphere;
					object_id = 0;
				}

				if (floor.intersect(ray, t_floor) && t_floor < t_min) {
					t_min = t_floor;
					object_id = 1;
				}

				if (background.intersect(ray, t_background) && t_background < t_min) {
					t_min = t_background;
					object_id = 2;
				}

				if (object_id != -1) {
					glm::vec3 Pi = glm::vec3(eye.position) + t_min * glm::normalize(glm::vec3(ray.direction));

					glm::vec3 n;
					if (object_id == 0) {
						n = glm::normalize(Pi - glm::vec3(sphere.center.position));
						
						sphere.shade(Pi, n, ray, light, I_A, color);

					}
					else if (object_id == 1) {
						n = glm::normalize(glm::vec3(floor.normal_n));
						floor.shade(Pi, ray, light, I_A, sphere, color);

					}
					else if (object_id == 2) {
						n = glm::normalize(glm::vec3(background.normal_n));
						background.shade(Pi, ray, light, I_A, sphere, color);
					}
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
