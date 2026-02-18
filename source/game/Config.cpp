/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/game/Config.cpp
 *****************************************************************************/

#include "Config.h"
#include <Windows.h>
#include <winnls.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace RadarConfig
{
    static const char* s_configFileName = "radar-trilogy-sa.ini";
    static std::string s_configPath;
    static std::map<std::string, std::string> s_values;

    static bool s_shapeCircle      = true;
    static bool s_showGangZones    = true;
    static bool s_modeMoreIcon     = false;
    static int  s_circleSize       = 265;
    static int  s_squareSizeX      = 265;
    static int  s_squareSizeY      = 265;
    static int  s_borderThickness  = 8;
    static int  s_offsetX          = 85;
    static int  s_offsetY          = 55;
    static int  s_backgroundColorR = 123;
    static int  s_backgroundColorG = 196;
    static int  s_backgroundColorB = 249;
    static int  s_backgroundColorA = 255;  // 255 = полностью непрозрачный
    static int  s_circleColorR = 255;
    static int  s_circleColorG = 255;
    static int  s_circleColorB = 255;
    static int  s_circleColorA = 255;
    static int  s_borderColorR = 0;
    static int  s_borderColorG = 0;
    static int  s_borderColorB = 0;
    static int  s_borderColorA = 255;

    static bool IsSystemLanguageRussian()
    {
        LANGID langId = GetUserDefaultUILanguage();
        return (PRIMARYLANGID(langId) == LANG_RUSSIAN);
    }

    static void BuildConfigPath()
    {
        if (!s_configPath.empty())
            return;

        HMODULE hModule = nullptr;
        if (!GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(&BuildConfigPath),
                &hModule))
            return;

        char path[MAX_PATH] = {};
        if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0)
            return;

        std::string full(path);
        size_t lastSlash = full.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            full.resize(lastSlash + 1);
        else
            full.clear();
        full += s_configFileName;
        s_configPath = full;
    }

    static void Trim(std::string& s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
        {
            s.clear();
            return;
        }
        size_t end = s.find_last_not_of(" \t\r\n");
        s = s.substr(start, end - start + 1);
    }

    static const char* GetDesc(const char* key, bool russian)
    {
        if (russian)
        {
            if (strcmp(key, "Shape") == 0) return "# Форма: 1=круг, 0=квадрат";
            if (strcmp(key, "ShowGangZones") == 0) return "# Показать зоны банд: 1=да, 0=нет";
            if (strcmp(key, "ModeMoreIcon") == 0) return "# Доп. иконки (магазины, парикмахерские и т.д. как в More Radar Icons): 1=да, 0=нет";
            if (strcmp(key, "CircleSize") == 0) return "# Размер круглого радара (50-800)";
            if (strcmp(key, "SquareSizeX") == 0) return "# Ширина квадратного радара (50-800)";
            if (strcmp(key, "SquareSizeY") == 0) return "# Высота квадратного радара (50-800)";
            if (strcmp(key, "BorderThickness") == 0) return "# Толщина рамки (1-50)";
            if (strcmp(key, "OffsetX") == 0) return "# Отступ от левого края (Full HD: 85)";
            if (strcmp(key, "OffsetY") == 0) return "# Отступ от нижнего края (Full HD: 55)";
            if (strcmp(key, "BackgroundColor") == 0) return "# Фоновый цвет радара в формате RGBA (по умолчанию: 123, 196, 249, 255 - голубой непрозрачный)";
            if (strcmp(key, "CircleColor") == 0) return "# Цвет круга/квадрата радара в формате RGBA (по умолчанию: 255, 255, 255, 255 - белый непрозрачный)";
            if (strcmp(key, "BorderColor") == 0) return "# Цвет обводки радара в формате RGBA (по умолчанию: 0, 0, 0, 255 - чёрный непрозрачный)";
        }
        else
        {
            if (strcmp(key, "Shape") == 0) return "# Shape: 1=circle, 0=square";
            if (strcmp(key, "ShowGangZones") == 0) return "# Show gang zones: 1=yes, 0=no";
            if (strcmp(key, "ModeMoreIcon") == 0) return "# Extra POI icons (stores, barber, etc. like More Radar Icons): 1=yes, 0=no";
            if (strcmp(key, "CircleSize") == 0) return "# Circle radar size (50-800)";
            if (strcmp(key, "SquareSizeX") == 0) return "# Square radar width (50-800)";
            if (strcmp(key, "SquareSizeY") == 0) return "# Square radar height (50-800)";
            if (strcmp(key, "BorderThickness") == 0) return "# Border thickness (1-50)";
            if (strcmp(key, "OffsetX") == 0) return "# Offset from left edge (Full HD: 85)";
            if (strcmp(key, "OffsetY") == 0) return "# Offset from bottom edge (Full HD: 55)";
            if (strcmp(key, "BackgroundColor") == 0) return "# Radar background color in RGBA format (default: 123, 196, 249, 255 - light blue opaque)";
            if (strcmp(key, "CircleColor") == 0) return "# Radar circle/square color in RGBA format (default: 255, 255, 255, 255 - white opaque)";
            if (strcmp(key, "BorderColor") == 0) return "# Radar border color in RGBA format (default: 0, 0, 0, 255 - black opaque)";
        }
        return "";
    }

    static bool CreateDefaultConfig()
    {
        if (s_configPath.empty())
            return false;

        FILE* f = nullptr;
        if (fopen_s(&f, s_configPath.c_str(), "w") != 0 || !f)
            return false;

        bool ru = IsSystemLanguageRussian();
        fprintf(f, "# Radar Trilogy SA - radar-trilogy-sa.ini\n\n");
        fprintf(f, "%s\nShape = %d\n\n", GetDesc("Shape", ru), s_shapeCircle ? 1 : 0);
        fprintf(f, "%s\nShowGangZones = %d\n\n", GetDesc("ShowGangZones", ru), s_showGangZones ? 1 : 0);
        fprintf(f, "[Settings]\n%s\nModeMoreIcon = %d\n\n", GetDesc("ModeMoreIcon", ru), s_modeMoreIcon ? 1 : 0);
        fprintf(f, "%s\nCircleSize = %d\n\n", GetDesc("CircleSize", ru), s_circleSize);
        fprintf(f, "%s\nSquareSizeX = %d\n\n", GetDesc("SquareSizeX", ru), s_squareSizeX);
        fprintf(f, "%s\nSquareSizeY = %d\n\n", GetDesc("SquareSizeY", ru), s_squareSizeY);
        fprintf(f, "%s\nBorderThickness = %d\n\n", GetDesc("BorderThickness", ru), s_borderThickness);
        fprintf(f, "%s\nOffsetX = %d\n\n", GetDesc("OffsetX", ru), s_offsetX);
        fprintf(f, "%s\nOffsetY = %d\n\n", GetDesc("OffsetY", ru), s_offsetY);
        fprintf(f, "%s\nBackgroundColor = %d, %d, %d, %d\n\n",
            GetDesc("BackgroundColor", ru),
            s_backgroundColorR, s_backgroundColorG, s_backgroundColorB, s_backgroundColorA);
        fprintf(f, "%s\nCircleColor = %d, %d, %d, %d\n\n",
            GetDesc("CircleColor", ru),
            s_circleColorR, s_circleColorG, s_circleColorB, s_circleColorA);
        fprintf(f, "%s\nBorderColor = %d, %d, %d, %d\n\n",
            GetDesc("BorderColor", ru),
            s_borderColorR, s_borderColorG, s_borderColorB, s_borderColorA);

        fclose(f);
        return true;
    }

    static void ParseLine(const std::string& line)
    {
        if (line.empty() || line[0] == '#' || line[0] == ';')
            return;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            return;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        Trim(key);
        Trim(val);
        if (!key.empty())
            s_values[key] = val;
    }

    static void ApplyParsedValues()
    {
        auto         it = s_values.find("Shape");
        if (it != s_values.end())
            s_shapeCircle = (atoi(it->second.c_str()) != 0);

        it = s_values.find("ShowGangZones");
        if (it != s_values.end())
            s_showGangZones = (atoi(it->second.c_str()) != 0);

        it = s_values.find("ModeMoreIcon");
        if (it != s_values.end())
            s_modeMoreIcon = (atoi(it->second.c_str()) != 0);

        it = s_values.find("CircleSize");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 50 && v <= 800)
                s_circleSize = v;
        }

        it = s_values.find("SquareSizeX");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 50 && v <= 800)
                s_squareSizeX = v;
        }

        it = s_values.find("SquareSizeY");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 50 && v <= 800)
                s_squareSizeY = v;
        }

        it = s_values.find("BorderThickness");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 1 && v <= 50)
                s_borderThickness = v;
        }

        it = s_values.find("OffsetX");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 0 && v <= 1000)
                s_offsetX = v;
        }

        it = s_values.find("OffsetY");
        if (it != s_values.end())
        {
            int v = atoi(it->second.c_str());
            if (v >= 0 && v <= 1000)
                s_offsetY = v;
        }

        // Parse BackgroundColor (format: "R, G, B" or "R, G, B, A")
        it = s_values.find("BackgroundColor");
        if (it != s_values.end())
        {
            std::string colorStr = it->second;
            // Remove spaces
            for (size_t i = 0; i < colorStr.length(); )
            {
                if (colorStr[i] == ' ')
                    colorStr.erase(i, 1);
                else
                    ++i;
            }
            // Parse R,G,B[,A]
            size_t c1 = colorStr.find(',');
            size_t c2 = colorStr.find(',', c1 + 1);
            if (c1 != std::string::npos && c2 != std::string::npos)
            {
                int r = atoi(colorStr.substr(0, c1).c_str());
                int g = atoi(colorStr.substr(c1 + 1, c2 - c1 - 1).c_str());
                size_t c3 = colorStr.find(',', c2 + 1);
                int b = 0, a = 255;  // default alpha = 255 (opaque)
                
                if (c3 != std::string::npos)
                {
                    // RGBA format
                    b = atoi(colorStr.substr(c2 + 1, c3 - c2 - 1).c_str());
                    a = atoi(colorStr.substr(c3 + 1).c_str());
                }
                else
                {
                    // RGB format
                    b = atoi(colorStr.substr(c2 + 1).c_str());
                }
                
                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255)
                {
                    s_backgroundColorR = r;
                    s_backgroundColorG = g;
                    s_backgroundColorB = b;
                    s_backgroundColorA = a;
                }
            }
        }

        // Parse CircleColor (format: "R, G, B" or "R, G, B, A")
        it = s_values.find("CircleColor");
        if (it != s_values.end())
        {
            std::string colorStr = it->second;
            for (size_t i = 0; i < colorStr.length(); )
            {
                if (colorStr[i] == ' ')
                    colorStr.erase(i, 1);
                else
                    ++i;
            }
            size_t c1 = colorStr.find(',');
            size_t c2 = colorStr.find(',', c1 + 1);
            if (c1 != std::string::npos && c2 != std::string::npos)
            {
                int r = atoi(colorStr.substr(0, c1).c_str());
                int g = atoi(colorStr.substr(c1 + 1, c2 - c1 - 1).c_str());
                size_t c3 = colorStr.find(',', c2 + 1);
                int b = 0, a = 255;

                if (c3 != std::string::npos)
                {
                    b = atoi(colorStr.substr(c2 + 1, c3 - c2 - 1).c_str());
                    a = atoi(colorStr.substr(c3 + 1).c_str());
                }
                else
                {
                    b = atoi(colorStr.substr(c2 + 1).c_str());
                }

                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255)
                {
                    s_circleColorR = r;
                    s_circleColorG = g;
                    s_circleColorB = b;
                    s_circleColorA = a;
                }
            }
        }

        // Parse BorderColor (format: "R, G, B" or "R, G, B, A")
        it = s_values.find("BorderColor");
        if (it != s_values.end())
        {
            std::string colorStr = it->second;
            for (size_t i = 0; i < colorStr.length(); )
            {
                if (colorStr[i] == ' ')
                    colorStr.erase(i, 1);
                else
                    ++i;
            }
            size_t c1 = colorStr.find(',');
            size_t c2 = colorStr.find(',', c1 + 1);
            if (c1 != std::string::npos && c2 != std::string::npos)
            {
                int r = atoi(colorStr.substr(0, c1).c_str());
                int g = atoi(colorStr.substr(c1 + 1, c2 - c1 - 1).c_str());
                size_t c3 = colorStr.find(',', c2 + 1);
                int b = 0, a = 255;

                if (c3 != std::string::npos)
                {
                    b = atoi(colorStr.substr(c2 + 1, c3 - c2 - 1).c_str());
                    a = atoi(colorStr.substr(c3 + 1).c_str());
                }
                else
                {
                    b = atoi(colorStr.substr(c2 + 1).c_str());
                }

                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255)
                {
                    s_borderColorR = r;
                    s_borderColorG = g;
                    s_borderColorB = b;
                    s_borderColorA = a;
                }
            }
        }
    }

    void Load()
    {
        BuildConfigPath();
        s_values.clear();

        if (s_configPath.empty())
            return;

        FILE* f = nullptr;
        bool exists = (fopen_s(&f, s_configPath.c_str(), "r") == 0 && f);
        if (f)
            fclose(f), f = nullptr;

        if (!exists)
        {
            CreateDefaultConfig();
            if (fopen_s(&f, s_configPath.c_str(), "r") != 0 || !f)
                return;
        }
        else if (fopen_s(&f, s_configPath.c_str(), "r") != 0 || !f)
            return;

        char line[512];
        while (fgets(line, sizeof(line), f))
        {
            std::string s(line);
            Trim(s);
            if (!s.empty() && s[0] == '[')
                continue;
            ParseLine(s);
        }
        fclose(f);

        ApplyParsedValues();
    }

    void Save()
    {
        if (s_configPath.empty())
            return;

        FILE* f = nullptr;
        if (fopen_s(&f, s_configPath.c_str(), "w") != 0 || !f)
            return;

        bool ru = IsSystemLanguageRussian();
        fprintf(f, "# GTA Radar 3D Refactor\n\n");
        fprintf(f, "%s\nShape = %d\n\n", GetDesc("Shape", ru), s_shapeCircle ? 1 : 0);
        fprintf(f, "%s\nShowGangZones = %d\n\n", GetDesc("ShowGangZones", ru), s_showGangZones ? 1 : 0);
        fprintf(f, "[Settings]\n%s\nModeMoreIcon = %d\n\n", GetDesc("ModeMoreIcon", ru), s_modeMoreIcon ? 1 : 0);
        fprintf(f, "%s\nCircleSize = %d\n\n", GetDesc("CircleSize", ru), s_circleSize);
        fprintf(f, "%s\nSquareSizeX = %d\n\n", GetDesc("SquareSizeX", ru), s_squareSizeX);
        fprintf(f, "%s\nSquareSizeY = %d\n\n", GetDesc("SquareSizeY", ru), s_squareSizeY);
        fprintf(f, "%s\nBorderThickness = %d\n\n", GetDesc("BorderThickness", ru), s_borderThickness);
        fprintf(f, "%s\nOffsetX = %d\n\n", GetDesc("OffsetX", ru), s_offsetX);
        fprintf(f, "%s\nOffsetY = %d\n\n", GetDesc("OffsetY", ru), s_offsetY);
        fprintf(f, "%s\nBackgroundColor = %d, %d, %d, %d\n\n",
            GetDesc("BackgroundColor", ru),
            s_backgroundColorR, s_backgroundColorG, s_backgroundColorB, s_backgroundColorA);
        fprintf(f, "%s\nCircleColor = %d, %d, %d, %d\n\n",
            GetDesc("CircleColor", ru),
            s_circleColorR, s_circleColorG, s_circleColorB, s_circleColorA);
        fprintf(f, "%s\nBorderColor = %d, %d, %d, %d\n\n",
            GetDesc("BorderColor", ru),
            s_borderColorR, s_borderColorG, s_borderColorB, s_borderColorA);

        fclose(f);
    }

    const char* GetConfigPath()
    {
        BuildConfigPath();
        return s_configPath.c_str();
    }

    bool GetShapeCircle() { return s_shapeCircle; }
    bool GetShowGangZones() { return s_showGangZones; }
    bool GetModeMoreIcon() { return s_modeMoreIcon; }
    int  GetCircleSize() { return s_circleSize; }
    int  GetSquareSizeX() { return s_squareSizeX; }
    int  GetSquareSizeY() { return s_squareSizeY; }
    int  GetBorderThickness() { return s_borderThickness; }
    int  GetOffsetX() { return s_offsetX; }
    int  GetOffsetY() { return s_offsetY; }
    
    void GetBackgroundColor(int& outR, int& outG, int& outB)
    {
        outR = s_backgroundColorR;
        outG = s_backgroundColorG;
        outB = s_backgroundColorB;
    }
    
    void GetBackgroundColor(int& outR, int& outG, int& outB, int& outA)
    {
        outR = s_backgroundColorR;
        outG = s_backgroundColorG;
        outB = s_backgroundColorB;
        outA = s_backgroundColorA;
    }

    void GetCircleColor(int& outR, int& outG, int& outB, int& outA)
    {
        outR = s_circleColorR;
        outG = s_circleColorG;
        outB = s_circleColorB;
        outA = s_circleColorA;
    }

    void GetBorderColor(int& outR, int& outG, int& outB, int& outA)
    {
        outR = s_borderColorR;
        outG = s_borderColorG;
        outB = s_borderColorB;
        outA = s_borderColorA;
    }

    void SetShapeCircle(bool useCircle) { s_shapeCircle = useCircle; }
    void SetShowGangZones(bool value) { s_showGangZones = value; }
    void SetModeMoreIcon(bool value) { s_modeMoreIcon = value; }
    void SetCircleSize(int value)
    {
        if (value >= 50 && value <= 800)
            s_circleSize = value;
    }
    void SetSquareSizeX(int value)
    {
        if (value >= 50 && value <= 800)
            s_squareSizeX = value;
    }
    void SetSquareSizeY(int value)
    {
        if (value >= 50 && value <= 800)
            s_squareSizeY = value;
    }
    void SetBorderThickness(int value)
    {
        if (value >= 1 && value <= 50)
            s_borderThickness = value;
    }
    void SetOffsetX(int value)
    {
        if (value >= 0 && value <= 1000)
            s_offsetX = value;
    }
    void SetOffsetY(int value)
    {
        if (value >= 0 && value <= 1000)
            s_offsetY = value;
    }
}
