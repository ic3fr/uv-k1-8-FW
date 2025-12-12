/************************************************************
 * Morse Training Game - Freeware
 * Created with the assistance of ChatGPT (OpenAI)
 *
 * Compatible with the same environment as Breakout:
 * - BK4819 audio
 * - ST7565 LCD
 * - Keyboard driver
 * - Framebuffer-based UI
 *
 * Provides:
 *  - Manual Morse input mode
 *  - Automatic training mode (radio sends Morse, user guesses)
 ************************************************************/

#pragma once

#include "keyboard_state.h"

#include "../bitmaps.h"
#include "../board.h"
#include "py32f0xx.h"
#include "../driver/bk4819-regs.h"
#include "../driver/bk4819.h"
#include "../driver/gpio.h"
#include "../driver/keyboard.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/systick.h"
#include "../external/printf/printf.h"
#include "../font.h"
#include "../helper/battery.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/helper.h"
#include "../audio.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Lance le jeu
void APP_RunMorseGame(void);



