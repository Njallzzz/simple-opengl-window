#include "glad.c"
#include <GLFW/glfw3.h>

#include <windows.h>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <psapi.h>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;


void *
FindBasePtr(HANDLE hProcess) {
	DWORD cbNeeded, mNeeded = MAX_PATH;
	BOOL ret;
	wchar_t module[MAX_PATH];
	HMODULE hMods[1024];

	ret = QueryFullProcessImageNameW(hProcess, 0, module, &mNeeded);
	if (ret == 0)
		return nullptr;

	ret = EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded);
	if (ret == 0)
		return nullptr;

	for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
		TCHAR szModName[MAX_PATH];
		ret = GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR));
		if (ret) {
			if (lstrcmpi(module, szModName) == 0) {
				MODULEINFO mod;
				BOOL res = GetModuleInformation(hProcess, hMods[i], &mod, sizeof(mod));
				return (void *)mod.lpBaseOfDll;
			}
		}
	}

	return nullptr;
}

template<typename T>
inline int32_t
GetProcessValueByLocation(HANDLE hProcess, void *location, T *result) {
	size_t read;

	BOOL res = ReadProcessMemory(hProcess, location, result, sizeof(T), &read);
	if (res == 0)
		return -1;
	if (read != sizeof(T))
		return -1;

	return 0;
}

void *
FindDMAAddy(HANDLE hProcess, void *base, std::vector<uint64_t> offsets) {
	char *addr = (char *)base;

	for (size_t i = 0; i < offsets.size(); i++) {
		void *buffer;
		addr = addr + offsets[i];

		int res = GetProcessValueByLocation<void *>(hProcess, addr, &buffer);
		if (res)
			return nullptr;
		if (buffer == nullptr)
			return nullptr;

		addr = (char *)buffer;
	}

	return (void *)addr;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

unsigned int VBO;
int texture_location = -1;

float scale = 1.0f;
float positionx = 0.0f, positiony = 0.0f;

bool left_mouse = false;
bool right_mouse = false;
bool middle_mouse = false;

double lastx = 0.0, lasty = 0.0;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	float diffx = (lastx - xpos) / 640.0f;
	float diffy = (lasty - ypos) / 360.0f;

	if (left_mouse) {
		positionx -= diffx;
		positiony += diffy;
	}
	else if (right_mouse) {
		scale += diffx;
	}

	if (right_mouse | left_mouse) {
		float half = scale;
		float vertices[12];
		vertices[0] = positionx + half;		vertices[1] = positiony + half;
		vertices[2] = positionx + half;		vertices[3] = positiony - half;
		vertices[4] = positionx - half;		vertices[5] = positiony - half;

		vertices[6] = positionx + half;		vertices[7] = positiony + half;
		vertices[8] = positionx - half;		vertices[9] = positiony - half;
		vertices[10] = positionx - half;	vertices[11] = positiony + half;

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	}

	lastx = xpos;
	lasty = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		right_mouse = true;
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		right_mouse = false;

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		left_mouse = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		left_mouse = false;
}

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
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	{
		float vertices[] = {
			// Position
			1.0f,	1.0f,		
			1.0f,	-1.0f,		
			-1.0f,	-1.0f,		

			1.0f,	1.0f,		
			-1.0f,	-1.0f,		
			-1.0f,	1.0f,		

			// Texture Coordinates
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(6 * 2 * sizeof(float)));
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

	texture_location = glGetUniformLocation(shaderProgram, "image");
	glUniform1i(texture_location, 0);

	*ret_window = window;
	return 0;
}

int main(int argc, char *argv[]) {
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;
	int res, width = 1920, height = 1080, comps = 3;
	unsigned int texture;
	GLFWwindow *window;

	size_t buffer_size = width * height * comps;
	char *framebuffer = new char[buffer_size];
	void *ffmpeg_base_ptr = nullptr;
	char *ffmpeg_framebuffer_ptr = nullptr;
	size_t read = 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	wchar_t command[] = L"ffmpeg -i rtp://224.1.1.99:21216?localaddr=192.168.1.11 -pix_fmt rgb24 -f null -";
	//wchar_t command[] = L"ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\5min.mp4 -pix_fmt rgb24 -f null -";
	//wchar_t command[] = L"ffmpeg -re -i D:\\Recordings\\5seconds.mp4 -pix_fmt rgb24 -f null -";
	//wchar_t command[] = L"ffmpeg -re -i D:\\Recordings\\aq40-speedrun-2.mkv -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\asd.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f rawvideo udp://127.0.0.1:1234";

#ifdef TRUE_
	res = CreateProcess(NULL,	// No module name (use command line)
		command,			// Command line
		NULL,				// Process handle not inheritable
		NULL,				// Thread handle not inheritable
		FALSE,				// Set handle inheritance to FALSE
		CREATE_NO_WINDOW,	// Creation flags, CREATE_NO_WINDOW | CREATE_SUSPENDED
		NULL,			// Use parent's environment block
		NULL,			// Use parent's starting directory 
		&si,			// Pointer to STARTUPINFO structure
		&pi);			// Pointer to PROCESS_INFORMATION structure

	if (!res) {
		std::cout << "CreateProcess failed, error:" << GetLastError() << endl;
		std::system("pause");
		return res;
	}
#else
	pi.dwProcessId = 10964;
	if (pi.dwProcessId == 0) {
		cout << "Process ID: ";
		cin >> pi.dwProcessId;
	}
	pi.hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE,
		pi.dwProcessId);
	if (pi.hProcess == INVALID_HANDLE_VALUE) {
		std::cout << "OpenProcess failed, error:" << GetLastError() << endl;
		std::system("pause");
		return 1;
	}
