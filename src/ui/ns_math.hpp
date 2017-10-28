
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

template<typename TypeResult, typename A, typename B>
auto createAutoWrappingFunction(std::function<TypeResult(A, B)> callback) {

    return [callback](sol::object a, sol::object b) {
        return (TStaticValue<TypeResult>*)new TransformedValue2<TypeResult, A, B>(callback, wrapValue<A>(a), wrapValue<B>(b));
    };
};

class NumberNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = [](float a) {
            return new Number(a);
        };

        table["Add2"] = sol::overload(
            [](Number* a, Number* b) {
                return (Number*)new TransformedValue2<float, float, float>([](float a, float b) {
                    return a + b;
                }, a, b);
            }
        );

        table["Add"] = createAutoWrappingFunction<float, float, float>([](float a, float b) {
            return a + b;
        });

        table["Divide"] = [](sol::object a, sol::object b) {
            return (Number*)new TransformedValue2<float, float, float>([](float a, float b) {
                return a / b;
            }, wrapValue<float>(a), wrapValue<float>(b));
        };

        m_pLua->new_usertype<Number>("Number");

        m_pLua->set("Number", table);
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