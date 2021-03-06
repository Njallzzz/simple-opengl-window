#include "glad.c"
#include <GLFW/glfw3.h>
#include <windows.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

GLuint texture;

int StartOpenGL(GLFWwindow **ret_window) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 1280, 720);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	{
		float vertices[] = {
			// Position
			// Image 1 - Top Left
			0.0f,	1.0f,
			0.0f,	0.0f,
			-1.0f,	0.0f,

			0.0f,	1.0f,
			-1.0f,	0.0f,
			-1.0f,	1.0f,

			// Image 2 - Top Right
			1.0f,	1.0f,
			1.0f,	0.0f,
			0.0f,	0.0f,

			1.0f,	1.0f,
			0.0f,	0.0f,
			0.0f,	1.0f,

			// Image 3 - Bottom Left
			0.0f,	0.0f,
			0.0f,	-1.0f,
			-1.0f,	-1.0f,

			0.0f,	0.0f,
			-1.0f,	-1.0f,
			-1.0f,	0.0f,

			// Image 4 - Bottom Right
			1.0f,	0.0f,
			1.0f,	-1.0f,
			0.0f,	-1.0f,

			1.0f,	0.0f,
			0.0f,	-1.0f,
			0.0f,	0.0f,

			// Texture Coordinates
			// Image 1 - Top Left
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f,

			// Image 2 - Top Right
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,

			0.0f, 0.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			// Image 3 - Bottom Left
			1.0f, 1.0f,
			1.0f, 0.0f,
			0.0f, 0.0f,

			1.0f, 1.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,

			// Image 4 - Bottom Right
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,

			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(4 * 6 * 2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	const char *vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec2 aPos;\n"
		"layout (location = 1) in vec2 aTexCoord;\n"
		"out vec2 TexCoord;\n"
		"void main()\n"
		"{\n"
		"	TexCoord = aTexCoord;\n"
		"   gl_Position = vec4(aPos.xy, 0.0, 1.0);\n"
		"}\0";

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	const char *fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 TexCoord;\n"
		"uniform sampler2D image;\n"
		"void main()\n"
		"{\n"
		"	FragColor = vec4(texture(image, TexCoord).xyz, 1.0);\n"
		"}\0";

	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glUseProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	int texture_location = glGetUniformLocation(shaderProgram, "image");
	glUniform1i(texture_location, 0);

	*ret_window = window;
	return 0;
}

int main(int argc, char *argv[]) {
	GLFWwindow *window;
	int res = StartOpenGL(&window);
	if (res)
		return res;

	glGenTextures(1, &texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load image to show something
	int width, height, original_no_channels;
	int desired_no_channels = 3;
	unsigned char *img = stbi_load("example.jpeg", &width, &height, &original_no_channels, desired_no_channels);
	if (img == nullptr) {
		cout << "Error in loading the image" << endl;
		return 1;
	}

	bool first_screen = true;

	// Main render loop
	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (first_screen) {
			// Update Texture Image here by calling this function with different picture data in variable 'img'
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
			first_screen = false;
		}

		glDrawArrays(GL_TRIANGLES, 0, 24);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	stbi_image_free(img);

	glfwTerminate();
	return 0;
}

