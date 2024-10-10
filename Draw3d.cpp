#include <iostream>

#include "stanford_dragon.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

const int RESTART_INDEX = 2147483647;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// окно просмотра (координаты левого нижнего углаб ширина и высота окна рендеринга)
	glViewport(0, 0, width, height);
}

// закрытие по нажатию клавиши escape
void proccesInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void faces_to_primitive_restart(std::vector<int>& faces, const int primitive_value)
{
	size_t size = faces.size();
	int local_size;
	for (int i = 0; i < size; )
	{
		local_size = faces[i];
		faces[i] = primitive_value;
		i += local_size + 1;
	}
	faces.erase(faces.begin());
}

void draw_model(const Vertex* const p_vertices, const int num_vertices,
	std::vector<int> faces)
{
	//___________________________________________________________________________________________
	// Работа с GLFW
	//___________________________________________________________________________________________

	// Shaders
	const GLchar* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 position;\n"
		"layout (location = 1) in vec3 colors;\n"
		"out vec3 outColor;"
		"uniform mat4 transform;\n"
		"uniform mat4 projection;"
		"uniform mat4 view;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * transform * vec4(position.x/120.0f, position.y/120.0f, position.z/120.0f, 1.0);\n"
		"outColor = colors;"
		"}\0";
	const GLchar* fragmentShaderSource = "#version 330 core\n"
		"in vec3 outColor;\n"
		"out vec4 FragColor;"
		"void main()\n"
		"{\n"
		"FragColor = vec4(outColor, 1.0f);\n"
		"}\n\0";

	glfwInit();    // инициализация GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// создание объекта окна (width, height, window_name, ...)
	GLFWwindow* window = glfwCreateWindow(800, 600, "Dragon render", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	// Делаем контекст окна основным контекстом в текущем потоке
	glfwMakeContextCurrent(window);

	// привязка данной функции к GLFW
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// загрузка всех укахателей на OpenGL функции
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return;
	}

	// Build and compile our shader program
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Check for compile time errors
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Check for compile time errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Link shaders
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	faces_to_primitive_restart(faces, RESTART_INDEX);

	// создание буфера вершин
	unsigned int VBO, VAO, EBO; // Vertex Buffer Objects
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// Привязка буфера к целевому типу
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// копирование данные вершин в память буфера
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*num_vertices, p_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(faces[0]), &faces[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, red));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(shaderProgram);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 200.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -100.0f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(RESTART_INDEX);
	

	// цикл рендеринга
	while (!glfwWindowShouldClose(window))  // проверка закрытия приложения
	{
		proccesInput(window);

		// установка цвета, которым очищаем буфер (т.е. каждый пиксель будет такого цвета и прозрачности
		glClearColor(0.2f, 0.3f, 0.3f, 0.7f);

		glEnable(GL_DEPTH_TEST);

		// непосредственно очистка буфера цвета
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw our first triangle
		glUseProgram(shaderProgram);

		// Создаем преобразование
		glm::mat4 transform = glm::mat4(1.0f); // сначала инициализируем единичную матрицу
		transform = glm::rotate(transform, 20.0f*(float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));

		// Получаем location uniform-переменной матрицы и настраиваем её
		unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));


		glBindVertexArray(VAO);
		
		glDrawElements(GL_LINE_LOOP, faces.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		// меняет местами цветовой буфер (большой 2d-буфер, содержащий значения цвета для каждого пикселя в окне
		glfwSwapBuffers(window);

		// проверяет инициализируются ли какие-либо события (например ввод перемещение мыши)
		glfwPollEvents();
	}

	glDisable(GL_PRIMITIVE_RESTART);

	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
}

