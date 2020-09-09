#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sys/stat.h>

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

GLFWwindow *initGlfwAndImgui(int width, int height, const char *window_name, const char *font_path)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
	    exit(1);

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(width, height, window_name, NULL, NULL);
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

    // bigger font size
    std::ifstream font_file(font_path);
    if(font_file.good())
    {
        ImFont *font = io.Fonts->AddFontFromFileTTF(font_path, 18);
    }
    else
    {
        ImFontConfig config;
        config.SizePixels = 16;
        ImFont *font = io.Fonts->AddFontDefault(&config);
    }
    font_file.close();
    
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

int getButtonColor(Button button)
{
    const char *buffer = button.label.c_str();
    size_t seed = std::hash<std::string>{}(button.label);
    srand(seed);
    int r = (rand()%128) + 16;
    int g = (rand()%128) + 16;
    int b = (rand()%128) + 16;

    int result = 0xff000000;
    result |= r;
    result |= g << 8;
    result |= b << 16;
    return result;
}

#define CONFIG_FOLDER ".config/scriptpanel"
#define SCRIPTS_FILE "scripts/scriptpanel.yaml"
#define DEFAULT_FONT "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf"
struct Config
{
    std::string script_file;
    int num_buttons_per_row;
    int button_width;
    int button_height;
    std::string font_path;
};
    
Config loadOrCreateConfigIfMissing()
{
    struct stat st = {0};
    const char* homedir = getenv("HOME");
    std::string config_folder = std::string(homedir) + "/" + CONFIG_FOLDER;
    if(stat(config_folder.c_str(), &st) == -1)
    {
        bool success = mkdir(config_folder.c_str(),0700);
        if(success == -1)
            printf("Could not create the directory %s\n", config_folder.c_str());
        else
            printf("Created config folder at %s\n", config_folder.c_str());
    }

    std::ifstream ifs(config_folder+"/config.yaml");
    Config config = {std::string(homedir) + "/" + SCRIPTS_FILE, 5, 125, 60, DEFAULT_FONT};

    // Writeout default config if file does not exist
    if(!ifs.good())
    {
        std::ofstream ofs;
        ofs.open(config_folder+"/config.yaml");
        YAML::Emitter out(ofs);
        out << YAML::BeginMap;
        out << YAML::Key << "ScriptFile";
        out << YAML::Value << config.script_file;
        out << YAML::Key << "NumButtonsPerRow";
        out << YAML::Value << config.num_buttons_per_row;
        out << YAML::Key << "ButtonWidth";
        out << YAML::Value << config.button_width;
        out << YAML::Key << "ButtonHeight";
        out << YAML::Value << config.button_height;
        out << YAML::Key << "FontPath";
        out << YAML::Value << config.font_path;
        out << YAML::EndMap;
        ofs.close();
    }
    else
    {
        YAML::Node root = YAML::Load(ifs);
        if(root["ScriptFile"])
            config.script_file = root["ScriptFile"].as<std::string>();
        
        if(root["NumButtonsPerRow"])
            config.num_buttons_per_row = root["NumButtonsPerRow"].as<int>();
        
        if(root["ButtonWidth"])
            config.button_width = root["ButtonWidth"].as<int>();
        
        if(root["ButtonHeight"])
            config.button_height = root["ButtonHeight"].as<int>();

        if(root["FontPath"])
            config.font_path = root["FontPath"].as<std::string>();
    }
    ifs.close();
    return config;
}

int main(int, char**)
{
    Config cfg = loadOrCreateConfigIfMissing();
    std::vector<Button> buttons = ParseFile(cfg.script_file.c_str());
    ImVec2 button_size = {(float)cfg.button_width, (float)cfg.button_height};

    GLFWwindow *window = initGlfwAndImgui(800, 600, "Scriptpanel", cfg.font_path.c_str());
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();
    
    ImVec2 spacing = style.ItemSpacing;
    ImVec2 padding = style.WindowPadding;
    int width  = (button_size.x + spacing.x)*cfg.num_buttons_per_row + padding.x + 5;
    int height = (button_size.y + spacing.y)*ceil(buttons.size()/(float)cfg.num_buttons_per_row) + padding.y + 5;
    glfwSetWindowSize(window, width, height); 
    glfwSetWindowPos(window, 0, 0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGuiViewport *viewport = ImGui::GetWindowViewport();
            ImGuiID id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_AutoHideTabBar);

            // NOTE: remove tab bar
            ImGui::SetNextWindowDockID(id);
            ImGuiWindowClass wc;
            wc.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
            ImGui::SetNextWindowClass(&wc);

            if(ImGui::Begin("Controlpanel"))
            {
                float window_max_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                int button_id = 0;
                for(auto &button : buttons)
                {
                    ImGui::PushID(button_id);
                    ImGui::PushStyleColor(ImGuiCol_Button, getButtonColor(button));
                    if(ImGui::Button(button.label.c_str(), button_size))
                    {
                        glfwMakeContextCurrent(NULL);
                        pid_t pid = fork();
                        if(pid==0)
                        {
                            if(button.keep_open)
                            {
                                std::string extra = "; exec bash";
                                std::string cmd = button.path + extra;
                                execl("/usr/bin/gnome-terminal", "ControlpanelTerminal", "--" , "bash", "-c", cmd.c_str(), NULL);
                            }
                            {
                                execl(button.path.c_str(), button.path.c_str(), NULL);
                            }
                        }
                        glfwMakeContextCurrent(window);
                    }
                    if(ImGui::IsItemHovered())
                    {
                        if(button.tooltip != "")
                            ImGui::SetTooltip("%s", button.tooltip.c_str());
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
