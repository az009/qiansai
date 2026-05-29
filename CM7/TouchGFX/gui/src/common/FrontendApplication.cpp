#include <new>
#include <mvp/View.hpp>
#include <mvp/MVPApplication.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screen1_screen/Screen1View.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : touchgfx::MVPApplication(),
      transitionCallback(),
      frontendHeap(heap),
      model(m)
{
}

void FrontendApplication::gotoScreen1Screen()
{
    transitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoScreen1ScreenImpl);
    pendingScreenTransitionCallback = &transitionCallback;
}

void FrontendApplication::gotoScreen1ScreenImpl()
{
    touchgfx::makeTransition<Screen1View, Screen1Presenter, touchgfx::NoTransition, Model>(
        &currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
