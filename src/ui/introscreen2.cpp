#include "pch.h"

#include "ui.h"
#include "introscreen2.h"


IntroScreen2::IntroScreen2()
{
}


IntroScreen2::~IntroScreen2()
{
}

Image* IntroScreen2::CreateImage()
{
    sol::state lua;
    int value = lua.script("return 54");

    return Image::GetEmpty();
}