#endif


	std::cout << "Spawned process, pid: " << pi.dwProcessId << endl;

	while (ffmpeg_base_ptr == nullptr) {
		ffmpeg_base_ptr = FindBasePtr(pi.hProcess);

		if (ffmpeg_base_ptr == nullptr)
			Sleep(1);
	}

	std::cout << "FFmpeg Base pointer: " << (void *)ffmpeg_base_ptr << endl;

	/*std::system("pause");
	DWORD res_val = SuspendThread(pi.hThread);
	std::cout << "Suspended thread" << endl;*/

	ffmpeg_framebuffer_ptr = (char *)0x2C0A5785000;
	ffmpeg_framebuffer_ptr += 128;
	bool is_fixed = ffmpeg_framebuffer_ptr != nullptr;

	while (true) {
		if (!is_fixed) {
			int64_t raw_num;
			std::cout << "Framebuffer Location: ";
			cin >> hex >> raw_num;
			ffmpeg_framebuffer_ptr = (char *)raw_num;

			std::cout << "framebuffer pointer set to:" << (void*)ffmpeg_framebuffer_ptr << endl;
			if (ffmpeg_framebuffer_ptr == nullptr)
				goto close;

			ffmpeg_framebuffer_ptr += 128;		//*/
		}

		/*Sleep(300);
		cout << "Looking for framebuffer..." << endl;
		while (ffmpeg_framebuffer_ptr == nullptr) {
			//ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x48B3030, 0x20, 0x8, 0x18, 0x8 });		// v1
			//ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x5207F40, 0x278, 0xB8, 0x8, 0x8, 0x8 });	// v2
			ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x48B3030, 0x20, 0x8, 0x18, 0x8, 0x8, 0x8 });	// v3

			if (ffmpeg_framebuffer_ptr == nullptr)
				Sleep(50);
			else
				ffmpeg_framebuffer_ptr += 128;
		}

		if (ffmpeg_framebuffer_ptr == nullptr) {
			cout << "Unable to find framebuffer in ffmpeg" << endl;
			goto close;
		}
		else
			cout << "FFmpeg frame pointer: " << (void *)ffmpeg_framebuffer_ptr << endl;	//*/

		/*res = GetProcessValueByLocation<int>(pi.hProcess, nullptr, &width);
		if (res) {
			cout << "Unable to get width of frame" << endl;
			goto close;
		}

		res = GetProcessValueByLocation<int>(pi.hProcess, nullptr, &height);
		if (res) {
			cout << "Unable to get height of frame" << endl;
			goto close;
		}*/

		/*
		else if (read > 0) {
			if (read != buffer_size)
				cout << "Warning, unable to read full frame (ReadProcessMemory), wanted: " << buffer_size << ", got: " << read << endl;

			stbi_write_bmp("test_image.bmp", width, height, comps, framebuffer);
		}*/

		res = StartOpenGL(&window);
		if (res)
			goto close;

		glGenTextures(1, &texture);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		double lasttime = glfwGetTime();
		bool firstbla = false;
		while (!glfwWindowShouldClose(window)) {
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			/*ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x48B3030, 0x20, 0x8, 0x18, 0x8, 0x8, 0x8 }) + 128;	// v3
			if (ffmpeg_framebuffer_ptr == nullptr)
				break;*/
			
			if (!firstbla) {
				res = ReadProcessMemory(pi.hProcess, ffmpeg_framebuffer_ptr, framebuffer, buffer_size, &read);
				if (res == 0 && GetLastError() != ERROR_PARTIAL_COPY)
					break;

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
				//firstbla = true;
			}

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glfwPollEvents();
			glfwSwapBuffers(window);

			while (glfwGetTime() < lasttime + 1.0 / 30.0)
				Sleep(1);

			lasttime += 1.0 / 30.0;
		}

		glfwTerminate();
		if (is_fixed)
			break;
	}	// */

	if (res == 0)
		goto close;

close:
	delete[] framebuffer;

	glfwTerminate();

	/*TerminateProcess(pi.hProcess, 0);

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);*/
	std::cout << "Process ended, closing..." << endl;
	//std::system("pause");
	return 0;
}

