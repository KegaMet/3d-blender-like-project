#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
//#include "glm/gtx/quaternion.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>

class Triangle
{
private:
	unsigned int VAO, VBO;

public:
	
	float color[4];
	float currentVertices[9];

	Triangle(float vertices[9], float colors[4])
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*9, vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		for(int i = 0; i<9; i++) currentVertices[i] = vertices[i];
		for(int i = 0; i<4; i++) color[i] = colors[i];

	}

	void draw(unsigned int shaderProgram, int renderType = 1)
	{
		glUseProgram(shaderProgram);
		
		if(renderType>=1)
		{
			int vertexColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
			glUniform4f(vertexColorLoc, color[0], color[1], color[2], color[3]);
		
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			if(renderType==2)
			{
				glUniform4f(vertexColorLoc, 0.1f, 0.1f, 0.1f, 1.0f);
				glDrawArrays(GL_LINE_LOOP, 0, 3);
			}
		} 
	}

	std::unique_ptr<Triangle> clone()
	{
		auto new_tri = std::make_unique<Triangle>(this->currentVertices, this->color);
		return new_tri;
	}
	
	~Triangle()
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
};

class Object
{
private:
	std::vector<std::unique_ptr<Triangle>> triangles;
public:
	Object(std::vector<std::unique_ptr<Triangle>> old_triangles)
	{
		triangles = std::move(old_triangles);
	}
	
	float position[3] = {0.00f};
	float rotation[3] = {0.00f};
	float scale = 1.00f;
	int renderType = 1;

	void draw(unsigned int shaderProgram)
	{
		glUseProgram(shaderProgram);
		glm::mat4 rotMatrix = glm::mat4_cast(glm::quat(glm::vec3(glm::radians(rotation[0]), glm::radians(rotation[1]), glm::radians(rotation[2]))));
		glm::mat4 model = glm::mat4(1.0f) * glm::translate(glm::mat4(1.0f), glm::vec3(position[0], position[1], position[2])) * rotMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
		int vertexModelLoc = glGetUniformLocation(shaderProgram, "model");
		glUniformMatrix4fv(vertexModelLoc, 1, GL_FALSE, glm::value_ptr(model));
		for(auto& tri : triangles) tri->draw(shaderProgram, renderType);
	}

	std::unique_ptr<Object> clone()
	{
		std::vector<std::unique_ptr<Triangle>> new_tri;
		for(auto& tri : this->triangles)
			new_tri.push_back(tri->clone());
		
		auto new_obj = std::make_unique<Object>(std::move(new_tri));
		for(int i = 0; i<3; i++)
		{
			new_obj->position[i] = this->position[i];
			new_obj->rotation[i] = this->rotation[i];
		}
		new_obj->scale = this->scale;
		new_obj->renderType = this->renderType;
		return new_obj;
	}
};

std::vector<std::unique_ptr<Triangle>> makeCube(float colors[4])
{
	std::vector<std::unique_ptr<Triangle>> cube_triangle;
	const float variables[8][3] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
	int indices[6 * 6] = 
	{
	    0, 1, 3, 3, 1, 2,
	    1, 5, 2, 2, 5, 6,
	    5, 4, 6, 6, 4, 7,
	    4, 0, 7, 7, 0, 3,
	    3, 2, 7, 7, 2, 6,
	    4, 5, 0, 0, 5, 1
	};
	for(int i = 0;i<36;i++)
	{
		if((i+3)%3==0){
			cube_triangle.push_back(std::make_unique<Triangle>((float[])
						{
						variables[indices[i]][0], variables[indices[i]][1], variables[indices[i]][2],
					       	variables[indices[i+1]][0], variables[indices[i+1]][1], variables[indices[i+1]][2],
					       	variables[indices[i+2]][0], variables[indices[i+2]][1], variables[indices[i+2]][2]
						}, colors));
		}
	}
	return cube_triangle;
}

unsigned int createShaderProgram(const char* fragmentShaderSource, const char* vertexShaderSource) {
    // 1. Компиляция Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Проверка на ошибки (важно для дебага!)
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 2. Компиляция Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 3. Связывание (Linking)
    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return ID;
}

