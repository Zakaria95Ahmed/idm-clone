/**
 * @file SkinManager.cpp
 */

#include "stdafx.h"
#include "SkinManager.h"

namespace idm {

SkinManager& SkinManager::Instance() {
    static SkinManager instance;
    return instance;
}

void SkinManager::SetToolbarStyle(ToolbarStyle style) {
    m_style = style;
}

SkinManager::ColorScheme SkinManager::GetLightScheme() const {
    return {
        RGB(240, 240, 240),   // background
        RGB(0, 0, 0),         // text
        RGB(51, 153, 255),    // selection
        RGB(200, 200, 200),   // border
        RGB(235, 235, 235),   // toolbarBg
        RGB(230, 230, 230),   // statusBarBg
        RGB(255, 255, 255),   // treeBackground
        RGB(255, 255, 255)    // listBackground
    };
}

SkinManager::ColorScheme SkinManager::GetDarkScheme() const {
    return {
        RGB(32, 32, 32),      // background
        RGB(220, 220, 220),   // text
        RGB(0, 120, 212),     // selection
        RGB(60, 60, 60),      // border
        RGB(45, 45, 45),      // toolbarBg
        RGB(40, 40, 40),      // statusBarBg
        RGB(30, 30, 30),      // treeBackground
        RGB(25, 25, 25)       // listBackground
    };
}

SkinManager::ColorScheme SkinManager::GetCurrentScheme() const {
    return m_darkMode ? GetDarkScheme() : GetLightScheme();
}

} // namespace idm
