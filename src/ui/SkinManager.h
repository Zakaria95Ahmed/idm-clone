/**
 * @file SkinManager.h / SkinManager.cpp
 * @brief Toolbar skin/theme manager
 */

#pragma once
#include "stdafx.h"

namespace idm {

enum class ToolbarStyle { Large3D = 0, Small3D, LargeNeon, SmallNeon };

class SkinManager {
public:
    static SkinManager& Instance();
    
    void SetToolbarStyle(ToolbarStyle style);
    ToolbarStyle GetToolbarStyle() const { return m_style; }
    
    // Color scheme for dark/light mode
    struct ColorScheme {
        COLORREF background;
        COLORREF text;
        COLORREF selection;
        COLORREF border;
        COLORREF toolbarBg;
        COLORREF statusBarBg;
        COLORREF treeBackground;
        COLORREF listBackground;
    };
    
    ColorScheme GetLightScheme() const;
    ColorScheme GetDarkScheme() const;
    ColorScheme GetCurrentScheme() const;
    
    void SetDarkMode(bool dark) { m_darkMode = dark; }
    bool IsDarkMode() const { return m_darkMode; }
    
private:
    SkinManager() = default;
    ToolbarStyle m_style{ToolbarStyle::Large3D};
    bool m_darkMode{false};
};

} // namespace idm
