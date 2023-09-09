#include "global.h"
#include "option_menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "event_data.h"
#include "malloc.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "gba/m4a_internal.h"
#include "constants/rgb.h"

//------------------------------------------------------------------
// Menu Enums
//------------------------------------------------------------------
enum
{
    MENU_GENERAL,
    MENU_DIFFICULTY,
    MENU_COUNT,
};

// General Menu items
enum
{
    MENUITEM_GENERAL_TEXTSPEED,
    MENUITEM_GENERAL_BUTTONMODE,
    MENUITEM_GENERAL_SOUND,
    MENUITEM_GENERAL_MOVEMENTMODE,
    MENUITEM_GENERAL_MOVEANIMATIONS,
    MENUITEM_GENERAL_ENTRYANIMATIONS,
    MENUITEM_GENERAL_CLOCKFORMAT,
    MENUITEM_GENERAL_UNIT_SYSTEM,
    MENUITEM_GENERAL_FRAMETYPE,
    MENUITEM_GENERAL_CANCEL,
    MENUITEM_GENERAL_COUNT,
};

// Difficulty Menu items
enum
{
    MENUITEM_DIFFICULTY_EXPSHARE,
    MENUITEM_DIFFICULTY_BATTLESTYLE,
    MENUITEM_DIFFICULTY_BATTLEITEMS,
    MENUITEM_DIFFICULTY_LEVELSCALING,
    MENUITEM_DIFFICULTY_ENEMYAI,
    MENUITEM_DIFFICULTY_CANCEL,
    MENUITEM_DIFFICULTY_COUNT,
};

//------------------------------------------------------------------
// Window Ids
//------------------------------------------------------------------
enum
{
    WIN_TOPBAR,
    WIN_OPTIONS,
    WIN_DESCRIPTION
};

