#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <fstream>

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
    std::string label;
    std::string path;
    std::string tooltip;
    bool keep_open;
};

std::string default_label = "NO_LABLE";
std::string default_path = "/bin/bash";
std::string default_tooltip = "";
std::vector<Button> ParseFile(const char *filename)
{
    std::vector<Button> result;
    YAML::Node root = YAML::LoadFile(filename);

    for(int i = 0; i < root.size(); i++)
    {
        Button button = {default_label, default_path,default_tooltip, false};
        if(root[i]["Label"])
            button.label = root[i]["Label"].as<std::string>();
        if(root[i]["Path"])
            button.path = root[i]["Path"].as<std::string>();
        if(root[i]["ToolTip"])
            button.tooltip = root[i]["ToolTip"].as<std::string>();
        if(root[i]["KeepOpen"])
            button.keep_open = root[i]["KeepOpen"].as<bool>();
        result.push_back(button);
    }
    
    return result;
}

// TODO: better random function
int getButtonColor(Button button)
{
    const char *buffer = button.label.c_str();
    int seed = buffer[0];
    if(buffer[0] && buffer[1] && buffer[2] && buffer[3])
        seed = *(int *)buffer;
    srand (seed);
    int r = (rand()%64) + 64;
    int g = (rand()%64) + 64;
    int b = (rand()%64) + 64;

    int result = 0xff000000;
    result |= r;
    result |= g << 8;
    result |= b << 16;
    return result;
}

#define CONFIG_FILE "/home/pr2admin/pr2_script_panel/config/script_panel.yaml"
int main(int, char**)
{
    std::vector<Button> buttons = ParseFile(CONFIG_FILE);
    
    GLFWwindow *window = initGlfwAndImgui();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
                    ImGui::PushStyleColor(ImGuiCol_Button, getButtonColor(button));
                    if(ImGui::Button(button.label.c_str(), {125,125}))
                    {
                        glfwMakeContextCurrent(NULL);
                        pid_t pid = fork();
                        if(pid==0)
                        {
                            if(button.keep_open)
                            {
                                std::string extra = "; exec bash";
                                std::string cmd = button.path + extra;
                                execl("/usr/bin/gnome-terminal", "ControlpanelTerminal", "--" , "bash", "-c", cmd.c_str(), 0);
                            }
                            {
                                //execl("/usr/bin/gnome-terminal", "ControlpanelTerminal", "--" , "bash", "-c", button.path.c_str(), 0);
                                execl(button.path.c_str(), 0);
                            }
                        }
                        glfwMakeContextCurrent(window);
                    }
                    if(ImGui::IsItemHovered())
                    {
                        if(button.tooltip != "")
                            ImGui::SetTooltip(button.tooltip.c_str());
                    }
                    
                    ImGui::PopStyleColor();
		    
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
