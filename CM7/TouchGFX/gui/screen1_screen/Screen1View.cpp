#include "Screen1View.hpp"
#include <touchgfx/Color.hpp>

Screen1View::Screen1View()
{
}

void Screen1View::setupScreen()
{
    background.setPosition(0, 0, 800, 480);
    background.setColor(touchgfx::Color::getColorFromRGB(0x20, 0x20, 0x40));
    add(background);

    redBox.setPosition(50, 50, 200, 100);
    redBox.setColor(touchgfx::Color::getColorFromRGB(0xFF, 0x00, 0x00));
    add(redBox);

    yellowPainter.setColor(touchgfx::Color::getColorFromRGB(0xFF, 0xFF, 0x00));
    yellowCircle.setPosition(320, 160, 160, 160);
    yellowCircle.setCircle(80, 80, 80);
    yellowCircle.setPainter(yellowPainter);
    yellowCircle.setAlpha(255);
    add(yellowCircle);

    touchgfx::View<Screen1Presenter>::setupScreen();
}

void Screen1View::tearDownScreen()
{
    touchgfx::View<Screen1Presenter>::tearDownScreen();
}