static const struct WindowTemplate sOptionMenuWinTemplates[] =
{
    {//WIN_TOPBAR
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    {//WIN_OPTIONS
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 10,
        .paletteNum = 1,
        .baseBlock = 62
    },
    {//WIN_DESCRIPTION
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 1,
        .baseBlock = 500
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
    {
       .bg = 0,
       .charBaseIndex = 1,
       .mapBaseIndex = 30,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 1,
       .baseTile = 0
    },
    {
       .bg = 1,
       .charBaseIndex = 1,
       .mapBaseIndex = 31,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 0,
       .baseTile = 0
    },
};

struct OptionMenu
{
    u8 submenu;
    u8 sel_general[MENUITEM_GENERAL_COUNT];
    u8 sel_difficulty[MENUITEM_DIFFICULTY_COUNT];
    s32 menuCursor[MENU_COUNT];
    s32 visibleCursor[MENU_COUNT];
    u8 arrowTaskId;
};

#define Y_DIFF 16 // Difference in pixels between items.
#define OPTIONS_ON_SCREEN 5
#define NUM_OPTIONS_FROM_BORDER 1
#define SCROLL_DIR_DOWN 0
#define SCROLL_DIR_UP   1

//------------------------------------------------------------------
// local functions
//------------------------------------------------------------------
static void MainCB2(void);
static void VBlankCB(void);
static void DrawTopBarText(void); //top Option text
static void DrawLeftSideOptionText(s32 selection, s32 y);
static void DrawRightSideChoiceText(const u8 *str, s32 x, s32 y, bool8 choosen, bool8 active);
static void DrawOptionMenuTexts(void); //left side text;
static void DrawChoices(u32 id, s32 y); //right side draw function
static void HighlightOptionMenuItem(void);
static void Task_OptionMenuFadeIn(u8 taskId);
static void Task_OptionMenuProcessInput(u8 taskId);
static void Task_OptionMenuSave(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void ScrollMenu(s32 direction);
static void ScrollAll(s32 direction); // to bottom or top
static s32 GetMiddleX(const u8 *txt1, const u8 *txt2, const u8 *txt3);
static s32 XOptions_ProcessInput(s32 x, s32 selection);
static s32 ProcessInput_Options_Two(s32 selection);
static s32 ProcessInput_Options_Three(s32 selection);
static s32 ProcessInput_Options_Four(s32 selection);
static s32 ProcessInput_Sound(s32 selection);
static s32 ProcessInput_FrameType(s32 selection);
static const u8 *const OptionTextDescription(void);
static const u8 *const OptionTextRight(u8 menuItem);
static u8 MenuItemCount(void);
static u8 MenuItemCancel(void);
static void DrawDescriptionText(void);
static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style, bool8 active);
static void DrawChoices_Options_Four(const u8 *const *const strings, s32 selection, s32 y, bool8 active);
static void ReDrawAll(void);
// Options Draw Choices
static void DrawChoices_TextSpeed(s32 selection, s32 y);
static void DrawChoices_ButtonMode(s32 selection, s32 y);
static void DrawChoices_Sound(s32 selection, s32 y);
static void DrawChoices_MovementMode(s32 selection, s32 y);
static void DrawChoices_MoveAnimations(s32 selection, s32 y);
static void DrawChoices_EntryAnimations(s32 selection, s32 y);
static void DrawChoices_ClockFormat(s32 selection, s32 y);
static void DrawChoices_UnitSystem(s32 selection, s32 y);
static void DrawChoices_FrameType(s32 selection, s32 y);
// Difficulty Draw Choices
static void DrawChoices_ExpShare(s32 selection, s32 y);
static void DrawChoices_BattleStyle(s32 selection, s32 y);
static void DrawChoices_BattleItems(s32 selection, s32 y);
static void DrawChoices_LevelScaling(s32 selection, s32 y);
static void DrawChoices_EnemyAI(s32 selection, s32 y);
static void DrawBgWindowFrames(void);

//------------------------------------------------------------------
// EWRAM vars
//------------------------------------------------------------------
EWRAM_DATA static struct OptionMenu *sOptions = NULL;

//------------------------------------------------------------------
// const data
//------------------------------------------------------------------
static const u8 sEqualSignGfx[] = INCBIN_U8("graphics/interface/option_menu_equals_sign.4bpp"); // note: this is only used in the Japanese release
static const u16 sOptionMenuBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sOptionMenuText_Pal[] = INCBIN_U16("graphics/interface/option_menu_text.gbapal");

// Text colors
#define COLOR_TRANSPARENT    0
#define COLOR_WHITE          1
#define COLOR_GRAY_SHADOW    2
#define COLOR_GRAY_FG        3
#define COLOR_RED            4
#define COLOR_RED_SHADOW     5
#define COLOR_RED_FG         6
#define COLOR_ORANGE_SHADOW  7
#define COLOR_ORANGE_FG      8
#define COLOR_BLUE_SHADOW    9
#define COLOR_BLUE_FG        10
#define COLOR_TURQUOISE      11
#define COLOR_GREEN_SHADOW   12
#define COLOR_GREEN_FG       13
#define COLOR_PURPLE         14
#define COLOR_BLACK          15

//------------------------------------------------------------------
// Menu draw and input functions
//------------------------------------------------------------------
struct // MENU_GENERAL
{
    void (*drawChoices)(s32 selection, s32 y);
    s32 (*processInput)(s32 selection);
} static const sItemFunctionsGeneral[MENUITEM_GENERAL_COUNT] =
{
    [MENUITEM_GENERAL_TEXTSPEED]       = {DrawChoices_TextSpeed,       ProcessInput_Options_Three},
    [MENUITEM_GENERAL_BUTTONMODE]      = {DrawChoices_ButtonMode,      ProcessInput_Options_Three},
    [MENUITEM_GENERAL_SOUND]           = {DrawChoices_Sound,           ProcessInput_Options_Two},
    [MENUITEM_GENERAL_MOVEMENTMODE]    = {DrawChoices_MovementMode,    ProcessInput_Options_Two},
    [MENUITEM_GENERAL_MOVEANIMATIONS]  = {DrawChoices_MoveAnimations,  ProcessInput_Options_Two},
    [MENUITEM_GENERAL_ENTRYANIMATIONS] = {DrawChoices_EntryAnimations, ProcessInput_Options_Two},
    [MENUITEM_GENERAL_CLOCKFORMAT]     = {DrawChoices_ClockFormat,     ProcessInput_Options_Three},
    [MENUITEM_GENERAL_UNIT_SYSTEM]     = {DrawChoices_UnitSystem,      ProcessInput_Options_Two},
    [MENUITEM_GENERAL_FRAMETYPE]       = {DrawChoices_FrameType,       ProcessInput_FrameType},
    [MENUITEM_GENERAL_CANCEL]          = {NULL, NULL},
};

struct // MENU_DIFFICULTY
{
    void (*drawChoices)(s32 selection, s32 y);
    s32 (*processInput)(s32 selection);
} static const sItemFunctionsDifficulty[MENUITEM_DIFFICULTY_COUNT] =
{
    [MENUITEM_DIFFICULTY_EXPSHARE]          = {DrawChoices_ExpShare,        ProcessInput_Options_Two},
    [MENUITEM_DIFFICULTY_BATTLESTYLE]       = {DrawChoices_BattleStyle,     ProcessInput_Options_Two},
    [MENUITEM_DIFFICULTY_BATTLEITEMS]       = {DrawChoices_BattleItems,     ProcessInput_Options_Four},
    [MENUITEM_DIFFICULTY_LEVELSCALING]      = {DrawChoices_LevelScaling,    ProcessInput_Options_Two},
    [MENUITEM_DIFFICULTY_ENEMYAI]           = {DrawChoices_EnemyAI,         ProcessInput_Options_Two},
    [MENUITEM_DIFFICULTY_CANCEL]            = {NULL, NULL},
};

//------------------------------------------------------------------
// Menu left side option names text
//------------------------------------------------------------------
static const u8 *const sOptionMenuItemsNamesGeneral[MENUITEM_GENERAL_COUNT] =
{
    [MENUITEM_GENERAL_TEXTSPEED]         = gText_TextSpeed,
    [MENUITEM_GENERAL_BUTTONMODE]        = gText_ButtonMode,
    [MENUITEM_GENERAL_SOUND]             = gText_Sound,
    [MENUITEM_GENERAL_MOVEMENTMODE]      = gText_MovementMode,
    [MENUITEM_GENERAL_MOVEANIMATIONS]    = gText_MoveAnimations,
    [MENUITEM_GENERAL_ENTRYANIMATIONS]   = gText_EntryAnimations,
    [MENUITEM_GENERAL_CLOCKFORMAT]       = gText_ClockFormat,
    [MENUITEM_GENERAL_UNIT_SYSTEM]       = gText_UnitSystem,
    [MENUITEM_GENERAL_FRAMETYPE]         = gText_Frame,
    [MENUITEM_GENERAL_CANCEL]            = gText_OptionMenuSave,
};

static const u8 *const sOptionMenuItemsNamesDifficulty[MENUITEM_DIFFICULTY_COUNT] =
{
    [MENUITEM_DIFFICULTY_EXPSHARE]       = gText_ExpShare,
    [MENUITEM_DIFFICULTY_BATTLESTYLE]    = gText_BattleStyle,
    [MENUITEM_DIFFICULTY_BATTLEITEMS]    = gText_BattleItems,
    [MENUITEM_DIFFICULTY_LEVELSCALING]   = gText_LevelScaling,
    [MENUITEM_DIFFICULTY_ENEMYAI]        = gText_EnemyAI,
    [MENUITEM_DIFFICULTY_CANCEL]         = gText_OptionMenuSave,
};

static const u8 *const OptionTextRight(u8 menuItem)
{
    switch (sOptions->submenu)
    {
    case MENU_GENERAL:     return sOptionMenuItemsNamesGeneral[menuItem];
    case MENU_DIFFICULTY:  return sOptionMenuItemsNamesDifficulty[menuItem];
    }
}
//------------------------------------------------------------------
// Menu left side text conditions
//------------------------------------------------------------------
static bool8 CheckConditions(s32 selection)
{
    switch (sOptions->submenu)
    {
    case MENU_GENERAL:
        switch(selection)
        {
        case MENUITEM_GENERAL_TEXTSPEED:         return TRUE;
        case MENUITEM_GENERAL_BUTTONMODE:        return TRUE;
        case MENUITEM_GENERAL_SOUND:             return TRUE;
        case MENUITEM_GENERAL_MOVEMENTMODE:      return TRUE;
        case MENUITEM_GENERAL_MOVEANIMATIONS:    return TRUE;
        case MENUITEM_GENERAL_ENTRYANIMATIONS:   return TRUE;
        case MENUITEM_GENERAL_CLOCKFORMAT:       return TRUE;
        case MENUITEM_GENERAL_UNIT_SYSTEM:       return TRUE;
        case MENUITEM_GENERAL_FRAMETYPE:         return TRUE;
        case MENUITEM_GENERAL_CANCEL:            return TRUE;
        case MENUITEM_GENERAL_COUNT:             return TRUE;
        }
    case MENU_DIFFICULTY:
        switch(selection)
        {
        case MENUITEM_DIFFICULTY_EXPSHARE:      return TRUE;
        case MENUITEM_DIFFICULTY_BATTLESTYLE:   return TRUE;
        case MENUITEM_DIFFICULTY_BATTLEITEMS:   return TRUE;
        case MENUITEM_DIFFICULTY_LEVELSCALING:  return TRUE;
        case MENUITEM_DIFFICULTY_ENEMYAI:       return TRUE;
        case MENUITEM_DIFFICULTY_CANCEL:        return TRUE;
        case MENUITEM_DIFFICULTY_COUNT:         return TRUE;
        }
    }
}

//------------------------------------------------------------------
// General Descriptions
//------------------------------------------------------------------
static const u8 sText_Empty[]                    = _("");
static const u8 sText_Desc_TextSpeed[]           = _("Choose one of the three text-display\nspeeds.");
static const u8 sText_Desc_ButtonMode_Normal[]   = _("All buttons work as normal.");
static const u8 sText_Desc_ButtonMode_LR[]       = _("On some screens the L and R buttons\nact as left and right.");
static const u8 sText_Desc_ButtonMode_LA[]       = _("The L button acts as another A\nbutton for one-handed play.");
static const u8 sText_Desc_SoundMono[]           = _("Sound is the same in all speakers.\nRecommended for original hardware.");
static const u8 sText_Desc_SoundStereo[]         = _("Play the left and right audio channel\nseperately. Great with headphones.");
static const u8 sText_Desc_MovementMode_Walk[]   = _("Walking is the default movement.\nHold the B button to run.");
static const u8 sText_Desc_MovementMode_Run[]    = _("Running is the default movement.\nHold the B button to walk.");
static const u8 sText_Desc_MoveAnimations_On[]   = _("Show the Pokémon move animations\nduring battle.");
static const u8 sText_Desc_MoveAnimations_Off[]  = _("Skip the Pokémon move animations\nduring battle.");
static const u8 sText_Desc_EntryAnimations_On[]  = _("Show the Pokémon entry animations.");
static const u8 sText_Desc_EntryAnimations_Off[] = _("Skip the Pokémon entry animations.");
static const u8 sText_Desc_ClockFormat_12hr[]    = _("Start Menu displays the current\ntime in 12-hour format.");
static const u8 sText_Desc_ClockFormat_24hr[]    = _("Start Menu displays the current\ntime in 24-hour format.");
static const u8 sText_Desc_ClockFormat_Off[]     = _("Start Menu does not display the\ncurrent time.");
static const u8 sText_Desc_UnitSystemImperial[]  = _("Display Berry and Pokémon weight\nand size in pounds and inches.");
static const u8 sText_Desc_UnitSystemMetric[]    = _("Display Berry and Pokémon weight\nand size in kilograms and meters.");
static const u8 sText_Desc_FrameType[]           = _("Choose the frame surrounding the\nwindows. Press B to cycle backwards.");
static const u8 sText_Desc_Save[]                = _("Save your settings.");

static const u8 *const sOptionMenuItemDescriptionsGeneral[MENUITEM_GENERAL_COUNT][3] =
{
    [MENUITEM_GENERAL_TEXTSPEED]       = {sText_Desc_TextSpeed,           sText_Empty,                    sText_Empty},
    [MENUITEM_GENERAL_BUTTONMODE]      = {sText_Desc_ButtonMode_Normal,   sText_Desc_ButtonMode_LR,       sText_Desc_ButtonMode_LA},
    [MENUITEM_GENERAL_SOUND]           = {sText_Desc_SoundMono,           sText_Desc_SoundStereo,         sText_Empty},
    [MENUITEM_GENERAL_MOVEMENTMODE]    = {sText_Desc_MovementMode_Walk,   sText_Desc_MovementMode_Run,    sText_Empty},
    [MENUITEM_GENERAL_MOVEANIMATIONS]  = {sText_Desc_MoveAnimations_On,   sText_Desc_MoveAnimations_Off,  sText_Empty},
    [MENUITEM_GENERAL_ENTRYANIMATIONS] = {sText_Desc_EntryAnimations_On,  sText_Desc_EntryAnimations_Off, sText_Empty},
    [MENUITEM_GENERAL_CLOCKFORMAT]     = {sText_Desc_ClockFormat_Off,     sText_Desc_ClockFormat_12hr,    sText_Desc_ClockFormat_24hr},
    [MENUITEM_GENERAL_UNIT_SYSTEM]     = {sText_Desc_UnitSystemImperial,  sText_Desc_UnitSystemMetric,    sText_Empty},
    [MENUITEM_GENERAL_FRAMETYPE]       = {sText_Desc_FrameType,           sText_Empty,                    sText_Empty},
    [MENUITEM_GENERAL_CANCEL]          = {sText_Desc_Save,                sText_Empty,                    sText_Empty},
};

//------------------------------------------------------------------
// Difficulty Descriptions
//------------------------------------------------------------------
static const u8 sText_Desc_ExpShare_Off[]          = _("Your Pokémon must battle to receive\nexperience.");
static const u8 sText_Desc_ExpShare_On[]           = _("Your Pokémon share experience with\nparty members who didn't battle.");
static const u8 sText_Desc_BattleStyle_Shift[]     = _("You are allowed to switch your\nPokémon after the foe faints.");
static const u8 sText_Desc_BattleStyle_Set[]       = _("No free switch after fainting a foe\nin battle.");
static const u8 sText_Desc_BattleItems[]           = _("Who can use items from the BAG during\ntrainer battles.");
static const u8 sText_Desc_LevelScaling_Off[]      = _("Foe trainers' Pokémon have their\nintended level.");
static const u8 sText_Desc_LevelScaling_On[]       = _("Foe trainers' Pokémon have their\nlevel raised to better match yours.");
static const u8 sText_Desc_EnemyAI_Normal[]        = _("Foes have their default AI.");
static const u8 sText_Desc_EnemyAI_Smart[]         = _("Foes have smarter AI and make better\ndecisions.");

static const u8 *const sOptionMenuItemDescriptionsDifficulty[MENUITEM_DIFFICULTY_COUNT][2] =
{
    [MENUITEM_DIFFICULTY_EXPSHARE]        = {sText_Desc_ExpShare_Off,             sText_Desc_ExpShare_On},
    [MENUITEM_DIFFICULTY_BATTLESTYLE]     = {sText_Desc_BattleStyle_Shift,       sText_Desc_BattleStyle_Set},
    [MENUITEM_DIFFICULTY_BATTLEITEMS]     = {sText_Desc_BattleItems,             sText_Empty},
    [MENUITEM_DIFFICULTY_LEVELSCALING]    = {sText_Desc_LevelScaling_Off,        sText_Desc_LevelScaling_On},
    [MENUITEM_DIFFICULTY_ENEMYAI]         = {sText_Desc_EnemyAI_Normal,          sText_Desc_EnemyAI_Smart},
    [MENUITEM_DIFFICULTY_CANCEL]          = {sText_Desc_Save,                    sText_Empty},
};

static const u8 *const OptionTextDescription(void)
{
    u8 menuItem = sOptions->menuCursor[sOptions->submenu];
    u8 selection;

    switch (sOptions->submenu)
    {
    case MENU_GENERAL:
        selection = sOptions->sel_general[menuItem];
        if (menuItem == MENUITEM_GENERAL_TEXTSPEED || menuItem == MENUITEM_GENERAL_FRAMETYPE)
            selection = 0;
        return sOptionMenuItemDescriptionsGeneral[menuItem][selection];
    case MENU_DIFFICULTY:
        selection = sOptions->sel_difficulty[menuItem];
        if (menuItem == MENUITEM_DIFFICULTY_BATTLEITEMS)
            selection = 0;
        return sOptionMenuItemDescriptionsDifficulty[menuItem][selection];
    }
}

static u8 MenuItemCount(void)
{
    switch (sOptions->submenu)
    {
    case MENU_GENERAL:   return MENUITEM_GENERAL_COUNT;
    case MENU_DIFFICULTY:    return MENUITEM_DIFFICULTY_COUNT;
    }
}

static u8 MenuItemCancel(void)
{
    switch (sOptions->submenu)
    {
    case MENU_GENERAL:   return MENUITEM_GENERAL_CANCEL;
    case MENU_DIFFICULTY:    return MENUITEM_DIFFICULTY_CANCEL;
    }
}

//------------------------------------------------------------------
// Main code
//------------------------------------------------------------------
static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static const u8 sText_TopBar_Controls[]        = _("{DPAD_LEFTRIGHT}PAGE {A_BUTTON}SWITCH");
static const u8 sText_TopBar_General[]         = _("General Options");
static const u8 sText_TopBar_Difficulty[]      = _("Difficulty Options");

static void DrawTopBarText(void)
{
    const u8 color[3] = {COLOR_TRANSPARENT, COLOR_WHITE, COLOR_GRAY_SHADOW};
    s32 positionWhenScreenCentered = 120;
    s32 textDistanceFromRight = 240 - GetStringWidth(FONT_SMALL, sText_TopBar_Controls, 0) - 5;

    FillWindowPixelBuffer(WIN_TOPBAR, PIXEL_FILL(COLOR_BLUE_SHADOW));
    switch (sOptions->submenu)
    {
        case MENU_GENERAL:
            AddTextPrinterParameterized3(WIN_TOPBAR, FONT_SMALL, 1, 1, color, 0, sText_TopBar_General);
            break;
        case MENU_DIFFICULTY:
            AddTextPrinterParameterized3(WIN_TOPBAR, FONT_SMALL, 1, 1, color, 0, sText_TopBar_Difficulty);
            break;
    }
    AddTextPrinterParameterized3(WIN_TOPBAR, FONT_SMALL, textDistanceFromRight, 1, color, 0, sText_TopBar_Controls);
    
    PutWindowTilemap(WIN_TOPBAR);
    CopyWindowToVram(WIN_TOPBAR, COPYWIN_FULL);
}

static void DrawOptionMenuTexts(void) //left side text
{
    u8 i;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < MenuItemCount(); i++)
        DrawLeftSideOptionText(i, (i * Y_DIFF) + 1);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

static void DrawDescriptionText(void)
{
    u8 color_gray[3];
    color_gray[0] = COLOR_TRANSPARENT;
    color_gray[1] = COLOR_BLACK;
    color_gray[2] = COLOR_GRAY_FG;
        
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    AddTextPrinterParameterized4(WIN_DESCRIPTION, FONT_NORMAL, 8, 1, 0, 0, color_gray, TEXT_SKIP_DRAW, OptionTextDescription());
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_FULL);
}

static void DrawLeftSideOptionText(s32 selection, s32 y)
{
    u8 color_yellow[3];
    u8 color_gray[3];

    color_yellow[0] = COLOR_TRANSPARENT;
    color_yellow[1] = COLOR_ORANGE_FG;
    color_yellow[2] = COLOR_ORANGE_SHADOW;
    color_gray[0] = COLOR_TRANSPARENT;
    color_gray[1] = COLOR_GRAY_FG;
    color_gray[2] = COLOR_GRAY_SHADOW;

    if (CheckConditions(selection))
        AddTextPrinterParameterized4(WIN_OPTIONS, FONT_NORMAL, 8, y, 0, 0, color_yellow, TEXT_SKIP_DRAW, OptionTextRight(selection));
    else
        AddTextPrinterParameterized4(WIN_OPTIONS, FONT_NORMAL, 8, y, 0, 0, color_gray, TEXT_SKIP_DRAW, OptionTextRight(selection));
}

static void DrawRightSideChoiceText(const u8 *text, s32 x, s32 y, bool8 choosen, bool8 active)
{
    u8 color_red[3];
    u8 color_gray[3];

    if (active)
    {
        color_red[0]  = COLOR_TRANSPARENT;
        color_red[1]  = COLOR_RED_FG;
        color_red[2]  = COLOR_RED_SHADOW;
        color_gray[0] = COLOR_TRANSPARENT;
        color_gray[1] = COLOR_BLACK;
        color_gray[2] = COLOR_GRAY_FG;
    }
    else
    {
        color_red[0]  = COLOR_TRANSPARENT;
        color_red[1]  = COLOR_RED_FG;
        color_red[2]  = COLOR_RED_SHADOW;
        color_gray[0] = COLOR_TRANSPARENT;
        color_gray[1] = COLOR_BLACK;
        color_gray[2] = COLOR_GRAY_FG;
    }


    if (choosen)
        AddTextPrinterParameterized4(WIN_OPTIONS, FONT_NORMAL, x, y, 0, 0, color_red, TEXT_SKIP_DRAW, text);
    else
        AddTextPrinterParameterized4(WIN_OPTIONS, FONT_NORMAL, x, y, 0, 0, color_gray, TEXT_SKIP_DRAW, text);
}

static void DrawChoices(u32 id, s32 y) //right side draw function
{
    switch (sOptions->submenu)
    {
        case MENU_GENERAL:
            if (sItemFunctionsGeneral[id].drawChoices != NULL)
                sItemFunctionsGeneral[id].drawChoices(sOptions->sel_general[id], y);
            break;
        case MENU_DIFFICULTY:
            if (sItemFunctionsDifficulty[id].drawChoices != NULL)
                sItemFunctionsDifficulty[id].drawChoices(sOptions->sel_difficulty[id], y);
            break;
    }
}

static void HighlightOptionMenuItem(void)
{
    s32 cursor = sOptions->visibleCursor[sOptions->submenu];

    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(Y_DIFF, 224));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(cursor * Y_DIFF + 24, cursor * Y_DIFF + 40));
}

