#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <mvp/View.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include "Screen1Presenter.hpp"

class Screen1View : public touchgfx::View<Screen1Presenter>
{
public:
    Screen1View();
    virtual ~Screen1View() {}

    void setupScreen() override;
    void tearDownScreen() override;

private:
    touchgfx::Box background;
    touchgfx::Box redBox;
    touchgfx::Circle yellowCircle;
    touchgfx::PainterRGB565 yellowPainter;
};

#endif
