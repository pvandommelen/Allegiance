
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

using namespace std::literals;

class PointV : public PointValue {
public:
    PointV(Number* px, Number* py) :
        PointValue(px, py)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            Point(
                Get0()->GetValue(),
                Get1()->GetValue()
            );
    }
};

class PointNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        
        table["Create"] = [](sol::object x, sol::object y) {
            return (PointValue*)new PointV(
                wrapValue<float>(x),
                wrapValue<float>(y)
            );
            //return new PointValue(Point(x, y));
        };

        //table["Create"] = [](float x, float y) {
        //    return new PointValue(Point(x, y));
        //};
        table["X"] = [](PointValue* pPoint) {
            return (Number*)new TransformedValue<float, Point>([](Point point) {
                return point.X();
            }, pPoint);
        };
        table["Y"] = [](PointValue* pPoint) {
            return (Number*)new TransformedValue<float, Point>([](Point point) {
                return point.Y();
            }, pPoint);
        };
        m_pLua->set("Point", table);
    }
};
