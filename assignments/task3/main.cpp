//#include <glad/glad.h>
//#define GLFW_INCLUDE_NONE
//#include <GLFW/glfw3.h>
//#include <openglDebug.h>
//#include <demoShaderLoader.h>
//#include <Eigen/Dense>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include "model/Point.h"
//#include "model/Ray.h"
//#include "model/LightSource.h"
//#include "model/Objects/Sphere.h"
//#include "model/Objects/Plane.h"
//#include "operations/Shading.h"
//#include "model/Objects/Object.h"
//#include <iostream>
//
//
//#define MAX_HEIGHT 500
//#define MAX_WIDHT 500
//
//#define USE_GPU_ENGINE 0
//extern "C"
//{
//	__declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
//	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
//}
//
//
//static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//		glfwSetWindowShouldClose(window, GLFW_TRUE);
//}
//
//int main(void)
//{
//
//	if (!glfwInit())
//		return -1;
//
//
//#pragma region report opengl errors to std
//	//enable opengl debugging output.
//	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
//#pragma endregion
//
//
//	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
//	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //you might want to do this when testing the game for shipping
//
//
//	GLFWwindow* window = window = glfwCreateWindow(MAX_HEIGHT, MAX_WIDHT, "Sphere", NULL, NULL);
//	if (!window)
//	{
//		glfwTerminate();
//		return -1;
//	}
//
//	glfwSetKeyCallback(window, key_callback);
//
//	glfwMakeContextCurrent(window);
//	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//	glfwSwapInterval(1);
//
//
//#pragma region report opengl errors to std
//	glEnable(GL_DEBUG_OUTPUT);
//	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//	glDebugMessageCallback(glDebugOutput, 0);
//	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
//#pragma endregion
//
//	//shader loading example
//	Shader s;
//	s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");
//	s.bind();
//
//	GLint pixelColorLocation = glGetUniformLocation(s.id, "u_PixelColor");
//
//	while (!glfwWindowShouldClose(window))
//	{
//		float wJanela = 0.6f;
//		float hJanela = 0.6f;
//		float dJanela = 0.3f;
//		int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;
//
//		// scene definition
//		std::vector<std::unique_ptr<Object>> objects;
//		objects.push_back(std::make_unique<Sphere>(
//			Point(0.0f, 0.0f, -1.0f, 1.0f),
//			0.4f,
//			glm::vec3(0.7f, 0.2f, 0.2f),
//			glm::vec3(0.7f, 0.2f, 0.2f),
//			glm::vec3(0.7f, 0.2f, 0.2f),
//			10.0f));
//		objects.push_back(std::make_unique<Plane>(
//			Point(0.0f, -0.4f, 0.0f, 1.0f),
//			glm::vec4(0, 1, 0, 0),
//			glm::vec3(0.2f, 0.7f, 0.2f),
//			glm::vec3(0.2f, 0.7f, 0.2f),
//			glm::vec3(0.0f, 0.0f, 0.0f),
//			1.0f));
//		objects.push_back(std::make_unique<Plane>(
//			Point(0.0f, 0.0f, -2.0f, 1.0f),
//			glm::vec4(0, 0, 1, 0),
//			glm::vec3(0.3f, 0.3f, 0.7f),
//			glm::vec3(0.3f, 0.3f, 0.7f),
//			glm::vec3(0.0f, 0.0f, 0.0f),
//			1.0f));
//
//		LightSource light(glm::vec3(0.7f, 0.7f, 0.7f), Point(0.0f, 0.6f, -0.3f, 1.0f));
//		glm::vec3 I_A = glm::vec3(0.3f, 0.3f, 0.3f);
//
//		Point eye(0.0f, 0.0f, 0.0f, 1.0f);
//
//		float Dx = wJanela / (float)nCol;
//		float Dy = hJanela / (float)nLin;
//
//		glViewport(0, 0, 500, 500);
//		glClearColor(0.39f, 0.39f, 0.39f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);
//
//		s.bind();
//
//		float z = -dJanela;
//		for (int l = 0; l < nLin; l++) {
//			float y = hJanela / 2.0f - Dy / 2.0f - l * Dy;
//			for (int c = 0; c < nCol; c++) {
//				float x = -wJanela / 2 + Dx / 2.0f + c * Dx;
//				Ray ray(eye, glm::vec4(x, y, z, 0.0f));
//
//				glm::vec3 color(0.0f);
//				float t_min = FLT_MAX;
//				Object* hitObject = nullptr;
//
//				for (auto& obj : objects) {
//					float t;
//					if (obj->intersect(ray, t) && t < t_min) {
//						t_min = t;
//						hitObject = obj.get();
//					}
//				}
//
//				if (hitObject) {
//					glm::vec3 Pi = glm::vec3(eye.position) + t_min * glm::normalize(glm::vec3(ray.direction));
//					glm::vec3 n = hitObject->getNormal(Pi, glm::normalize(glm::vec3(ray.direction)));
//					color = shade(Pi, n, ray, light, I_A, *hitObject, objects);
//				}
//				else {
//					color = I_A;
//				}
//
//				glUniform3f(pixelColorLocation, color.r, color.g, color.b);
//				glBegin(GL_POINTS);
//
//				glVertex2f(x / (wJanela / 2.0f), y / (hJanela / 2.0f));
//				glEnd();
//			}
//		}
//
//		glfwSwapBuffers(window);
//		glfwPollEvents();
//	}
//
//	//there is no need to call the clear function for the libraries since the os will do that for us.
//	//by calling this functions we are just wasting time.
//	//glfwDestroyWindow(window);
//	//glfwTerminate();
//	return 0;
//}
