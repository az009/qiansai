#ifndef SCREEN1PRESENTER_HPP
#define SCREEN1PRESENTER_HPP

#include <mvp/Presenter.hpp>
#include <gui/model/Model.hpp>

class Screen1View;

class Screen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1Presenter(Screen1View& v) : view(v) {}

    void activate() override {}
    void deactivate() override {}
    void bind(Model* m) { model = m; }

private:
    Screen1View& view;
    Model* model = nullptr;
};

#endif
