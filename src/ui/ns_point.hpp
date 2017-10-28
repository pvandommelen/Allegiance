
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

using namespace std::literals;

class PointNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = [](int x, int y) {
            return new PointValue(Point(x, y));
        };
        m_pLua->set("Point", table);
    }
};