void CB2_InitOptionMenu(void)
{
    u32 i, taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sOptionMenuBgTemplates, ARRAY_COUNT(sOptionMenuBgTemplates));
        ResetBgPositions();
        InitWindows(sOptionMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN1_BG0 | WININ_WIN0_OBJ);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_DARKEN | BLDCNT_TGT1_BG0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sOptionMenuBg_Pal, 0, sizeof(sOptionMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, 0x70, 0x20);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sOptionMenuText_Pal, 16, sizeof(sOptionMenuText_Pal));
        gMain.state++;
        break;
    case 6:
        sOptions = AllocZeroed(sizeof(*sOptions));
        sOptions->sel_general[MENUITEM_GENERAL_TEXTSPEED]       = gSaveBlock2Ptr->optionsTextSpeed;
        sOptions->sel_general[MENUITEM_GENERAL_BUTTONMODE]      = gSaveBlock2Ptr->optionsButtonMode;
        sOptions->sel_general[MENUITEM_GENERAL_SOUND]           = gSaveBlock2Ptr->optionsSound;
        sOptions->sel_general[MENUITEM_GENERAL_MOVEMENTMODE]    = gSaveBlock2Ptr->optionsMovementMode;
        sOptions->sel_general[MENUITEM_GENERAL_MOVEANIMATIONS]  = gSaveBlock2Ptr->optionsMoveAnimationsOff;
        sOptions->sel_general[MENUITEM_GENERAL_ENTRYANIMATIONS] = gSaveBlock2Ptr->optionsEntryAnimationsOff;
        sOptions->sel_general[MENUITEM_GENERAL_CLOCKFORMAT]     = gSaveBlock2Ptr->optionsClockFormat;
        sOptions->sel_general[MENUITEM_GENERAL_UNIT_SYSTEM]     = gSaveBlock2Ptr->optionsUnitSystem;
        sOptions->sel_general[MENUITEM_GENERAL_FRAMETYPE]       = gSaveBlock2Ptr->optionsWindowFrameType;

        sOptions->sel_difficulty[MENUITEM_DIFFICULTY_EXPSHARE]      = FlagGet(I_EXP_SHARE_FLAG);
        sOptions->sel_difficulty[MENUITEM_DIFFICULTY_BATTLESTYLE]   = gSaveBlock2Ptr->optionsBattleStyle;
        sOptions->sel_difficulty[MENUITEM_DIFFICULTY_BATTLEITEMS]   = gSaveBlock2Ptr->optionsBattleItems;
        sOptions->sel_difficulty[MENUITEM_DIFFICULTY_LEVELSCALING]  = gSaveBlock2Ptr->optionsLevelScaling;
        sOptions->sel_difficulty[MENUITEM_DIFFICULTY_ENEMYAI]       = gSaveBlock2Ptr->optionsEnemyAI;

        sOptions->submenu = MENU_GENERAL;

        gMain.state++;
        break;
    case 7:
        PutWindowTilemap(WIN_TOPBAR);
        DrawTopBarText();
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_DESCRIPTION);
        DrawDescriptionText();
        gMain.state++;
        break;
    case 9:
        PutWindowTilemap(WIN_OPTIONS);
        DrawOptionMenuTexts();
        gMain.state++;
        break;
    case 10:
        taskId = CreateTask(Task_OptionMenuFadeIn, 0);
        
        sOptions->arrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 240 / 2, 20, 110, MENUITEM_GENERAL_COUNT - 1, 110, 110, 0);

        for (i = 0; i < min(OPTIONS_ON_SCREEN, MenuItemCount()); i++)
            DrawChoices(i, i * Y_DIFF);

        HighlightOptionMenuItem();

        CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
        gMain.state++;
        break;
    case 11:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 12:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_OptionMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput;
}

