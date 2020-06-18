#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

static void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void cleanupGlfwAndImgui(GLFWwindow* window)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow *initGlfwAndImgui()
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
	exit(1);

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Pr2 Scripts controlpanel", NULL, NULL);
    if (window == NULL)
	exit(1);
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        exit(1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;          
    io.IniFilename = 0;
    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    return window;
}

void renderAndSwapBuffers(GLFWwindow *window, ImVec4 clear_color)
{
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    
    glfwSwapBuffers(window);
}

struct Button
{
    char *label;
    char *path;
    bool keep_open;
};

bool isWhitespace(char c)
{
    return (c == ' ' || c == '\n' || c == '\t');
}

std::vector<Button> ParseFile(const char *filename)
{
    std::vector<Button> result;
    FILE *file = fopen(filename, "rb");
    if(file)
    {
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);
	char *buffer = (char *)malloc(size+1);
	fread(buffer, size, 1, file);
	buffer[size] = 0;
	fclose(file);

	char *cur = buffer;
	bool parsing_button = false;
	Button button = {};
	while(*cur)
	{
	    char c = *cur;
	    switch(c)
	    {
	    case ' ': case '\n': case '\t': {++cur;}break;
	    case '{':
	    {
		// TODO: error when open multiple curly braces
		++cur;
		parsing_button = true;
		button.label = 0;
		button.path = 0;
		button.keep_open = true;
	    }break;
	    case '}':
	    {
		++cur;
		parsing_button = false;
		result.push_back(button);
	    }break;
	    default:
	    {
		if(strncmp(cur, "Label", 5) == 0)
		{
		    cur += 5;
		    while(isWhitespace(*cur)){++cur;} // TODO: refactor
		    if(*cur != ':')
		    {
			printf("Expected ':' got '%c'. Skipping line.\n", *cur);
			while(*cur != '\n') {++cur;};
		    }
		    ++cur;
		    while(isWhitespace(*cur)){++cur;}
		    
		    char *start = cur;
		    while(*cur != '\n'){++cur;}
		    size_t length = cur - start;
		    button.label = (char *)malloc(length + 1);
		    memcpy(button.label, start, length);
		    button.label[length] = 0;
		}
		else if(strncmp(cur, "Path", 4) == 0)
		{
		    cur += 4;
		    while(isWhitespace(*cur)){++cur;} // TODO: refactor
		    if(*cur != ':')
		    {
			printf("Expected ':' got '%c'. Skipping line.\n", *cur);
			while(*cur != '\n') {++cur;};
		    }
		    ++cur;
		    while(isWhitespace(*cur)){++cur;}

		    //const char *extra = "; exec bash";
		    char *start = cur;
		    while(*cur != '\n'){++cur;}
		    size_t length = cur - start;
		    button.path = (char *)malloc(length); // + 13);
		    memcpy(button.path, start, length);
		    //memcpy(button.path+length, extra, 12);
		    button.path[length] = 0; // +12] = 0; // TODO: this is not needed
		}
		else if(strncmp(cur, "KeepOpen", 8) == 0)
		{
		    cur += 8;
		    while(isWhitespace(*cur)){++cur;} // TODO: refactor
		    if(*cur != ':')
		    {
			printf("Expected ':' got '%c'. Skipping line.\n", *cur);
			while(*cur != '\n') {++cur;};
		    }
		    ++cur;
		    while(isWhitespace(*cur)){++cur;}

		    if(strncmp(cur, "true", 4) == 0)
		    {
			cur+=4;
			button.keep_open = true;
		    }
		    else if(strncmp(cur, "false", 5) == 0)
		    {
			cur+=5;
			button.keep_open = false;
		    }
		    else
		    {
			char *start = cur;
			while(*cur != '\n') {++cur;};
			printf("Expected 'true' or 'false' got '%.*s'. Skipping line.\n", (int)(cur-start), start);		
		    }
		}
		else
		{
		    printf("unexpected char '%c'\n", c);
		    ++cur;
		}
	    }
	    }
	}	
	free(buffer);
    }
    return result;
}

