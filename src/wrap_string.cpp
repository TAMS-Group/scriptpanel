// Note: I know that this is totally hacky and not the proper way
std::string wrappedString(std::string string, double width)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float button_width = width - style.FramePadding.x;
    
    std::string result = "";
    ImFont *font = ImGui::GetFont();
    const ImFontGlyph *glyph = font->FindGlyph((ImWchar)' ');
    double space_width = glyph->AdvanceX;

    std::vector<std::pair<std::string,double>> words;
    std::string word;
    float word_width = 0.0;
    for(auto &c : string)
    {
        const ImFontGlyph *glyph = font->FindGlyph((ImWchar) c);
        double glyph_width = glyph->AdvanceX;

        if(c == ' ')
        {
            words.push_back(make_pair(word, word_width));
            word_width = 0.0;
            word.clear();
        }
        else
        {
            word_width += glyph_width;
            word.push_back(c);
        }
    }
    if(word.size() > 0)
        words.push_back(make_pair(word, word_width));

    float total_line_width = 0.0;
    std::string line;
    bool first = true;
    for(auto &p : words)
    {
        std::string w = p.first;
        double word_width = p.second;

        if(first)
        {
            first = false;
            line += w;
            total_line_width = word_width;
        }
        else
        {
            double next_total_line_width = total_line_width + word_width + space_width;
            
            if(next_total_line_width > button_width)
            {
                if(button_width > total_line_width)
                {
                    double d_width = button_width - total_line_width;
                    int num_spaces = (int)((d_width/space_width)/2.0);
                    line = std::string(num_spaces, ' ') + line;
                }
                
                result += line + "\n";
                line.clear();
                
                total_line_width = word_width;
                line += w;
            }
            else
            {
                line += " " + w;
                total_line_width = next_total_line_width;
            }
        }
    }
    if(line.size() > 0)
    {
        if(button_width > total_line_width)
        {        
            double d_width = button_width - total_line_width;
            int num_spaces = (int)((d_width/space_width)/2.0);
            line = std::string(num_spaces, ' ') + line;
        }
        result += line + "\n";
    }

    return result;
}