static void Task_OptionMenuProcessInput(u8 taskId)
{
    s32 i = 0;
    u8 optionsToDraw = min(OPTIONS_ON_SCREEN , MenuItemCount());

    if (JOY_NEW(START_BUTTON) || (JOY_NEW(B_BUTTON) && sOptions->menuCursor[sOptions->submenu] != MENUITEM_GENERAL_FRAMETYPE))
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sOptions->visibleCursor[sOptions->submenu] == NUM_OPTIONS_FROM_BORDER) // don't advance visible cursor until scrolled to the bottom
        {
            if (--sOptions->menuCursor[sOptions->submenu] == 0)
                sOptions->visibleCursor[sOptions->submenu]--;
            else
                ScrollMenu(SCROLL_DIR_UP);
        }
        else
        {
            if (--sOptions->menuCursor[sOptions->submenu] < 0) // Scroll all the way to the bottom.
            {
                sOptions->visibleCursor[sOptions->submenu] = sOptions->menuCursor[sOptions->submenu] = optionsToDraw - 2;
                ScrollAll(SCROLL_DIR_DOWN);
                sOptions->visibleCursor[sOptions->submenu] = optionsToDraw - 1;
                sOptions->menuCursor[sOptions->submenu] = MenuItemCount() - 1;
            }
            else
            {
                sOptions->visibleCursor[sOptions->submenu]--;
            }
        }
        HighlightOptionMenuItem();
        DrawDescriptionText();
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (sOptions->visibleCursor[sOptions->submenu] == optionsToDraw - 2) // don't advance visible cursor until scrolled to the bottom
        {
            if (++sOptions->menuCursor[sOptions->submenu] == MenuItemCount() - 1)
                sOptions->visibleCursor[sOptions->submenu]++;
            else
                ScrollMenu(SCROLL_DIR_DOWN);
        }
        else
        {
            if (++sOptions->menuCursor[sOptions->submenu] >= MenuItemCount() - 1) // Scroll all the way to the top.
            {
                sOptions->visibleCursor[sOptions->submenu] = optionsToDraw - 2;
                sOptions->menuCursor[sOptions->submenu] = MenuItemCount() - optionsToDraw - 1;
                ScrollAll(SCROLL_DIR_UP);
                sOptions->visibleCursor[sOptions->submenu] = sOptions->menuCursor[sOptions->submenu] = 0;
            }
            else
            {
                sOptions->visibleCursor[sOptions->submenu]++;
            }
        }
        HighlightOptionMenuItem();
        DrawDescriptionText();
    }
    else if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        if (sOptions->menuCursor[sOptions->submenu] == MenuItemCancel())
            gTasks[taskId].func = Task_OptionMenuSave;

        if (sOptions->submenu == MENU_GENERAL)
        {
            s32 cursor = sOptions->menuCursor[sOptions->submenu];
            u8 previousOption = sOptions->sel_general[cursor];
            if (CheckConditions(cursor))
            {
                if (sItemFunctionsGeneral[cursor].processInput != NULL)
                {
                    sOptions->sel_general[cursor] = sItemFunctionsGeneral[cursor].processInput(previousOption);
                    ReDrawAll();
                    DrawDescriptionText();
                }

                if (previousOption != sOptions->sel_general[cursor])
                    DrawChoices(cursor, sOptions->visibleCursor[sOptions->submenu] * Y_DIFF);
            }
        }
        else if (sOptions->submenu == MENU_DIFFICULTY)
        {
            s32 cursor = sOptions->menuCursor[sOptions->submenu];
            u8 previousOption = sOptions->sel_difficulty[cursor];
            if (CheckConditions(cursor))
            {
                if (sItemFunctionsDifficulty[cursor].processInput != NULL)
                {
                    sOptions->sel_difficulty[cursor] = sItemFunctionsDifficulty[cursor].processInput(previousOption);
                    ReDrawAll();
                    DrawDescriptionText();
                }

                if (previousOption != sOptions->sel_difficulty[cursor])
                    DrawChoices(cursor, sOptions->visibleCursor[sOptions->submenu] * Y_DIFF);
            }
        }
    }
    else if (JOY_NEW(DPAD_RIGHT) || (JOY_NEW(R_BUTTON)
    && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR))
    {
        if (sOptions->submenu < MENU_COUNT - 1)
            sOptions->submenu++;

        DrawTopBarText();
        ReDrawAll();
        HighlightOptionMenuItem();
        DrawDescriptionText();
    }
    else if (JOY_NEW(DPAD_LEFT) || (JOY_NEW(L_BUTTON)
    && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR))
    {
        if (sOptions->submenu > 0)
            sOptions->submenu--;
        
        DrawTopBarText();
        ReDrawAll();
        HighlightOptionMenuItem();
        DrawDescriptionText();
    }
}

