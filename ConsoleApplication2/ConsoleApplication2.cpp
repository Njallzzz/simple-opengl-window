#include "glad.c"
#include <GLFW/glfw3.h>

#include <windows.h>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <psapi.h>

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main(int argc, char *argv[]) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", NULL, NULL);
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

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	{
		float vertices[] = {
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f,  0.5f, 0.0f
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	const char *vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	const char *fragmentShaderSource = "#version 330 core\n"
		"	out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
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

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;

	int width = 1920, height = 1080, comps = 3;

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




	//wchar_t command[] = L"ffmpeg -re -i D:\\Recordings\\5seconds.mp4 -pix_fmt rgb24 -f null -";
	wchar_t command[] = L"ffmpeg -re -i D:\\Recordings\\aq40-speedrun-2.mkv -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\asd.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f null -";
	//char command[] = "ffmpeg -re -i C:\\Users\\Maxsea\\Pictures\\rainbow.mp4 -pix_fmt rgb24 -f rawvideo udp://127.0.0.1:1234";

	BOOL res = CreateProcess(NULL,	// No module name (use command line)
		command,			// Command line
		NULL,				// Process handle not inheritable
		NULL,				// Thread handle not inheritable
		FALSE,				// Set handle inheritance to FALSE
		0,	// Creation flags, CREATE_NO_WINDOW | CREATE_SUSPENDED
		NULL,			// Use parent's environment block
		NULL,			// Use parent's starting directory 
		&si,			// Pointer to STARTUPINFO structure
		&pi);			// Pointer to PROCESS_INFORMATION structure

	if (!res) {
		cout << "CreateProcess failed, error:" << GetLastError() << endl;
		system("pause");
		return res;
	}

	cout << "Spawned process, pid: " << pi.dwProcessId << endl;

	while (ffmpeg_base_ptr == nullptr) {
		ffmpeg_base_ptr = FindBasePtr(pi.hProcess);

		if (ffmpeg_base_ptr == nullptr)
			Sleep(1);
	}

	cout << "FFmpeg Base pointer: " << (void *)ffmpeg_base_ptr << endl;

	while (ffmpeg_framebuffer_ptr == nullptr) {
		ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x48B3030, 0x20, 0x8, 0x18, 0x8 });		// v1
		//ffmpeg_framebuffer_ptr = (char *)FindDMAAddy(pi.hProcess, ffmpeg_base_ptr, { 0x5207F40, 0x278, 0xB8, 0x8, 0x8, 0x8 });	// v2

		if (ffmpeg_framebuffer_ptr == nullptr)
			Sleep(1);
		else
			ffmpeg_framebuffer_ptr += 128;
	}

	if (ffmpeg_framebuffer_ptr == nullptr) {
		cout << "Unable to find picture frame in ffmpeg" << endl;
		goto close;
	}
	else
		cout << "FFmpeg frame pointer: " << (void *)ffmpeg_framebuffer_ptr << endl;

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

	res = ReadProcessMemory(pi.hProcess, ffmpeg_framebuffer_ptr, framebuffer, buffer_size, &read);
	if (res == 0 && GetLastError() != ERROR_PARTIAL_COPY) {
		cout << "Error from reading frame (ReadProcessMemory), return code: " << GetLastError() << endl;
	} else if(read > 0) {
		if (read != buffer_size)
			cout << "Warning, unable to read full frame (ReadProcessMemory), wanted: " << buffer_size << ", got: " << read << endl;

		stbi_write_bmp("test_image.bmp", width, height, comps, framebuffer);
	}

	if (res == 0)
		goto close;

	/*while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
		system("pause");
		DWORD res_val = SuspendThread(pi.hThread);
		cout << "Suspended thread" << endl;

		system("pause");
		res_val = ResumeThread(pi.hThread);
		cout << "Resuming thread" << endl;
	}*/

close:
	delete[] framebuffer;

	system("pause");
	TerminateProcess(pi.hProcess, 0);

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	cout << "Process ended, closing..." << endl;
	system("pause");
	return 0;
}

