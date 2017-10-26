
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

class MathNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        m_pLua->set("Math", table);
    }
};

class RectNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = [](int left, int bottom, int width, int height) {
            return new RectValue(Rect(left, bottom, width, height));
        };

        m_pLua->set("Rect", table);
    }
};