static void Task_OptionMenuSave(u8 taskId)
{
    gSaveBlock2Ptr->optionsTextSpeed          = sOptions->sel_general[MENUITEM_GENERAL_TEXTSPEED];
    gSaveBlock2Ptr->optionsButtonMode         = sOptions->sel_general[MENUITEM_GENERAL_BUTTONMODE];
    gSaveBlock2Ptr->optionsSound              = sOptions->sel_general[MENUITEM_GENERAL_SOUND];
    gSaveBlock2Ptr->optionsMovementMode       = sOptions->sel_general[MENUITEM_GENERAL_MOVEMENTMODE];
    gSaveBlock2Ptr->optionsMoveAnimationsOff  = sOptions->sel_general[MENUITEM_GENERAL_MOVEANIMATIONS];
    gSaveBlock2Ptr->optionsEntryAnimationsOff = sOptions->sel_general[MENUITEM_GENERAL_ENTRYANIMATIONS];
    gSaveBlock2Ptr->optionsClockFormat        = sOptions->sel_general[MENUITEM_GENERAL_CLOCKFORMAT];
    gSaveBlock2Ptr->optionsUnitSystem         = sOptions->sel_general[MENUITEM_GENERAL_UNIT_SYSTEM];
    gSaveBlock2Ptr->optionsWindowFrameType    = sOptions->sel_general[MENUITEM_GENERAL_FRAMETYPE];

    sOptions->sel_difficulty[MENUITEM_DIFFICULTY_EXPSHARE] ? FlagSet(I_EXP_SHARE_FLAG) : FlagClear(I_EXP_SHARE_FLAG);
    gSaveBlock2Ptr->optionsBattleStyle        = sOptions->sel_difficulty[MENUITEM_DIFFICULTY_BATTLESTYLE];
    gSaveBlock2Ptr->optionsBattleItems        = sOptions->sel_difficulty[MENUITEM_DIFFICULTY_BATTLEITEMS];
    gSaveBlock2Ptr->optionsLevelScaling       = sOptions->sel_difficulty[MENUITEM_DIFFICULTY_LEVELSCALING];
    gSaveBlock2Ptr->optionsEnemyAI            = sOptions->sel_difficulty[MENUITEM_DIFFICULTY_ENEMYAI];

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOut;
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        FREE_AND_SET_NULL(sOptions);
        SetMainCallback2(gMain.savedCallback);
    }
}

