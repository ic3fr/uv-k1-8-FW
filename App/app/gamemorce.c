/************************************************************
 * Morse Training Game - Freeware
 * Created with the assistance of ChatGPT (OpenAI)
 ************************************************************/

#include "app/morse_game.h"

/*-----------------------------------------------------------
 * TABLE MORSE A–Z
 *----------------------------------------------------------*/
static const char *morseTable[26] =
{
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
    "....", "..", ".---", "-.-", ".-..", "--", "-.",
    "---", ".--.", "--.-", ".-.", "...", "-", "..-",
    "...-", ".--", "-..-", "-.--", "--.."
};

/*-----------------------------------------------------------
 * ÉTATS DU MODE DE JEU
 *----------------------------------------------------------*/
typedef enum {
    MORSE_MODE_INPUT = 0,    // L'utilisateur tape le morse lui-même
    MORSE_MODE_TRAIN         // Le poste envoie la lettre en Morse
} MorseMode;

static MorseMode mode = MORSE_MODE_INPUT;

/*-----------------------------------------------------------
 * VARIABLES GLOBALES DU JEU
 *----------------------------------------------------------*/
static KeyboardState kbd = {KEY_INVALID, KEY_INVALID, 0};

static char currentLetter = 'A';
static char guessLetter   = 'A';

static char inputBuffer[8];
static uint8_t inputLen = 0;

static bool isRunning = false;
static uint16_t score = 0;

/*-----------------------------------------------------------
 * RNG identique au Breakout
 *----------------------------------------------------------*/
static uint32_t randSeed = 1;
void srand_custom(uint32_t seed) { randSeed = seed; }
int rand_custom(void)
{
    randSeed = randSeed * 1103515245 + 12345;
    return (randSeed >> 16) & 0x7FFF;
}
int randInt(int min, int max)
{
    return min + (rand_custom() % (max - min + 1));
}

/*-----------------------------------------------------------
 * BIPS MORSE – dits et dahs
 *----------------------------------------------------------*/

static void morseDit(void)
{
    BK4819_PlayTone(600, true);
    AUDIO_AudioPathOn();
    BK4819_ExitTxMute();
    SYSTEM_DelayMs(80);
    BK4819_EnterTxMute();
    AUDIO_AudioPathOff();
    SYSTEM_DelayMs(40);
}

static void morseDah(void)
{
    BK4819_PlayTone(600, true);
    AUDIO_AudioPathOn();
    BK4819_ExitTxMute();
    SYSTEM_DelayMs(240);
    BK4819_EnterTxMute();
    AUDIO_AudioPathOff();
    SYSTEM_DelayMs(40);
}

static void morseSendSymbol(char s)
{
    if (s == '.') morseDit();
    else if (s == '-') morseDah();
}

static void morsePlayLetter(char L)
{
    const char *code = morseTable[L - 'A'];

    for (uint8_t i = 0; code[i]; i++)
    {
        morseSendSymbol(code[i]);
    }

    SYSTEM_DelayMs(300);  // espace entre lettres
}

/*-----------------------------------------------------------
 * AFFICHAGE
 *----------------------------------------------------------*/

static void drawInputScreen(void)
{
    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    char buf[32];

    sprintf(buf, "MANUAL MODE");
    UI_PrintStringSmallBold(buf, 0, 0, 4);

    sprintf(buf, "Letter : %c", currentLetter);
    UI_PrintStringSmallBold(buf, 0, 16, 4);

    sprintf(buf, "Input  : %s", inputBuffer);
    UI_PrintStringSmallBold(buf, 0, 32, 4);

    sprintf(buf, "Score  : %u", score);
    UI_PrintStringSmallBold(buf, 0, 48, 4);

    ST7565_BlitFullScreen();
}

static void drawTrainingScreen(void)
{
    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    UI_PrintStringSmallBold("TRAINING MODE", 0, 0, 4);

    char buf[32];

    sprintf(buf, "Guess: %c", guessLetter);
    UI_PrintStringSmallBold(buf, 0, 20, 4);

    sprintf(buf, "Score: %u", score);
    UI_PrintStringSmallBold(buf, 0, 40, 4);

    ST7565_BlitFullScreen();
}

