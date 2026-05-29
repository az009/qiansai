#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <mvp/MVPApplication.hpp>
#include <gui/model/Model.hpp>

class FrontendHeap;

class FrontendApplication : public touchgfx::MVPApplication
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);

    void gotoScreen1Screen();

private:
    void gotoScreen1ScreenImpl();

    touchgfx::Callback<FrontendApplication> transitionCallback;
    FrontendHeap& frontendHeap;
    Model& model;
};

#endif