static void ScrollMenu(s32 direction)
{
    s32 menuItem, pos;
    u8 optionsToDraw = min(OPTIONS_ON_SCREEN, MenuItemCount());

    if (direction == SCROLL_DIR_DOWN)
        menuItem = sOptions->menuCursor[sOptions->submenu] + NUM_OPTIONS_FROM_BORDER, pos = optionsToDraw - 1;
    else
        menuItem = sOptions->menuCursor[sOptions->submenu] - NUM_OPTIONS_FROM_BORDER, pos = 0;

    // Hide one
    ScrollWindow(WIN_OPTIONS, direction, Y_DIFF, PIXEL_FILL(0));
    // Show one
    FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), 0, Y_DIFF * pos, 26 * 8, Y_DIFF);
    // Print
    DrawChoices(menuItem, pos * Y_DIFF);
    DrawLeftSideOptionText(menuItem, (pos * Y_DIFF) + 1);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
}

static void ScrollAll(s32 direction) // to bottom or top
{
    s32 i, y, menuItem, pos;
    s32 scrollCount;
    u8 optionsToDraw = min(OPTIONS_ON_SCREEN, MenuItemCount());

    scrollCount = MenuItemCount() - optionsToDraw;

    // Move items up/down
    ScrollWindow(WIN_OPTIONS, direction, Y_DIFF * scrollCount, PIXEL_FILL(1));

    // Clear moved items
    if (direction == SCROLL_DIR_DOWN)
    {
        y = optionsToDraw - scrollCount;
        if (y < 0)
            y = optionsToDraw;
        y *= Y_DIFF;
    }
    else
    {
        y = 0;
    }

    FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), 0, y, 26 * 8, Y_DIFF * scrollCount);
    // Print new texts
    for (i = 0; i < scrollCount; i++)
    {
        if (direction == SCROLL_DIR_DOWN) // From top to bottom
            menuItem = MenuItemCount() - 1 - i, pos = optionsToDraw - 1 - i;
        else // From bottom to top
            menuItem = i, pos = i;
        DrawChoices(menuItem, pos * Y_DIFF);
        DrawLeftSideOptionText(menuItem, (pos * Y_DIFF) + 1);
    }
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
}

