
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

class ColorNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();
        table["Create"] = [](float r, float g, float b, sol::optional<float> alpha) {
            return new ColorValue(Color(r, g, b, alpha.value_or(1.0f)));
        };
        m_pLua->set("Color", table);
    }
};
