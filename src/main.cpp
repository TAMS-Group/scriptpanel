#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <unordered_set>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <fontconfig/fontconfig.h>
#include <yaml-cpp/yaml.h>

#include "wrap_string.cpp"

#define DEFAULT_FONT_PATTERN "LiberationMono:Bold"
struct Config
{
    int num_buttons_per_row;
    int button_width;
    int button_height;
    std::string font_path;
};

std::string getFontPath(std::string pattern)
{
    std::string result;

    FcInit();
    FcConfig  *config     = FcInitLoadConfigAndFonts();
    FcPattern *fc_pattern = FcNameParse((const FcChar8*)pattern.c_str());

    FcConfigSubstitute(config, fc_pattern, FcMatchPattern);
    FcDefaultSubstitute(fc_pattern);

    FcChar8* file = 0;
    FcResult match_result;
    FcPattern *font = FcFontMatch(config, fc_pattern, &match_result);
    if (font && (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch))
        result = std::string((char*)file);

    FcPatternDestroy(font);
    FcPatternDestroy(fc_pattern);
    FcConfigDestroy(config);
    FcFini();

    return result;
}

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

GLFWwindow *initGlfwAndImgui(int width, int height, const char *window_name, Config *cfg)
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

    int font_size = cfg->button_height/4;
    std::ifstream font_file(cfg->font_path);
    if(font_file.good())
    {
        ImFont *font = io.Fonts->AddFontFromFileTTF(cfg->font_path.c_str(), font_size);
    }
    else
    {
        ImFontConfig config;
        config.SizePixels = font_size;
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

struct ParseFileResult
{
    std::map<std::string,std::vector<Button>> groups;
    Config config;
    int num_buttons;
};

std::unordered_set<std::string> getFilenames(std::string script_folder)
{
    std::unordered_set<std::string> result;
    DIR *dir = opendir(script_folder.c_str());
    if(dir)
    {
        dirent *entry = readdir(dir);
        while(entry)
        {
            struct stat file_stat;
            std::string path = script_folder + "/" + entry->d_name;
            if(stat(path.c_str(), &file_stat)==0)
            {
                bool executable = file_stat.st_mode & S_IXUSR;
                std::string name = entry->d_name;
                int len = name.size();
                if((name[len-3] == '.' && name[len-2] == 's' && name[len-1] == 'h') ||
                    (name[len-3] == '.' && name[len-2] == 'p' && name[len-1] == 'y'))
                {
                    result.insert(name);
                }
            }
            entry = readdir(dir);
        }
    }
    else
    {
        printf("Could not open %s\n", script_folder.c_str());
    }
    return result;
}

std::string default_label = "NO_LABLE";
std::string default_path = "/bin/bash";
std::string default_tooltip = "";
ParseFileResult ParseFile(std::string script_folder, std::unordered_set<std::string> name_list)
{
    ParseFileResult result;
    Config config = {5, 125, 60, getFontPath(DEFAULT_FONT_PATTERN)};
    std::map<std::string,std::vector<Button>> groups;
    std::string filename = script_folder  + "/scriptpanel.yaml";
    int num_buttons = 0;
    try
    {
        YAML::Node root = YAML::LoadFile(filename);
        if(root["config"])
        {
            YAML::Node config_node = root["config"];

            if(config_node["num_buttons_per_row"])
                config.num_buttons_per_row = config_node["num_buttons_per_row"].as<int>();
            if(config_node["button_width"])
                config.button_width = config_node["button_width"].as<int>();
            if(config_node["button_height"])
                config.button_height = config_node["button_height"].as<int>();
            if(config_node["font"])
            {
                std::string font_pattern = config_node["font"].as<std::string>();
                config.font_path = getFontPath(font_pattern);
            }
        }

        if(root["scripts"])
        {
            YAML::Node scripts = root["scripts"];

            for(int i = 0; i < scripts.size(); i++)
            {
                std::string group = "default";
                Button button = {default_label, default_path,default_tooltip, false};
                if(scripts[i]["group"])
                    group = scripts[i]["group"].as<std::string>();
                if(scripts[i]["label"])
                    button.label = scripts[i]["label"].as<std::string>();
                if(scripts[i]["script"])
                {
                    std::string filename = scripts[i]["script"].as<std::string>();
                    button.path = script_folder + "/" + filename;
                    name_list.erase(filename);
                }
                if(scripts[i]["tooltip"])
                    button.tooltip = scripts[i]["tooltip"].as<std::string>();
                if(scripts[i]["terminal"])
                    button.keep_open = scripts[i]["terminal"].as<bool>();

                groups[group].push_back(button);
                num_buttons++;
            }
        }
    }
    catch(...)
    {
        printf("Could not open or read %s.\n", filename.c_str());
    }

    for(auto &f : name_list)
    {
        Button button = {f, script_folder + "/" + f, "", false};
        groups["default"].push_back(button);
        num_buttons++;
    }

    result.groups = groups;
    result.config = config;
    result.num_buttons = num_buttons;
    return result;
}

// TODO: variation in group
int getButtonColor(Button button, std::string group)
{
    size_t seed = 0;
    if(group == "default")
        seed = std::hash<std::string>{}(button.label);
    else
        seed = std::hash<std::string>{}(group);

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

void displayButton(Button button, int button_id, std::string group, ImVec2 button_size,
                   GLFWwindow *window, ImGuiStyle& style, int num_buttons, float window_max_x)
{
    ImGui::PushID(button_id);
    int color = getButtonColor(button, group);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color + 0x00202020);
    
    if(ImGui::Button(wrappedString(button.label, button_size.x).c_str(), button_size))
    {
        glfwMakeContextCurrent(NULL);
        pid_t pid = fork();
        if(pid==0)
        {
            if(button.keep_open)
            {
                system(("/usr/bin/gnome-terminal -- " + button.path).c_str());
            }
            else
            {
                system(button.path.c_str());
            }
            exit(1); // only reached if exec failed above
        }
        glfwMakeContextCurrent(window);
    }
    if(ImGui::IsItemHovered())
    {
        if(button.tooltip != "")
            ImGui::SetTooltip("%s", button.tooltip.c_str());
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    float current_button_max_x = ImGui::GetItemRectMax().x;
    float next_button_max_x    = current_button_max_x + style.ItemSpacing.x + 100;

    ++button_id;
    if (button_id < num_buttons && next_button_max_x < window_max_x)
        ImGui::SameLine();

    ImGui::PopID();
}

int main(int argc, char** argv)
{
    const char* homedir = getenv("HOME");
    std::string script_folder =  std::string(homedir) + "/scripts";
    if(argc == 2)
    {
        script_folder = argv[1];
    }
    else
    {
        if(argc != 1)
        printf("Unexpected number of arguments.\n");
    }

    std::unordered_set<std::string> name_list = getFilenames(script_folder);
    ParseFileResult pfr = ParseFile(script_folder, name_list);
    Config cfg = pfr.config;
    std::map<std::string,std::vector<Button>> groups = pfr.groups;

    ImVec2 button_size = {(float)cfg.button_width, (float)cfg.button_height};

    GLFWwindow *window = initGlfwAndImgui(800, 600, "Scriptpanel", &cfg);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.ButtonTextAlign = {style.FramePadding.x/button_size.x,0.5};

    ImVec2 spacing = style.ItemSpacing;
    ImVec2 padding = style.WindowPadding;
    int width  = (button_size.x + spacing.x)*cfg.num_buttons_per_row + padding.x + 5;
    int height = (button_size.y + spacing.y)*ceil(pfr.num_buttons/(float)cfg.num_buttons_per_row) + padding.y + 5;
    glfwSetWindowSize(window, width, height);

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

                std::vector<Button> default_group;
                for(auto &g : groups)
                {
                    if(g.first == "default")
                    {
                        default_group = g.second;
                        continue;
                    }
                    for(auto &button : g.second)
                    {
                        displayButton(button, button_id, g.first, button_size, window, style, pfr.num_buttons, window_max_x);
                    }
                }
                // diplay default group last
                for(auto &button : default_group)
                {
                    displayButton(button, button_id, "default", button_size, window, style, pfr.num_buttons, window_max_x);
                }
                ImGui::End();
            }

        }

        renderAndSwapBuffers(window, clear_color);
    }

    cleanupGlfwAndImgui(window);
    return 0;
}