void appenToFile(const char *filename, Button button)
{
    FILE *file = fopen(filename, "ab");
    if(file)
    {
	fputs("\n{\n", file);

	fputs("\tLabel: ", file);
	fputs(button.label, file);
	fputc('\n', file);
	fputs("\tPath: ", file);
	fputs(button.path, file);
	fputc('\n', file);
	if(!button.keep_open)
	    fputs("\tKeepOpen: false\n", file);
	
	fputs("}\n", file);
	fclose(file);
    }    
}

Button copyPersistentButton(Button button)
{
    Button result;

    const char *extra = "; exec bash";
    
    size_t label_len = strlen(button.label) + 1;
    result.label = (char *)malloc(label_len);
    memcpy(result.label, button.label, label_len);

    size_t path_len = strlen(button.path);
    result.path = (char *)malloc(path_len +1); // + 13);
    memcpy(result.path, button.path, path_len);
    result.path[path_len] = 0;
    //memcpy(result.path+path_len, extra, 13);		    

    result.keep_open = button.keep_open;
    return result;
}

#define CONFIG_FILE "control_panel.txt"
int main(int, char**)
{
    std::vector<Button> buttons = ParseFile(CONFIG_FILE);
    
    GLFWwindow *window = initGlfwAndImgui();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();

    // Note: buffers for the add button popup dialog
    char add_button_label[32] = "";
    char add_button_path[64] = "";
    bool add_button_keep_open = true;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

	{
	    bool open_add_button_popup = false;
	    if(ImGui::BeginMainMenuBar())
	    {
		if(ImGui::BeginMenu("Edit"))
		{
		    if(ImGui::MenuItem("Add Button"))
		    {
			open_add_button_popup = true;
		    }
		    ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	    }
	    if(open_add_button_popup)
		ImGui::OpenPopup("add_button");
	    if(ImGui::BeginPopup("add_button"))
	    {
		ImGui::InputText("Label", add_button_label, 32);
		ImGui::InputText("Path", add_button_path, 64);
		ImGui::Checkbox("KeepOpen", &add_button_keep_open);
		
		if(ImGui::Button("Add"))
		{
		    Button button = {add_button_label, add_button_path, add_button_keep_open};
		    appenToFile(CONFIG_FILE, button);
		    buttons.push_back(copyPersistentButton(button)); 
		    open_add_button_popup = false;
		}
		ImGui::EndPopup();
	    }
	}

	{
	    ImGuiViewport *viewport = ImGui::GetWindowViewport();
	    ImGuiID id = ImGui::DockSpaceOverViewport(viewport);
	    ImGui::SetNextWindowDockID(id);

	    if(ImGui::Begin("Controlpanel"))
	    {
		float window_max_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		int button_id = 0;
		for(auto &button : buttons)
		{
		    ImGui::PushID(button_id);
		    if(ImGui::Button(button.label, {100,100}))
		    {
			pid_t pid = fork();
			if(pid)
			{
			    if(button.keep_open)
			    {
				    const char *extra = "; exec bash";
    
				    size_t path_len = strlen(button.path);
				    char *cmd = (char *)malloc(path_len + 13);
				    memcpy(cmd, button.path, path_len);
				    memcpy(cmd+path_len, extra, 13);		    
				    execl("/usr/bin/gnome-terminal", "ControlpanelTerminal", "--" , "bash", "-c", cmd, 0);
				    free(cmd);
			    }
			    {
				execl("/usr/bin/gnome-terminal", "ControlpanelTerminal", "--" , "bash", "-c", button.path, 0);
			    }
			}
		    }
		    
		    float current_button_max_x = ImGui::GetItemRectMax().x;
		    float next_button_max_x    = current_button_max_x + style.ItemSpacing.x + 100;
		    
		    ++button_id;
		    if (button_id < buttons.size() && next_button_max_x < window_max_x)
			ImGui::SameLine();
		    
		    ImGui::PopID();
		}
		ImGui::End();
	    }

	}
	
	renderAndSwapBuffers(window, clear_color);
    }

    cleanupGlfwAndImgui(window);
    return 0;
}
