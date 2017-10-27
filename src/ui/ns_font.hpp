
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

class FontNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();
        table["Create"] = [](std::string name, int size) {
            new FontValue(
                CreateEngineFont(
                    CreateFont(
                    (int)pnumberSize->GetValue(),
                        (int)pnumberStretch->GetValue(), 0, 0,
                        pboolBold->GetValue() ? FW_BOLD : FW_DONTCARE,
                        FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN,
                        pstringName->GetValue()
                    )
                )
            );
        };
        m_pLua->set("Font", table);
    }
};
