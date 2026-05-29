#include <touchgfx/hal/GPIO.hpp>

namespace touchgfx
{

void GPIO::init()
{
}

void GPIO::set(GPIO_ID id)
{
    (void)id;
}

void GPIO::clear(GPIO_ID id)
{
    (void)id;
}

void GPIO::toggle(GPIO_ID id)
{
    (void)id;
}

bool GPIO::get(GPIO_ID id)
{
    (void)id;
    return false;
}

} // namespace touchgfx