int main()
{
	if(!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(glfwGetVideoMode(glfwGetPrimaryMonitor())->width, glfwGetVideoMode(glfwGetPrimaryMonitor())->height ,"3D Editor", NULL, NULL);
	if(!window)
	{
		std::cout << "Failed to initialize glfw window\n";
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window);
	
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD\n";
		return -1;
	}
	
	const char* vertexShaderSource = 
		"#version 330 core\n"
    		"layout (location = 0) in vec3 aPos;\n"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
    		"void main() {\n"
    		"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    		"}\0";

	const char* fragmentShaderSource = 
		"#version 330 core\n"
    		"out vec4 color;\n"
		"uniform vec3 lightColor;\n"
    		"uniform vec4 objectColor;\n"
		"void main() {\n"
    		"   color = objectColor * vec4(lightColor, 1.0f);\n"
    		"}\n\0";

	const char* lightShaderSource =
		"#version 330 core\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	color = vec4(1.0f);\n"
		"}\n\0";

	unsigned int shaderProgram = createShaderProgram(fragmentShaderSource, vertexShaderSource);
	unsigned int lightShaderProgram = createShaderProgram(lightShaderSource, vertexShaderSource);

	glEnable(GL_DEPTH_TEST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	float backColor[3] = {0.3f, 0.3f, 0.3f};
	
	std::vector<std::unique_ptr<Triangle>> triangles;
	float triangleColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	float verticesList[9] = {0.00f};
	
	std::vector<std::unique_ptr<Object>> objects;

	glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 5.0f);
	float cameraRot[2] = {-20.0f, -90.0f};
	float cameraSpeed = 0.70f;
	float cameraRotSpeed = 0.50f;
	float cameraFieldView = 70.0f;
	
	static float presetColors[4] = {0.5f, 0.5f, 0.5f, 1.0f};

	while(!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glfwPollEvents();
	
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		{
			//ImGui Scene Settings
			ImGui::Begin("Scene Settings");
			ImGui::ColorEdit3("BackgroundColor", backColor);
			ImGui::SliderFloat("Camera FOV", &cameraFieldView, 10.0f, 140.0f);
			ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.00f, 1.00f);
			ImGui::SliderFloat("Camera Rotation Speed", &cameraRotSpeed, 0.00f, 1.00f);
			ImGui::End();
			
			//ImGui Camera Settings
			ImGui::Begin("Camera Move");
			ImGui::DragFloat3("Move", glm::value_ptr(cameraPos), 0.1f); ImGui::SetItemTooltip("Left: X, Center: Y, Right: Z");
			ImGui::DragFloat("Pitch", &cameraRot[0], 0.1f, -85.0f, 85.0f);
			ImGui::DragFloat("Yaw", &cameraRot[1], 0.1f);
			ImGui::End();

			//ImGui Triangle Creator
			ImGui::Begin("Triangle Creator");
			const char* verticesText[] = {"Vertice 1", "Vertice 2", "Vertice 3"};
			static int selected_vertex = 0;
			if(ImGui::BeginListBox("Vertices"))
			{
				for(int i = 0; i < 3; i++)
				{
					const bool is_selected = (selected_vertex == i);
					if(ImGui::Selectable(verticesText[i], is_selected))
						selected_vertex = i;
					if(is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}
			ImGui::DragFloat("Vertex X", &verticesList[3*(selected_vertex+1)-3], 0.05f, -50.00f, 50.00f);
			ImGui::DragFloat("Vertex Y", &verticesList[3*(selected_vertex+1)-2], 0.05f, -50.00f, 50.00f);
			ImGui::DragFloat("Vertex Z", &verticesList[3*(selected_vertex+1)-1], 0.05f, -50.00f, 50.00f);
			ImGui::ColorEdit4("Triangle Color", triangleColor);
			if(ImGui::Button("Debug out vars"))
			{
				for(int i=0;i<9;i++) std::cout << "vertex " << i << ": " << verticesList[i] << std::endl;
				for(int i=0;i<4;i++) std::cout << "color " << i << ": " << triangleColor[i] << std::endl;
			}
			if(ImGui::Button("Bake Triangle"))
			{
				triangles.push_back(std::make_unique<Triangle>(verticesList, triangleColor));
				//for(int i=0;i<9;i++) verticesList[i] = 0.00f;
			}
			if(ImGui::Button("Push To Object"))
			{
				objects.push_back(std::make_unique<Object>(std::move(triangles)));
			}
			ImGui::End();
			
			//ImGui Object Editor
			ImGui::Begin("Object Editor");
			static int selected_object = -1;
			if(ImGui::BeginListBox("Objects"))
			{
				if(selected_object>=(int)objects.size()) selected_object = (int)objects.size()-1;
				if(objects.size()==0) { ImGui::Text("There is no objects");
				}else{
					for(int i = 0; i < (int)objects.size(); i++)
					{
						char label[32];
						sprintf(label, "Object %d", i+1);
						const bool is_selected = (selected_object == i);
						ImGui::SetNextItemAllowOverlap(); if(ImGui::Selectable(label, is_selected)) selected_object = i;
						sprintf(label, "Delete##%d", i);
					       	ImGui::SameLine(); ImGui::SetNextItemAllowOverlap(); if(ImGui::SmallButton(label)) objects.erase(objects.begin()+i);
						sprintf(label, "Copy##%d", i);
						ImGui::SameLine(); if(ImGui::SmallButton(label)) objects.push_back(objects[i]->clone());
						if(is_selected) ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			if(selected_object>=0&&selected_object<(int)objects.size())
			{
				ImGui::Text("Transform");
				ImGui::DragFloat3("Move", objects[selected_object]->position, 0.1f);
				ImGui::DragFloat3("Rotate", objects[selected_object]->rotation, 0.1f);
				ImGui::DragFloat("Scale", &objects[selected_object]->scale, 0.1f);
				ImGui::Text("Render Type");
				ImGui::RadioButton("##ObjectRenderType_1", &objects[selected_object]->renderType, 1); ImGui::SetItemTooltip("Normal render");
				ImGui::SameLine(); ImGui::RadioButton("##ObjectRenderType_2", &objects[selected_object]->renderType, 2); ImGui::SetItemTooltip("Normal with wireframe lines");
				//if(ImGui::Button("Delete")) objects.erase(objects.begin()+selected_object);
			}
			ImGui::End();
			
			//ImGui Presets
			ImGui::Begin("Presets");
			ImGui::ColorEdit4("Color", presetColors);
			if(ImGui::Button("Create Cube")) objects.push_back(std::make_unique<Object>(makeCube(presetColors)));
			ImGui::End();
		}
		ImGui::Render();
		

		//нормализация камеры
		glm::vec3 cameraNormalized = glm::normalize(glm::vec3(cos(glm::radians(cameraRot[1])) * cos(glm::radians(cameraRot[0])), sin(glm::radians(cameraRot[0])), sin(glm::radians(cameraRot[1])) * cos(glm::radians(cameraRot[0]))));
		
		//Camera Movement
		double cursorPos[2], cursorPrePos[2];
		glfwGetCursorPos(window, &cursorPos[0], &cursorPos[1]);
		
		if(ImGui::IsMouseDown(1)&&!ImGui::GetIO().WantCaptureMouse) 
		{
		ImGuiIO io = ImGui::GetIO();
			cameraRot[0] += (float)(cursorPrePos[1]-cursorPos[1]) * cameraRotSpeed;
			cameraRot[1] += (float)(cursorPos[0]-cursorPrePos[0]) * cameraRotSpeed;
		}
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraNormalized * glm::vec3(cameraSpeed/10.0f);
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraNormalized * glm::vec3(cameraSpeed/10.0f);
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos += glm::vec3(cameraNormalized.z * (cameraSpeed/10.0f), 0, -cameraNormalized.x * (cameraSpeed/10.0f));
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::vec3(-cameraNormalized.z * (cameraSpeed/10.0f), 0, cameraNormalized.x * (cameraSpeed/10.0f));
		
		glfwSetCursorPos(window, cursorPrePos[0], cursorPrePos[1]);
		for(int i = 0; i<2; i++) cursorPrePos[i] = cursorPos[i];
		
		if(cameraSpeed>1.00f) {cameraSpeed = 1.00f;} if(cameraSpeed<0.00f) {cameraSpeed = 0.00f;}
		if(cameraRotSpeed>1.00f) {cameraRotSpeed = 1.00f;} if(cameraSpeed<0.00f) {cameraSpeed = 0.00f;}
		if(cameraRot[0]>90.0f) {cameraRot[0] = 89.9f;} if(cameraRot[0]<-90.0f) {cameraRot[0] = -89.9f;}
		if(cameraFieldView>179.9f) {cameraFieldView = 179.9f;} if(cameraFieldView<10.0f) {cameraFieldView = 10.0f;}
		
		//код для введения камеры и перспективы в шейдер мб
		glm::mat4 camView = glm::lookAt(glm::make_vec3(cameraPos), glm::vec3(cameraPos[0], cameraPos[1], cameraPos[2])+cameraNormalized, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 projection = glm::perspective(glm::radians(cameraFieldView), (float)display_w/(float)display_h, 0.1f, 100.0f);
		int vertexViewLoc = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(vertexViewLoc, 1, GL_FALSE, glm::value_ptr(camView));
		int vertexProjLoc = glGetUniformLocation(shaderProgram, "projection");
		glUniformMatrix4fv(vertexProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

		int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
		glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

		glClearColor(backColor[0], backColor[1], backColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for(auto& obj : objects)
		{
			obj->draw(shaderProgram);
		}

		//код для рендера треугольничков
		for(auto& tri : triangles)
		{
			glUseProgram(shaderProgram);
			glm::mat4 model = glm::mat4(1.0f);
			int vertexModelLoc = glGetUniformLocation(shaderProgram, "model");
			glUniformMatrix4fv(vertexModelLoc, 1, GL_FALSE, glm::value_ptr(model));
			tri->draw(shaderProgram, 2);
		}

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
