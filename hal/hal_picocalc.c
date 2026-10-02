/*
 *  YASAG  -  HAL for the ClockworkPi PicoCalc  (RP2040 / RP2350)
 *  ---------------------------------------------------------------------------
 *  Thin forwarders onto the vendored picocalc-text-starter drivers by Blair
 *  Leduc (LCD on spi1, STM32 south-bridge keyboard on i2c1, PIO square-wave
 *  audio on GP26/27) plus the Raspberry Pi Pico SDK.  This is the ONLY file
 *  in the PicoCalc build that touches hardware; game.c is platform-neutral.
 *
 *  Behaviour here is intended to be identical to picocalc-space-game V1.00A.
 */
#include "hal.h"

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "drivers/lcd.h"          /* lcd_init(), lcd_solid_rectangle(), lcd_enable_cursor() */
#include "drivers/southbridge.h"  /* sb_init(), sb_read_keyboard()                          */
#include "drivers/audio.h"        /* audio_init(), audio_play_sound(), audio_stop()         */

/* --- Video ------------------------------------------------------------- */

void vid_init(void)
{
    lcd_init();                   /* turns the display on */
    lcd_enable_cursor(false);     /* no blink cursor - game.c draws its own text (text.c) */
}

void vid_fill_rect(uint16_t rgb565, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
        return;
    lcd_solid_rectangle(rgb565, (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
}

void vid_present(void)
{
    /* The ST7789 GRAM *is* the framebuffer - writes are already on the panel. */
}

void vid_reboot_to_loader(void)
{
    rom_reset_usb_boot(0, 0);     /* USB mass-storage bootloader; does not return */
}

/* --- Input ----------------------------------------------------------------- *
 *  sb_read_keyboard() returns (state << 8) | code, or 0 when the FIFO is
 *  empty - exactly the shape inp_poll() promises, so this is a straight split. */

void inp_init(void)
{
    sb_init();
}

bool inp_poll(inp_event_t *out)
{
    uint16_t e = sb_read_keyboard();
    if (!e)
        return false;
    out->state = (uint8_t)((e >> 8) & 0xFF);
    out->code  = (uint8_t)(e & 0xFF);
    return true;
}

void inp_flush(void)
{
    while (sb_read_keyboard())
        ;
}

/* --- Audio ----------------------------------------------------------------- */

void snd_init(void)                       { audio_init(); }
void snd_tone(uint16_t l, uint16_t r)     { audio_play_sound(l, r); }
void snd_off(void)                        { audio_stop(); }

/* --- Time ----------------------------------------------------------------- */

uint32_t hal_now_ms(void)          { return to_ms_since_boot(get_absolute_time()); }
void     hal_sleep_ms(uint32_t ms) { sleep_ms(ms); }
uint32_t hal_entropy(void)         { return time_us_32(); }