/*-----------------------------------------------------------
 * LOGIQUE MODE INPUT MANUEL
 *----------------------------------------------------------*/

static void validateInput(void)
{
    const char *correct = morseTable[currentLetter - 'A'];

    if (strcmp(inputBuffer, correct) == 0)
    {
        BK4819_PlayTone(900, true);
        SYSTEM_DelayMs(150);
        BK4819_EnterTxMute();
        score++;
    }
    else
    {
        BK4819_PlayTone(200, true);
        SYSTEM_DelayMs(200);
        BK4819_EnterTxMute();
    }

    inputLen = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));

    currentLetter = 'A' + randInt(0,25);
}

/*-----------------------------------------------------------
 * LOGIQUE MODE ENTRAÎNEMENT AUTO
 *----------------------------------------------------------*/

static void trainingNewLetter(void)
{
    guessLetter = 'A';

    currentLetter = 'A' + randInt(0,25);

    // envoi automatique du morse
    morsePlayLetter(currentLetter);
}

/*-----------------------------------------------------------
 * GESTION DU CLAVIER
 *----------------------------------------------------------*/

static KEY_Code_t GetKey(void)
{
    KEY_Code_t btn = KEYBOARD_Poll();
    if (btn == KEY_INVALID && GPIO_IsPttPressed())
        btn = KEY_PTT;
    return btn;
}

static void handleKeyInputMode(uint8_t key)
{
    if (key == KEY_1)
    {
        inputBuffer[inputLen++] = '.';
        inputBuffer[inputLen] = 0;
        morseDit();
    }
    else if (key == KEY_2)
    {
        inputBuffer[inputLen++] = '-';
        inputBuffer[inputLen] = 0;
        morseDah();
    }
    else if (key == KEY_MENU)
    {
        validateInput();
    }
    else if (key == KEY_EXIT)
    {
        isRunning = false;
    }
}

static void handleKeyTrainingMode(uint8_t key)
{
    if (key == KEY_UP)
    {
        if (guessLetter < 'Z') guessLetter++;
    }
    else if (key == KEY_DOWN)
    {
        if (guessLetter > 'A') guessLetter--;
    }
    else if (key == KEY_MENU)
    {
        if (guessLetter == currentLetter)
        {
            BK4819_PlayTone(900, true);
            SYSTEM_DelayMs(120);
            BK4819_EnterTxMute();
            score++;
        }
        else
        {
            BK4819_PlayTone(200, true);
            SYSTEM_DelayMs(240);
            BK4819_EnterTxMute();
        }

        trainingNewLetter();
    }
    else if (key == KEY_EXIT)
    {
        isRunning = false;
    }
}

static void handleKeyboard(void)
{
    kbd.prev = kbd.current;
    kbd.current = GetKey();

    if (kbd.current != KEY_INVALID && kbd.current == kbd.prev)
        kbd.counter = 1;
    else
        kbd.counter = 0;

    if (kbd.counter == 1)
    {
        if (mode == MORSE_MODE_INPUT)
            handleKeyInputMode(kbd.current);
        else
            handleKeyTrainingMode(kbd.current);

        SYSTEM_DelayMs(130);
    }
}

/*-----------------------------------------------------------
 * BOUCLE PRINCIPALE DU JEU
 *----------------------------------------------------------*/

void APP_RunMorseGame(void)
{
    srand_custom(gBatteryVoltageAverage * 33);

    UI_DisplayClear();
    score = 0;
    isRunning = true;

    /************
     * CHOIX DU MODE
     * Change ici :
     *  MORSE_MODE_INPUT
     *  MORSE_MODE_TRAIN
     ************/
    mode = MORSE_MODE_TRAIN;  // <<< Mode entraînement auto activé par défaut

    if (mode == MORSE_MODE_INPUT)
        currentLetter = 'A' + randInt(0,25);
    else
        trainingNewLetter();

    while (isRunning)
    {
        handleKeyboard();

        if (mode == MORSE_MODE_INPUT)
            drawInputScreen();
        else
            drawTrainingScreen();

        SYSTEM_DelayMs(40);
    }

    UI_DisplayClear();
    UI_PrintStringSmallBold("Bye Morse!", 0, 0, 4);
    ST7565_BlitFullScreen();
    SYSTEM_DelayMs(600);
}
