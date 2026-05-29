#ifndef FRONTENDHEAP_HPP
#define FRONTENDHEAP_HPP

#include <common/Meta.hpp>
#include <common/Partition.hpp>
#include <mvp/MVPHeap.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/model/Model.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screen1_screen/Screen1View.hpp>

class FrontendHeap : public touchgfx::MVPHeap
{
public:
    typedef touchgfx::meta::TypeList<Screen1View,
                                     touchgfx::meta::Nil>
        ViewTypes;

    typedef touchgfx::meta::select_type_maxsize<ViewTypes>::type MaxViewType;

    typedef touchgfx::meta::TypeList<Screen1Presenter,
                                     touchgfx::meta::Nil>
        PresenterTypes;

    typedef touchgfx::meta::select_type_maxsize<PresenterTypes>::type MaxPresenterType;

    typedef touchgfx::meta::TypeList<touchgfx::NoTransition,
                                     touchgfx::meta::Nil>
        TransitionTypes;

    typedef touchgfx::meta::select_type_maxsize<TransitionTypes>::type MaxTransitionType;

    static FrontendHeap& getInstance()
    {
        static FrontendHeap instance;
        return instance;
    }

    touchgfx::Partition<PresenterTypes, 1> presenters;
    touchgfx::Partition<ViewTypes, 1> views;
    touchgfx::Partition<TransitionTypes, 1> transitions;
    Model model;
    FrontendApplication app;

private:
    FrontendHeap()
        : MVPHeap(presenters, views, transitions, app),
          presenters(),
          views(),
          transitions(),
          model(),
          app(model, *this)
    {
        app.gotoScreen1Screen();
    }
};

#endif
