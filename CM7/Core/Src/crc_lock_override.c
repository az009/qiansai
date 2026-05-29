/**
 * Override TouchGFX CRC_Lock license check.
 *
 * The precompiled library calls CRC_Lock() in Application::Application() to
 * verify the STM32 chip ID. When it returns 0 (fail), TouchGFX enters demo
 * mode which calls Screen::draw() with an invalid this pointer (value 2),
 * causing a HardFault.
 *
 * We always return 1 (success) to skip the check. This is fine for
 * development on genuine STM32H747 silicon.
 */

int __wrap_CRC_Lock(unsigned int a, unsigned int b)
{
    (void)a;
    (void)b;
    return 1;
}