// Process Input functions ****GENERIC****
static s32 GetMiddleX(const u8 *txt1, const u8 *txt2, const u8 *txt3)
{
    s32 xMid;
    s32 widthLeft = GetStringWidth(1, txt1, 0);
    s32 widthMid = GetStringWidth(1, txt2, 0);
    s32 widthRight = GetStringWidth(1, txt3, 0);

    widthMid -= (198 - 104);
    xMid = (widthLeft - widthMid - widthRight) / 2 + 104;
    return xMid;
}

static s32 XOptions_ProcessInput(s32 x, s32 selection)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (++selection > (x - 1))
            selection = 0;
    }
    return selection;
}

static s32 ProcessInput_Options_Two(s32 selection)
{
    if (JOY_NEW(A_BUTTON))
        selection ^= 1;

    return selection;
}

static s32 ProcessInput_Options_Three(s32 selection)
{
    return XOptions_ProcessInput(3, selection);
}

static s32 ProcessInput_Options_Four(s32 selection)
{
    return XOptions_ProcessInput(4, selection);
}

// Process Input functions ****SPECIFIC****
static s32 ProcessInput_Sound(s32 selection)
{
    if (JOY_NEW(A_BUTTON))
    {
        selection ^= 1;
        SetPokemonCryStereo(selection);
    }

    return selection;
}

static s32 ProcessInput_FrameType(s32 selection)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (selection < WINDOW_FRAMES_COUNT - 1)
            selection++;
        else
            selection = 0;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, 0x70, 0x20);
    }
    if (JOY_NEW(B_BUTTON))
    {
        if (selection != 0)
            selection--;
        else
            selection = WINDOW_FRAMES_COUNT - 1;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, 0x70, 0x20);
    }
    return selection;
}

// Draw Choices functions ****GENERIC****
static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style, bool8 active)
{
    bool8 choosen = FALSE;
    if (style != 0)
        choosen = TRUE;

    DrawRightSideChoiceText(text, x, y+1, choosen, active);
}

static void DrawChoices_Options_Four(const u8 *const *const strings, s32 selection, s32 y, bool8 active)
{
    static const u8 choiceOrders[][3] =
    {
        {0, 1, 2},
        {0, 1, 2},
        {1, 2, 3},
        {1, 2, 3},
    };
    u8 styles[4] = {0};
    s32 xMid;
    const u8 *order = choiceOrders[selection];

    styles[selection] = 1;
    xMid = GetMiddleX(strings[order[0]], strings[order[1]], strings[order[2]]);

    DrawOptionMenuChoice(strings[order[0]], 104, y, styles[order[0]], active);
    DrawOptionMenuChoice(strings[order[1]], xMid, y, styles[order[1]], active);
    DrawOptionMenuChoice(strings[order[2]], GetStringRightAlignXOffset(1, strings[order[2]], 198), y, styles[order[2]], active);
}

static void ReDrawAll(void)
{
    u8 menuItem = sOptions->menuCursor[sOptions->submenu] - sOptions->visibleCursor[sOptions->submenu];
    u8 i;
    u8 optionsToDraw = min(OPTIONS_ON_SCREEN, MenuItemCount());

    if (MenuItemCount() <= OPTIONS_ON_SCREEN) // Draw or delete the scrolling arrows based on options in the menu
    {
        if (sOptions->arrowTaskId != TASK_NONE)
        {
            RemoveScrollIndicatorArrowPair(sOptions->arrowTaskId);
            sOptions->arrowTaskId = TASK_NONE;
        }
    }
    else
    {
        if (sOptions->arrowTaskId == TASK_NONE)
            sOptions->arrowTaskId = sOptions->arrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 240 / 2, 20, 110, MenuItemCount() - 1, 110, 110, 0);

    }

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < optionsToDraw; i++)
    {
        DrawChoices(menuItem+i, i * Y_DIFF);
        DrawLeftSideOptionText(menuItem+i, (i * Y_DIFF) + 1);
    }
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
}

// Process Input functions ****SPECIFIC****
static void DrawChoices_TextSpeed(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_TEXTSPEED);
    u8 styles[3] = {0};
    s32 xMid = GetMiddleX(gText_TextSpeedSlow, gText_TextSpeedMid, gText_TextSpeedFast);
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_TextSpeedSlow, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_TextSpeedMid, xMid, y, styles[1], active);
    DrawOptionMenuChoice(gText_TextSpeedFast, GetStringRightAlignXOffset(1, gText_TextSpeedFast, 198), y, styles[2], active);
}

static void DrawChoices_ButtonMode(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_BUTTONMODE);
    u8 styles[3] = {0};
    s32 xMid = GetMiddleX(gText_ButtonTypeNormal, gText_ButtonTypeLR, gText_ButtonTypeLEqualsA);
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_ButtonTypeNormal, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_ButtonTypeLR, xMid, y, styles[1], active);
    DrawOptionMenuChoice(gText_ButtonTypeLEqualsA, GetStringRightAlignXOffset(1, gText_ButtonTypeLEqualsA, 198), y, styles[2], active);
}

static void DrawChoices_Sound(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_SOUND);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_SoundMono, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_SoundStereo, GetStringRightAlignXOffset(FONT_NORMAL, gText_SoundStereo, 198), y, styles[1], active);
}

static void DrawChoices_MovementMode(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_MOVEMENTMODE);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_MovementModeWalk, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_MovementModeRun, GetStringRightAlignXOffset(FONT_NORMAL, gText_MovementModeRun, 198), y, styles[1], active);
}

static void DrawChoices_MoveAnimations(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_MOVEANIMATIONS);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_OptionOn, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_OptionOff, GetStringRightAlignXOffset(FONT_NORMAL, gText_OptionOff, 198), y, styles[1], active);
}

static void DrawChoices_EntryAnimations(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_ENTRYANIMATIONS);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_OptionOn, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_OptionOff, GetStringRightAlignXOffset(FONT_NORMAL, gText_OptionOff, 198), y, styles[1], active);
}

static void DrawChoices_ClockFormat(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_CLOCKFORMAT);
    u8 styles[3] = {0};
    s32 xMid = GetMiddleX(gText_OptionOff, gText_ClockFormat12Hour, gText_ClockFormat24Hour);
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_OptionOff, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_ClockFormat12Hour, xMid, y, styles[1], active);
    DrawOptionMenuChoice(gText_ClockFormat24Hour, GetStringRightAlignXOffset(1, gText_ClockFormat24Hour, 198), y, styles[2], active);
}

static void DrawChoices_UnitSystem(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_UNIT_SYSTEM);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_UnitSystemImperial, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_UnitSystemMetric, GetStringRightAlignXOffset(1, gText_UnitSystemMetric, 198), y, styles[1], active);
}

static void DrawChoices_FrameType(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_GENERAL_FRAMETYPE);
    u8 text[16];
    u8 n = selection + 1;
    u16 i;

    for (i = 0; gText_FrameTypeNumber[i] != EOS && i <= 5; i++)
        text[i] = gText_FrameTypeNumber[i];

    // Convert a number to decimal string
    if (n / 10 != 0)
    {
        text[i] = n / 10 + CHAR_0;
        i++;
        text[i] = n % 10 + CHAR_0;
        i++;
    }
    else
    {
        text[i] = n % 10 + CHAR_0;
        i++;
        text[i] = 0x77;
        i++;
    }

    text[i] = EOS;

    DrawOptionMenuChoice(gText_FrameType, 104, y, 0, active);
    DrawOptionMenuChoice(text, 128, y, 1, active);
}

static void DrawChoices_ExpShare(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_DIFFICULTY_EXPSHARE);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_OptionOff, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_OptionOn, GetStringRightAlignXOffset(FONT_NORMAL, gText_OptionOn, 198), y, styles[1], active);
}

static void DrawChoices_BattleStyle(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_DIFFICULTY_BATTLESTYLE);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_BattleStyleShift, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_BattleStyleSet, GetStringRightAlignXOffset(FONT_NORMAL, gText_BattleStyleSet, 198), y, styles[1], active);
}

static void DrawChoices_BattleItems(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_DIFFICULTY_BATTLEITEMS);
    u8 n = selection;

    switch (n)
    {
    case OPTIONS_BATTLE_ITEMS_EVERYBODY:
        StringCopyPadded(gStringVar1, gText_BattleItemsEverybody, 0, 15);
        break;
    case OPTIONS_BATTLE_ITEMS_PLAYER_ONLY:
        StringCopyPadded(gStringVar1, gText_BattleItemsPlayerOnly, 0, 15);
        break;
    case OPTIONS_BATTLE_ITEMS_FOE_ONLY:
        StringCopyPadded(gStringVar1, gText_BattleItemsFoeOnly, 0, 15);
        break;
    case OPTIONS_BATTLE_ITEMS_NOBODY:
        StringCopyPadded(gStringVar1, gText_BattleItemsNobody, 0, 15);
        break;
    }

    DrawOptionMenuChoice(gStringVar1, 104, y, 1, active);
}

static void DrawChoices_LevelScaling(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_DIFFICULTY_LEVELSCALING);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_OptionOff, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_OptionOn, GetStringRightAlignXOffset(FONT_NORMAL, gText_OptionOn, 198), y, styles[1], active);
}

static void DrawChoices_EnemyAI(s32 selection, s32 y)
{
    bool8 active = CheckConditions(MENUITEM_DIFFICULTY_ENEMYAI);
    u8 styles[2] = {0};
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_EnemyAINormal, 104, y, styles[0], active);
    DrawOptionMenuChoice(gText_EnemyAISmart, GetStringRightAlignXOffset(FONT_NORMAL, gText_EnemyAISmart, 198), y, styles[1], active);
}

// Background tilemap
#define TILE_TOP_CORNER_L 0x1A2 // 418
#define TILE_TOP_EDGE     0x1A3 // 419
#define TILE_TOP_CORNER_R 0x1A4 // 420
#define TILE_LEFT_EDGE    0x1A5 // 421
#define TILE_RIGHT_EDGE   0x1A7 // 423
#define TILE_BOT_CORNER_L 0x1A8 // 424
#define TILE_BOT_EDGE     0x1A9 // 425
#define TILE_BOT_CORNER_R 0x1AA // 426

static void DrawBgWindowFrames(void)
{
    //                     bg, tile,              x, y, width, height, palNum
    // Option Texts window
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  2, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  3,  1, 16,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  3,  1, 16,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 13,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 13, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 13,  1,  1,  7);

    // Description window
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2, 14, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, 15,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, 15,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
