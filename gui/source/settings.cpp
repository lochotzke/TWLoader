// TWLoader: Settings screen.
#include "settings.h"
#include "main.h"
#include "date.h"
#include "sound.h"
#include "download.h"
#include "log.h"
#include "language.h"
#include "textfns.h"
#include "inifile.h"

#include <unistd.h>
#include <string>
using std::string;
using std::wstring;

#include <3ds.h>
#include <sf2d.h>
#include "citrostuff.h"
#include "img/twpng.h"
#include "keyboard.h"

// Textures.

/** Top screen **/
static bool settings_tex_loaded = false;
static sf2d_texture *whomeicontex = NULL;	// HOME icon.
static sf2d_texture *setvoltex[6] = { };	// Volume levels.
static sf2d_texture *setbatterychrgtex = NULL;	// Fully charged.
static sf2d_texture *setbatterytex[6] = { };	// Battery levels.

static sf2d_texture *dsboottex = NULL;		// DS boot screen
static sf2d_texture *dsiboottex = NULL;	// DSi boot screen
static sf2d_texture *dshstex = NULL;		// DS H&S screen
static sf2d_texture *dsihstex = NULL;		// DSi H&S screen
static sf2d_texture *disabledtex = NULL;	// Red circle with line

/** Bottom screen **/
sf2d_texture *settingstex = NULL;

enum SubScreenMode {
	SUBSCREEN_MODE_FRONTEND = 0,			// Frontend settings (page 1)
	SUBSCREEN_MODE_FRONTEND2 = 1,			// Frontend settings (page 2)
	SUBSCREEN_MODE_FRONTEND3 = 2,			// Frontend settings (page 3)
	SUBSCREEN_MODE_NTR_TWL = 3,				// NTR/TWL-mode settings
	SUBSCREEN_MODE_FLASH_CARD = 4,			// Flash card options
	SUBSCREEN_MODE_SUB_THEME = 5,			// Sub-theme select
	SUBSCREEN_MODE_CHANGE_ROM_PATH = 6,		// Sub-menu with rom path location
	SUBSCREEN_MODE_TWLNAND_NOT_FOUND = 7,	// TWLNAND side not found message
};
static SubScreenMode subscreenmode = SUBSCREEN_MODE_FRONTEND;

/** Settings **/

const char *twlnand_msg;

static CIniFile settingsini("sdmc:/_nds/twloader/settings.ini");

// Color settings.
// Use SET_ALPHA() to replace the alpha value.
const ColorData *color_data = NULL;
u32 menucolor;

// 3D offsets. (0 == Left, 1 == Right)
Offset3D offset3D[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

// Cursor position. (one per subscreen)
static int cursor_pos[7] = {0, 0, 0, 0, 0, 0, 0};

// Location of the bottom screen image.
const char* bottomloc = NULL;

// Settings
Settings_t settings;


/**
 * Reset the settings screen's subscreen mode.
 */
void settingsResetSubScreenMode(void)
{
	if (TWLNANDnotfound_msg != 2) {
		subscreenmode = SUBSCREEN_MODE_TWLNAND_NOT_FOUND;
	} else {
		subscreenmode = SUBSCREEN_MODE_FRONTEND;
	}
	memset(cursor_pos, 0, sizeof(cursor_pos));
}

/**
 * Load the settings textures.
 */
void settingsLoadTextures(void)
{
	if (settings_tex_loaded)
		return;

	/** Top screen **/
	setvoltex[0] = sfil_load_PNG_file("romfs:/graphics/settings/volume0.png", SF2D_PLACE_RAM); // Show no volume (settings)
	setvoltex[1] = sfil_load_PNG_file("romfs:/graphics/settings/volume1.png", SF2D_PLACE_RAM); // Volume low above 0 (settings)
	setvoltex[2] = sfil_load_PNG_file("romfs:/graphics/settings/volume2.png", SF2D_PLACE_RAM); // Volume medium (settings)
	setvoltex[3] = sfil_load_PNG_file("romfs:/graphics/settings/volume3.png", SF2D_PLACE_RAM); // Hight volume (settings)	
	setvoltex[4] = sfil_load_PNG_file("romfs:/graphics/settings/volume4.png", SF2D_PLACE_RAM); // 100% (settings)
	setvoltex[5] = sfil_load_PNG_file("romfs:/graphics/settings/volume5.png", SF2D_PLACE_RAM); // No DSP firm found (settings)

	setbatterychrgtex = sfil_load_PNG_file("romfs:/graphics/settings/battery_charging.png", SF2D_PLACE_RAM);
	setbatterytex[0] = sfil_load_PNG_file("romfs:/graphics/settings/battery0.png", SF2D_PLACE_RAM);
	setbatterytex[1] = sfil_load_PNG_file("romfs:/graphics/settings/battery1.png", SF2D_PLACE_RAM);
	setbatterytex[2] = sfil_load_PNG_file("romfs:/graphics/settings/battery2.png", SF2D_PLACE_RAM);
	setbatterytex[3] = sfil_load_PNG_file("romfs:/graphics/settings/battery3.png", SF2D_PLACE_RAM);
	setbatterytex[4] = sfil_load_PNG_file("romfs:/graphics/settings/battery4.png", SF2D_PLACE_RAM);
	setbatterytex[5] = sfil_load_PNG_file("romfs:/graphics/settings/battery5.png", SF2D_PLACE_RAM);

	dsboottex = sfil_load_PNG_file("romfs:/graphics/settings/dsboot.png", SF2D_PLACE_RAM); // DS boot screen in settings
	dsiboottex = sfil_load_PNG_file("romfs:/graphics/settings/dsiboot.png", SF2D_PLACE_RAM); // DSi boot screen in settings
	switch (settings.ui.language) {
		case 0:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_JA.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_JA.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 1:
		default:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dsihs.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dsihs.png", SF2D_PLACE_RAM); // DSi H&S screen in settings
			break;
		case 2:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_FR.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_FR.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 3:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_DE.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_DE.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 4:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_IT.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_IT.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 5:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_ES.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_ES.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 6:
		case 11:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_CH.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_CH.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
		case 7:
			dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_KO.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs_KO.png", SF2D_PLACE_RAM); // DS H&S screen in settings
			break;
	}
	disabledtex = sfil_load_PNG_file("romfs:/graphics/settings/disable.png", SF2D_PLACE_RAM); // Red circle with line

	/** Bottom screen **/
	settingstex = sfil_load_PNG_file("romfs:/graphics/settings/screen.png", SF2D_PLACE_RAM); // Bottom of settings screen
	whomeicontex = sfil_load_PNG_file("romfs:/graphics/settings/whomeicon.png", SF2D_PLACE_RAM); // HOME icon

	// All textures loaded.
	settings_tex_loaded = true;
}

/**
 * Unload the settings textures.
 */
void settingsUnloadTextures(void)
{
	if (!settings_tex_loaded)
		return;

	/** Top screen **/
	for (int i = 0; i < 6; i++) {
		sf2d_free_texture(setvoltex[i]);
		setvoltex[i] = NULL;
	}
	sf2d_free_texture(setbatterychrgtex);

	for (int i = 0; i < 6; i++) {
		sf2d_free_texture(setbatterytex[i]);
		setbatterytex[i] = NULL;
	}

	sf2d_free_texture(dsboottex);
	dsboottex = NULL;
	sf2d_free_texture(dsiboottex);
	dsiboottex = NULL;
	sf2d_free_texture(dshstex);
	dshstex = NULL;
	sf2d_free_texture(dsihstex);
	dsihstex = NULL;
	sf2d_free_texture(disabledtex);
	disabledtex = NULL;

	/** Bottom screen **/
	sf2d_free_texture(settingstex);
	settingstex = NULL;
	sf2d_free_texture(whomeicontex);
	whomeicontex = NULL;

	// All textures unloaded.
	settings_tex_loaded = false;
}

/**
 * Draw the top settings screen.
 */
void settingsDrawTopScreen(void)
{
	/* if (!musicbool) {
		if (dspfirmfound) { bgm_settings->play(); }
		musicbool = true;
	} */
	if (!settings_tex_loaded) {
		settingsLoadTextures();
	}
	update_battery_level(setbatterychrgtex, setbatterytex);

	// Draw twice; once per 3D framebuffer.
	for (int topfb = GFX_LEFT; topfb <= GFX_RIGHT; topfb++) {
		sf2d_start_frame(GFX_TOP, (gfx3dSide_t)topfb);
		sf2d_draw_texture_scale(settingstex, 0, 0, 1.32, 1);
		if (subscreenmode == SUBSCREEN_MODE_FRONTEND2) {
			if (settings.ui.bootscreen == 2) {
				sf2d_draw_texture(dsiboottex, offset3D[topfb].boxart+136, 20); // Draw boot screen
			} else if (settings.ui.bootscreen <= 1) {
				sf2d_draw_texture(dsboottex, offset3D[topfb].boxart+136, 20); // Draw boot screen
			} else {
				drawRectangle(offset3D[topfb].boxart+136, 20, 128, 96, RGBA8(255, 255, 255, 255));
			}
			if (settings.ui.healthsafety == 1) {
				if (settings.ui.bootscreen == 2) {
					sf2d_draw_texture(dsihstex, offset3D[topfb].boxart+136, 124); // Draw H&S screen
				} else if (settings.ui.bootscreen <= 1) {
					sf2d_draw_texture(dshstex, offset3D[topfb].boxart+136, 124); // Draw H&S screen
				} else {
					drawRectangle(offset3D[topfb].boxart+136, 124, 128, 96, RGBA8(255, 255, 255, 255));
				}
			} else {
				// Draw a white screen in place of the H&S screen.
				drawRectangle(offset3D[topfb].boxart+136, 124, 128, 96, RGBA8(255, 255, 255, 255));
			}
			if (!settings.ui.showbootscreen) {
				sf2d_draw_texture(disabledtex, offset3D[topfb].disabled+136, 20); // Draw disabled texture
				sf2d_draw_texture(disabledtex, offset3D[topfb].disabled+136, 124); // Draw disabled texture
			}
		} else {
			sf2d_draw_texture(settingslogotex, offset3D[topfb].boxart+400/2 - settingslogotex->width/2, 240/2 - settingslogotex->height/2);
			if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
				setTextColor(RGBA8(0, 0, 255, 255));
				renderText_w(offset3D[topfb].disabled+72, 174, 0.60, 0.60, false, TR(STR_SETTINGS_XBUTTON_RELEASE));
				setTextColor(RGBA8(0, 255, 0, 255));
				renderText_w(offset3D[topfb].disabled+72, 190, 0.60, 0.60, false, TR(STR_SETTINGS_YBUTTON_UNOFFICIAL));
			} else if (subscreenmode == SUBSCREEN_MODE_CHANGE_ROM_PATH) {
				setTextColor(RGBA8(255, 255, 255, 255));
				renderText(offset3D[topfb].disabled+32, 192, 0.55, 0.55, false, "TWLoader will auto-restart if location is changed.");
			}
		}

		setTextColor(RGBA8(255, 255, 255, 255));
		renderText(318.0f, 1, 0.58f, 0.58f, false, RetTime(false).c_str());
		
		if(!isNightly){
			std::string version = settings_vertext;		
			if (version.substr(version.find_first_not_of(' '), (version.find_last_not_of(' ') - version.find_first_not_of(' ') + 1)).size() > 8) {
				renderText(324, 222, 0.60, 0.60f, false, settings_vertext);
			}else{
				renderText(336, 222, 0.60, 0.60f, false, settings_vertext);
			}
		}else{
			char nightlyhash[16];
			snprintf(nightlyhash, 16, "%s", NIGHTLY);
			renderText(272, 222, 0.60, 0.60f, false, nightlyhash);			
		}
		if (settings.twl.bootstrapfile == 2) {
			fat = "fat:/";
			setTextColor(RGBA8(255, 0, 0, 255));
			renderText(5, 222, 0.60, 0.60f, false, "Old version");
		} else if (settings.twl.bootstrapfile == 1) {
			fat = "sd:/";
			setTextColor(RGBA8(0, 255, 0, 255));
			renderText(5, 222, 0.60, 0.60f, false, settings_unofficialbootstrapver.c_str());
		} else if (settings.twl.bootstrapfile == 0) {
			fat = "sd:/";
			setTextColor(RGBA8(0, 0, 255, 255));
			renderText(5, 222, 0.60, 0.60f, false, settings_releasebootstrapver.c_str());
		}

		draw_volume_slider(setvoltex);
		sf2d_draw_texture(batteryIcon, 371, 2);
		if (!settings.ui.name.empty()) {
			setTextColor(SET_ALPHA(color_data->color, 255));
			renderText(34.0f, 1.0f, 0.58, 0.58f, false, settings.ui.name.c_str());
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		DrawDate(264.0f, 1.0f, 0.58f, 0.58f, false);
		if (fadealpha > 0) drawRectangle(0, 0, 400, 240, RGBA8(0, 0, 0, fadealpha)); // Fade in/out effect
		sf2d_end_frame();
	}
}

/**
 * Draw the bottom settings screen.
 */
void settingsDrawBottomScreen(void)
{
	if (!settings_tex_loaded) {
		settingsLoadTextures();
	}

	sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
	sf2d_draw_texture(settingstex, 0, 0);
	drawRectangle(0, 0, 320, 28, RGBA8(0, 0, 0, 31));
	for (int i = 0; i < 80; i++)
		drawRectangle(i*4, 26, 2, 1, RGBA8(127, 127, 127, 255));
	drawRectangle(0, 179, 320, 41, RGBA8(0, 0, 0, 31));
	for (int i = 0; i < 80; i++)
		drawRectangle(i*4, 180, 2, 1, RGBA8(127, 127, 127, 255));
	for (int i = 0; i < 80; i++)
		drawRectangle(i*4, 218, 2, 1, RGBA8(127, 127, 127, 255));
	
	if (subscreenmode < 4) {
		if (subscreenmode == 0) {
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText(252, 6, 0.50, 0.50, false, "1");
		if (subscreenmode == 1) {
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText(268, 6, 0.50, 0.50, false, "2");
		if (subscreenmode == 2) {
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText(284, 6, 0.50, 0.50, false, "3");
		if (subscreenmode == 3) {
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText(300, 6, 0.50, 0.50, false, "4");
	}
	
	// X positions.
	static const int Xpos = 24;
	static const int XposValue = 236;
	// Title for the bottom screen.
	const wchar_t *title = L"";

	if (subscreenmode == SUBSCREEN_MODE_FRONTEND) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		setTextColor(RGBA8(127, 127, 127, 255));
		renderText(17, LshoulderYpos+4, 0.50, 0.50, false, Lshouldertext);
		setTextColor(RGBA8(0, 0, 0, 255));
		renderText(252, RshoulderYpos+4, 0.50, 0.50, false, Rshouldertext);

		// Language.
		static const char *const language_text[] = {
			"日本語",	// Japanese
			"English",	// English
			"Français",	// French
			"Deutsch",	// German
			"Italiano",	// Italian
			"Español",	// Spanish
			"ZHCN",		// Simplified Chinese (TODO)
			"Korean",	// Korean [Font is missing characters]
			"Nederlands",	// Dutch
			"Português",	// Portuguese
			"Russian",	// Russian (TODO) [Font's characters are too wide]
			"ZHTW",		// Traditional Chinese (TODO)
		};
		// TODO: Cache the conversion?
		wstring languagevaluetext;
		if (settings.ui.language >= 0 && settings.ui.language < 12) {
			languagevaluetext = utf8_to_wstring(language_text[settings.ui.language]);
		} else {
			// TODO: Translate?
			languagevaluetext = latin1_to_wstring("System");
		}

		// Theme text.
		static const char *const theme_text[] = {
			"DSi Menu", "R4", "akMenu/Wood"
		};
		if (settings.ui.theme < 0 || settings.ui.theme > 2)
			settings.ui.theme = 0;
		const char *const themevaluetext = theme_text[settings.ui.theme];

		// Color text.
		const wchar_t *color_text[] = {
			TR(STR_SETTINGS_VALUES_GRAY),
			TR(STR_SETTINGS_VALUES_BROWN),
			TR(STR_SETTINGS_VALUES_RED),
			TR(STR_SETTINGS_VALUES_PINK),
			TR(STR_SETTINGS_VALUES_ORANGE),
			TR(STR_SETTINGS_VALUES_YELLOW),
			TR(STR_SETTINGS_VALUES_YELLOW_GREEN),
			TR(STR_SETTINGS_VALUES_GREEN_1),
			TR(STR_SETTINGS_VALUES_GREEN_2),
			TR(STR_SETTINGS_VALUES_LIGHT_GREEN),
			TR(STR_SETTINGS_VALUES_SKY_BLUE),
			TR(STR_SETTINGS_VALUES_LIGHT_BLUE),
			TR(STR_SETTINGS_VALUES_BLUE),
			TR(STR_SETTINGS_VALUES_VIOLET),
			TR(STR_SETTINGS_VALUES_PURPLE),
			TR(STR_SETTINGS_VALUES_FUCHSIA),
			TR(STR_SETTINGS_VALUES_RED_AND_BLUE),
			TR(STR_SETTINGS_VALUES_GREEN_AND_YELLOW),
			TR(STR_SETTINGS_VALUES_CHRISTMAS)
		};
		if (settings.ui.color < 0 || settings.ui.color > 18)
			settings.ui.color = 0;
		const wchar_t *colorvaluetext = color_text[settings.ui.color];

		// Menu color text.
		const wchar_t *menu_color_text[] = {
			TR(STR_SETTINGS_VALUES_WHITE),
			TR(STR_SETTINGS_VALUES_BLACK),
			TR(STR_SETTINGS_VALUES_BROWN),
			TR(STR_SETTINGS_VALUES_RED),
			TR(STR_SETTINGS_VALUES_PINK),
			TR(STR_SETTINGS_VALUES_ORANGE),
			TR(STR_SETTINGS_VALUES_YELLOW),
			TR(STR_SETTINGS_VALUES_YELLOW_GREEN),
			TR(STR_SETTINGS_VALUES_GREEN_1),
			TR(STR_SETTINGS_VALUES_GREEN_2),
			TR(STR_SETTINGS_VALUES_LIGHT_GREEN),
			TR(STR_SETTINGS_VALUES_SKY_BLUE),
			TR(STR_SETTINGS_VALUES_LIGHT_BLUE),
			TR(STR_SETTINGS_VALUES_BLUE),
			TR(STR_SETTINGS_VALUES_VIOLET),
			TR(STR_SETTINGS_VALUES_PURPLE),
			TR(STR_SETTINGS_VALUES_FUCHSIA)
		};
		if (settings.ui.menucolor < 0 || settings.ui.menucolor > 18)
			settings.ui.menucolor = 0;
		const wchar_t  *menucolorvaluetext = menu_color_text[settings.ui.menucolor];

		const char *const filenamevaluetext = (settings.ui.filename ? "On" : "Off");
		const char *const countervaluetext = (settings.ui.counter ? "On" : "Off");

		title = TR(STR_SETTINGS_GUI);
		struct {
			int x;
			int y;
		} buttons[] = {
			{ 17,  39},
			{169,  39},
			{ 17,  87},
			{169,  87},
			{ 17, 135},
			{169, 135},
		};
		const wchar_t *button_titles[] = {
			TR(STR_SETTINGS_LANGUAGE),
			TR(STR_SETTINGS_THEME),
			TR(STR_SETTINGS_COLOR),
			TR(STR_SETTINGS_MENUCOLOR),
			TR(STR_SETTINGS_FILENAME),
			TR(STR_SETTINGS_COUNTER),
		};
		const wchar_t *button_descw[] = {
			languagevaluetext.c_str(),
			NULL,
			colorvaluetext,
			menucolorvaluetext,
			NULL,
			NULL,
		};
		const char *button_desc[] = {
			NULL,
			themevaluetext,
			NULL,
			NULL,
			filenamevaluetext,
			countervaluetext,
		};
		
		for (int i = (int)(sizeof(buttons)/sizeof(buttons[0]))-1; i >= 0; i--) {
			if (cursor_pos[0] == i) {
				// Button is highlighted.
				sf2d_draw_texture(dboxtex_button, buttons[i].x, buttons[i].y);
			} else {
				// Button is not highlighted. Darken the texture.
				sf2d_draw_texture_blend(dboxtex_button, buttons[i].x, buttons[i].y, RGBA8(127, 127, 127, 255));
			}

			const wchar_t *title = button_titles[i];
			const wchar_t *value_descw = button_descw[i];
			const char *value_desc = button_desc[i];

			// Determine the text height.
			// NOTE: Button texture size is 132x34.
			const int h = 32;

			// Draw the title.
			int y = buttons[i].y + ((34 - h) / 2);
			// int w = sftd_get_wtext_width(font, 12, title);
			int w = 0;
			// int x = ((132 - w) / 2) + buttons[i].x;
			int x = ((2 - w) / 2) + buttons[i].x;
			setTextColor(RGBA8(0, 0, 0, 255));
			renderText_w(x, y, 0.50, 0.50, false, title);
			y += 16;

			// Draw the value.
			// w = sftd_get_wtext_width(font, 12, value_desc);
			w = 0;
			// x = ((132 - w) / 2) + buttons[i].x;
			x = ((2 - w) / 2) + buttons[i].x;
			if (i == 2) setTextColor(SET_ALPHA(color_data->color, 255));
			else setTextColor(RGBA8(0, 0, 0, 255));
			if (i == 0 || i == 2 || i == 3) renderText_w(x, y, 0.50, 0.50, false, value_descw);
			else renderText(x, y, 0.50, 0.50, false, value_desc);
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		if (cursor_pos[0] == 0) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_LANGUAGE_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_LANGUAGE_2));
		}
		if (cursor_pos[0] == 1) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_THEME_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_THEME_2));
		}
		if (cursor_pos[0] == 2) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_COLOR_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_COLOR_2));
		}
		if (cursor_pos[0] == 3) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_MENUCOLOR_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_MENUCOLOR_2));
		}
		if (cursor_pos[0] == 4) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_FILENAME_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_FILENAME_2));
		}
		if (cursor_pos[0] == 5) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_COUNTER_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_COUNTER_2));
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FRONTEND2) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		setTextColor(RGBA8(0, 0, 0, 255));
		renderText(17, LshoulderYpos+4, 0.50, 0.50, false, Lshouldertext);
		renderText(252, RshoulderYpos+4, 0.50, 0.50, false, Rshouldertext);

		const char *const custombotvaluetext = (settings.ui.custombot ? "On" : "Off");
		
		const char *autodlvaluetext = (settings.ui.autoupdate_twldr ? "On" : "Off");

		const char *filetypevaluetext;
		switch (settings.ui.filetype) {
			case 0:
			default:
				filetypevaluetext = "CIA";
				break;
			case 1:
				filetypevaluetext = "3DSX";
				break;
			case 2:
				filetypevaluetext = "CIA & 3DSX";
				break;
		}
		
		// Boot screen text.
		static const char *const bootscreen_text[] = {
			"Nintendo DS", "Nintendo DS (OAR)", "Nintendo DSi", "Nintendo DS (Black)", "Nintendo DSi (Black)"
		};
		if (settings.ui.bootscreen < 0 || settings.ui.bootscreen > 4)
			settings.ui.bootscreen = 0;
		const char *const bootscreenvaluetext = bootscreen_text[settings.ui.bootscreen];
		const char *healthsafetyvaluetext = (settings.ui.healthsafety ? "On" : "Off");

		title = TR(STR_SETTINGS_GUI);
		struct {
			int x;
			int y;
		} buttons[] = {
			{ 17,  39},
			{169,  39},
			{ 17,  87},
			{169,  87},
			{ 17, 135},
			{169, 135},
		};
		const wchar_t *button_titles[] = {
			TR(STR_SETTINGS_CUSTOM_BOTTOM),
			TR(STR_SETTINGS_AUTOUPDATE_TWLOADER),
			TR(STR_SETTINGS_TWLOADER_FILETYPE),
			TR(STR_SETTINGS_UPDATE_TWLOADER),
			TR(STR_SETTINGS_DS_DSi_BOOT_SCREEN),
			TR(STR_SETTINGS_DS_DSi_SAFETY_MESSAGE),
		};
		const char *button_desc[] = {
			custombotvaluetext,
			autodlvaluetext,
			filetypevaluetext,
			"",
			bootscreenvaluetext,
			healthsafetyvaluetext,
		};
		
		for (int i = (int)(sizeof(buttons)/sizeof(buttons[0]))-1; i >= 0; i--) {
			if (cursor_pos[1] == i) {
				// Button is highlighted.
				sf2d_draw_texture(dboxtex_button, buttons[i].x, buttons[i].y);
			} else {
				// Button is not highlighted. Darken the texture.
				sf2d_draw_texture_blend(dboxtex_button, buttons[i].x, buttons[i].y, RGBA8(127, 127, 127, 255));
			}

			const wchar_t *title = button_titles[i];
			const char *value_desc = button_desc[i];

			// Determine the text height.
			// NOTE: Button texture size is 132x34.
			const int h = 32;

			// Draw the title.
			int y = buttons[i].y + ((34 - h) / 2);
			// int w = sftd_get_wtext_width(font, 12, title);
			int w = 0;
			// int x = ((132 - w) / 2) + buttons[i].x;
			int x = ((2 - w) / 2) + buttons[i].x;
			setTextColor(RGBA8(0, 0, 0, 255));
			renderText_w(x, y, 0.50, 0.50, false, title);
			y += 16;

			// Draw the value.
			// w = sftd_get_wtext_width(font, 12, value_desc);
			w = 0;
			// x = ((132 - w) / 2) + buttons[i].x;
			x = ((2 - w) / 2) + buttons[i].x;
			renderText(x, y, 0.50, 0.50, false, value_desc);
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		if (cursor_pos[1] == 0) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_CUSTOM_BOTTOM_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_CUSTOM_BOTTOM_2));
		}
		if (cursor_pos[1] == 1) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_AUTOUPDATE_TWLOADER_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_AUTOUPDATE_TWLOADER_2));
		}
		if (cursor_pos[1] == 2) {
			renderText(8, 184, 0.60, 0.60f, false, "Select the filetype of TWLoader");
			renderText(8, 198, 0.60, 0.60f, false, "you're using.");
		}
		if (cursor_pos[1] == 3) {
			renderText(8, 184, 0.60, 0.60f, false, "Press  to update TWLoader.");
		}
		if (cursor_pos[1] == 4) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_DS_DSi_BOOT_SCREEN_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_DS_DSi_BOOT_SCREEN_2));
		}
		if (cursor_pos[1] == 5) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_DS_DSi_SAFETY_MESSAGE_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_DS_DSi_SAFETY_MESSAGE_2));
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FRONTEND3) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		setTextColor(RGBA8(0, 0, 0, 255));
		renderText(17, LshoulderYpos+4, 0.50, 0.50, false, Lshouldertext);
		renderText(252, RshoulderYpos+4, 0.50, 0.50, false, Rshouldertext);

		const char *showbootscreenvaluetext;
		switch (settings.ui.showbootscreen) {
			case 0:
			default:
				showbootscreenvaluetext = "No";
				break;
			case 1:
				showbootscreenvaluetext = "Before ROM select screen";
				break;
			case 2:
				showbootscreenvaluetext = "After launching a ROM";
				break;
		}

		char printedROMpath[256];
		snprintf (printedROMpath, sizeof(printedROMpath), "SD:/%s", settings.ui.romfolder.c_str());

		title = TR(STR_SETTINGS_GUI);
		struct {
			int x;
			int y;
		} buttons[] = {
			{ 17,  39},
			{169,  39},
			// { 17,  87},
			// {169,  87},
			// { 17, 135},
			// {169, 135},
		};
		const wchar_t *button_titles[] = {
			TR(STR_SETTINGS_SHOW_BOOT_SCREEN),
			TR(STR_SETTINGS_ROM_PATH),
		};
		const char *button_desc[] = {
			showbootscreenvaluetext,
			printedROMpath,
		};
		
		for (int i = (int)(sizeof(buttons)/sizeof(buttons[0]))-1; i >= 0; i--) {
			if (cursor_pos[2] == i) {
				// Button is highlighted.
				sf2d_draw_texture(dboxtex_button, buttons[i].x, buttons[i].y);
			} else {
				// Button is not highlighted. Darken the texture.
				sf2d_draw_texture_blend(dboxtex_button, buttons[i].x, buttons[i].y, RGBA8(127, 127, 127, 255));
			}

			const wchar_t *title = button_titles[i];
			const char *value_desc = button_desc[i];

			// Determine the text height.
			// NOTE: Button texture size is 132x34.
			const int h = 32;

			// Draw the title.
			int y = buttons[i].y + ((34 - h) / 2);
			// int w = sftd_get_wtext_width(font, 12, title);
			int w = 0;
			// int x = ((132 - w) / 2) + buttons[i].x;
			int x = ((2 - w) / 2) + buttons[i].x;
			setTextColor(RGBA8(0, 0, 0, 255));
			renderText_w(x, y, 0.50, 0.50, false, title);
			y += 16;

			// Draw the value.
			// w = sftd_get_wtext_width(font, 12, value_desc);
			w = 0;
			// x = ((132 - w) / 2) + buttons[i].x;
			x = ((2 - w) / 2) + buttons[i].x;
			renderText(x, y, 0.50, 0.50, false, value_desc);
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		if (cursor_pos[2] == 0) {
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_SHOW_BOOT_SCREEN_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_SHOW_BOOT_SCREEN_2));
		}
		if (cursor_pos[2] == 1) {
			// Selected			
			renderText(8, 184, 0.60, 0.60f, false, "Press  to change rom location folder.");
		}
	} else if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		setTextColor(RGBA8(0, 0, 0, 255));
		renderText(17, LshoulderYpos+4, 0.50, 0.50, false, Lshouldertext);
		setTextColor(RGBA8(127, 127, 127, 255));
		renderText(252, RshoulderYpos+4, 0.50, 0.50, false, Rshouldertext);

		const char *rainbowledvaluetext = (settings.twl.rainbowled ? "On" : "Off");
		const char *cpuspeedvaluetext = (settings.twl.cpuspeed ? "133mhz (TWL)" : "67mhz (NTR)");
		const char *enablesdvaluetext = (settings.twl.enablesd ? "On" : "Off");
		const char *resetslot1valuetext = (settings.twl.resetslot1 ? "On" : "Off");

		const char *autoupdatevaluetext;
		switch (settings.ui.autoupdate) {
			case 0:
			default:
				autoupdatevaluetext = "Off";
				break;
			case 1:
				autoupdatevaluetext = "Release";
				break;
			case 2:
				autoupdatevaluetext = "Unofficial";
				break;
		}

		const char *bootstrapfilevaluetext;
		switch (settings.twl.bootstrapfile) {
			case 0:
			default:
				bootstrapfilevaluetext = "Release";
				break;
			case 1:
				bootstrapfilevaluetext = "Unofficial";
				break;
			case 2:
				bootstrapfilevaluetext = "Old";
				break;
		}

		const char *consolevaluetext;
		switch (settings.twl.console) {
			case 0:
			default:
				consolevaluetext = "Off";
				break;
			case 1:
				consolevaluetext = "On";
				break;
			case 2:
				consolevaluetext = "On (Debug)";
				break;
		}

		// const char *lockarm9scfgextvaluetext = (settings.twl.lockarm9scfgext ? "On" : "Off");

		title = TR(STR_SETTINGS_NTR_TWL);
		int Ypos = 40;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 0) {			
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_FLASHCARD_SELECT_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_FLASHCARD_SELECT_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_FLASHCARD_SELECT));
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 1) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_RAINBOW_LED_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_RAINBOW_LED_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_RAINBOW_LED));
		renderText(XposValue, Ypos, 0.55, 0.55, false, rainbowledvaluetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 2) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(SRT_SETTINGS_DESCRIPTION_ARM9_CPU_SPEED_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(SRT_SETTINGS_DESCRIPTION_ARM9_CPU_SPEED_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(SRT_SETTINGS_ARM9_CPU_SPEED));
		renderText(XposValue, Ypos, 0.45, 0.55, false, cpuspeedvaluetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 3) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_VRAM_BOOST_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_VRAM_BOOST_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_VRAM_BOOST));
		renderText(XposValue, Ypos, 0.55, 0.55, false, enablesdvaluetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 4) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_RESET_SLOT_1_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_RESET_SLOT_1_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_RESET_SLOT_1));
		renderText(XposValue, Ypos, 0.55, 0.55, false, resetslot1valuetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 5) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_CONSOLE_OUTPUT_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_CONSOLE_OUTPUT_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_CONSOLE_OUTPUT));
		renderText(XposValue, Ypos, 0.55, 0.55, false, consolevaluetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 6) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_AUTOUPDATE_BOOTSTRAP_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_AUTOUPDATE_BOOTSTRAP_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_AUTOUPDATE_BOOTSTRAP));
		renderText(XposValue, Ypos, 0.55, 0.55, false, autoupdatevaluetext);
		Ypos += 12;
		if (cursor_pos[SUBSCREEN_MODE_NTR_TWL] == 7) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_BOOTSTRAP_1));
			renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_DESCRIPTION_BOOTSTRAP_2));
			setTextColor(SET_ALPHA(color_data->color, 255));
		} else {
			setTextColor(RGBA8(255, 255, 255, 255));
		}
		renderText_w(Xpos, Ypos, 0.55, 0.55, false, TR(STR_SETTINGS_BOOTSTRAP));
		renderText(XposValue, Ypos, 0.55, 0.55, false, bootstrapfilevaluetext);
		Ypos += 12;		
	} else if (subscreenmode == SUBSCREEN_MODE_FLASH_CARD) {
		// Flash card options.
		static const char *const flash_card_options[][6] = {
			{"DSTT", "R4i Gold", "R4i-SDHC (Non-v1.4.x version) (www.r4i-sdhc.com)",
				"R4 SDHC Dual-Core", "R4 SDHC Upgrade", "SuperCard DSONE"},
			{"Original R4", "M3 Simply", " ", " ", " ", " "},
			{"R4iDSN", "R4i Gold RTS", " ", " ", " ", " "},
			{"Acekard 2(i)", "Galaxy Eagle", "M3DS Real", " ", " ", " "},
			{"Acekard RPG", " ", " ", " ", " ", " "},
			{"Ace 3DS+", "Gateway Blue Card", "R4iTT", " ", " ", " "},
			{"SuperCard DSTWO", " ", " ", " ", " ", " "},
		};

		if (settings.twl.flashcard < 0 || settings.twl.flashcard > 6) {
			settings.twl.flashcard = 0;
		}
		const char *const *fctext = flash_card_options[settings.twl.flashcard];
		title = TR(STR_SETTINGS_FLASHCARD_SELECT);
		int Ypos = 40;
		for (int i = 0; i < 6; i++, Ypos += 12) {
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText(Xpos, Ypos, 0.45, 0.45, false, fctext[i]);
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		renderText_w(8, 184, 0.60, 0.60f, false, TR(STR_SETTINGS_LEFTRIGHT_PICK));
		renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_AB_SAVE_RETURN));
	} else if (subscreenmode == SUBSCREEN_MODE_SUB_THEME) {
		if (settings.ui.theme == 0) {
			title = TR(STR_SETTINGS_SUBTHEME_DSi);
			setTextColor(SET_ALPHA(color_data->color, 255));
			renderText_w(Xpos, 40, 0.55, 0.55, false, TR(STR_SETTINGS_NO_SUB_THEMES));
		} else if (settings.ui.theme == 1) {
			title = TR(STR_SETTINGS_SUBTHEME_R4);
			int Ypos = 30;
			if (settings.ui.subtheme == 0) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme01");
			Ypos += 12;
			if (settings.ui.subtheme == 1) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme02");
			Ypos += 12;
			if (settings.ui.subtheme == 2) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme03");
			Ypos += 12;
			if (settings.ui.subtheme == 3) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme04");
			Ypos += 12;
			if (settings.ui.subtheme == 4) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme05");
			Ypos += 12;
			if (settings.ui.subtheme == 5) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme06");
			Ypos += 12;
			if (settings.ui.subtheme == 6) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme07");
			Ypos += 12;
			if (settings.ui.subtheme == 7) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme08");
			Ypos += 12;
			if (settings.ui.subtheme == 8) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme09");
			Ypos += 12;
			if (settings.ui.subtheme == 9) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme10");
			Ypos += 12;
			if (settings.ui.subtheme == 10) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme11");
			Ypos += 12;
			if (settings.ui.subtheme == 11) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "theme12");
			Ypos += 12;
		} else if (settings.ui.theme == 2) {
			title = TR(STR_SETTINGS_SUBTHEME_WOOD);
			int Ypos = 40;
			if (settings.ui.subtheme == 0) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "GBATemp");
			Ypos += 12;
			if (settings.ui.subtheme == 1) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "Acekard black");
			Ypos += 12;
			if (settings.ui.subtheme == 2) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "akaio");
			Ypos += 12;
			if (settings.ui.subtheme == 3) {
				setTextColor(SET_ALPHA(color_data->color, 255));
			} else {
				setTextColor(RGBA8(255, 255, 255, 255));
			}
			renderText(Xpos, Ypos, 0.55, 0.55, false, "DSTWO");
			Ypos += 12;
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		renderText_w(8, 198, 0.60, 0.60f, false, TR(STR_SETTINGS_AB_SAVE_RETURN));
	}else if (subscreenmode == SUBSCREEN_MODE_CHANGE_ROM_PATH) {
		title = L"Rom path location";
	
		char printedROMpath[256];
		snprintf (printedROMpath, sizeof(printedROMpath), "SD:/%s", settings.ui.romfolder.c_str());

		char printedFCROMpath[256];
		snprintf (printedFCROMpath, sizeof(printedFCROMpath), "SD:/%s", settings.ui.fcromfolder.c_str());

		if (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] == 0){
			// Selected SD
			setTextColor(SET_ALPHA(color_data->color, 255));
			renderText(24, 40, 0.55, 0.55, false, "SD ROM location:");
			renderText(30, 52, 0.55, 0.55, false, printedROMpath);
			
			// Unselected Flashcard
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText(24, 66, 0.55, 0.55, false, "Flashcard INI location:");
			renderText(30, 78, 0.55, 0.55, false, printedFCROMpath);
			
		}else if (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] == 1){
			// Unselected SD
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText(24, 40, 0.55, 0.55, false, "SD ROM location:");
			renderText(30, 52, 0.55, 0.55, false, printedROMpath);
			
			// Selected Flashcard
			setTextColor(SET_ALPHA(color_data->color, 255));
			renderText(24, 66, 0.55, 0.55, false, "Flashcard INI location:");
			renderText(30, 78, 0.55, 0.55, false, printedFCROMpath);
			
		}
		
		setTextColor(RGBA8(255, 255, 255, 255));
		
		renderText(8, 184, 0.60, 0.60, false, ": Change path");
		renderText(8, 198, 0.60, 0.60, false, ": Return");
	} else if (subscreenmode == SUBSCREEN_MODE_TWLNAND_NOT_FOUND) {
		if(!isDemo) {
			const wchar_t *home_text = TR(STR_RETURN_TO_HOME_MENU);
			// const int home_width = sftd_get_wtext_width(font, 13, home_text) + 16;
			const int home_width = 144+16;
			const int home_x = (320-home_width)/2;
			sf2d_draw_texture(whomeicontex, home_x, 221); // Draw HOME icon
			setTextColor(RGBA8(255, 255, 255, 255));
			renderText_w(home_x+20, 222, 0.50, 0.50, false, home_text);
		}

		title = L"An error has occurred.";

		if (TWLNANDnotfound_msg == 0) {
			twlnand_msg =
				"TWLNAND-side (part 1) has not been installed.\n"
				"Please install the TWLNAND-side (part 1) CIA:\n"
				"\n"
				"sd:/_nds/twloader/cias/TWLoader - TWLNAND side.cia";
		} else if (TWLNANDnotfound_msg == 1) {
			twlnand_msg =
				"TWLNAND-side (part 2) has not been installed.\n"
				"Please install the TWLNAND-side (part 2) CIA:\n"
				"\n"
				"sd:/_nds/twloader/cias/\n"
				"TWLoader - TWLNAND side (part 2).cia";
		}
		setTextColor(RGBA8(255, 255, 255, 255));
		renderText(16, 40, 0.4, 0.4, false, twlnand_msg);
	}
	setTextColor(RGBA8(255, 255, 255, 255));
	renderText_w(2, 2, 0.75, 0.75, false, title);
}

/**
 * Move the cursor if necessary.
 * @param hDown Key value from hidKeysDown().
 * @return True if the bottom screen needs to be updated.
 */
bool settingsMoveCursor(u32 hDown)
{
	touchPosition touch;
	hidTouchRead(&touch);

	Lshouldertext = "Previous";
	Rshouldertext = "Next";

	if (hDown == 0) {
		// Nothing to do here.
		return false;
	}

	// Sound effect to play.
	sound *sfx = NULL;

	if (subscreenmode == SUBSCREEN_MODE_SUB_THEME) {
		if (hDown & KEY_UP) {
			settings.ui.subtheme--;
			sfx = sfx_select;
		} else if (hDown & KEY_DOWN) {
			settings.ui.subtheme++;
			sfx = sfx_select;
		} else if (hDown & (KEY_A | KEY_B)) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND;
			sfx = sfx_select;
		}
		if (settings.ui.theme == 0) {
			settings.ui.subtheme = 0;
		} else if (settings.ui.theme == 1) {
			if (settings.ui.subtheme < 0)
				settings.ui.subtheme = 11;
			else if (settings.ui.subtheme > 11)
				settings.ui.subtheme = 0;
		} else if (settings.ui.theme == 2) {
			if (settings.ui.subtheme < 0)
				settings.ui.subtheme = 3;
			else if (settings.ui.subtheme > 3)
				settings.ui.subtheme = 0;
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FLASH_CARD) {
		if (hDown & KEY_LEFT && settings.twl.flashcard > 0) {
			settings.twl.flashcard--; // Flashcard
			// if (settings.twl.flashcard == 2) settings.twl.flashcard = 1;
			sfx = sfx_select;
		} else if (hDown & KEY_RIGHT && settings.twl.flashcard < 6) {
			settings.twl.flashcard++; // Flashcard
			// if (settings.twl.flashcard == 2) settings.twl.flashcard = 3;
			sfx = sfx_select;
		} else if (hDown & (KEY_A | KEY_B)) {
			subscreenmode = SUBSCREEN_MODE_NTR_TWL;
			sfx = sfx_select;
		}
	} else if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		if (hDown & (KEY_A | KEY_LEFT | KEY_RIGHT)) {
			switch (cursor_pos[SUBSCREEN_MODE_NTR_TWL]) {
				case 0:
				default:
					// Top item: Only listen to 'A'.
					if (hDown & KEY_A) {
						subscreenmode = SUBSCREEN_MODE_FLASH_CARD;
					} else {
						// Ignore this key.
						return false;
					}
					break;
				case 1:	// Rainbow LED
					settings.twl.rainbowled = !settings.twl.rainbowled;
					break;
				case 2:	// CPU speed
					settings.twl.cpuspeed = !settings.twl.cpuspeed;
					break;
				case 3:	// SD card access for Slot-1
					settings.twl.enablesd = !settings.twl.enablesd;
					break;
				case 4:	// Reset Slot-1
					settings.twl.resetslot1 = !settings.twl.resetslot1;
					break;
				case 5:	// Console output
					if (hDown & (KEY_A | KEY_RIGHT)) {
						settings.twl.console++;
						if (settings.twl.console > 2) {
							settings.twl.console = 0;
						}
					} else if (hDown & KEY_LEFT) {
						settings.twl.console--;
						if (settings.twl.console < 0) {
							settings.twl.console = 2;
						}
					}
					break;
				case 6:	// Enable or disable autoupdate
					if (hDown & (KEY_A | KEY_RIGHT)) {
						settings.ui.autoupdate++;
						if (settings.ui.autoupdate > 2) {
							settings.ui.autoupdate = 0;
						}
					} else if (hDown & KEY_LEFT) {
						settings.ui.autoupdate--;
						if (settings.ui.autoupdate < 0) {
							settings.ui.autoupdate = 2;
						}
					}
					break;
				case 7: // Bootstrap version
					if (hDown & (KEY_A | KEY_RIGHT)) {
						settings.twl.bootstrapfile++;
						if (settings.twl.bootstrapfile > 2) {
							settings.twl.bootstrapfile = 0;
						}
					} else if (hDown & KEY_LEFT) {
						settings.twl.bootstrapfile--;
						if (settings.twl.bootstrapfile < 0) {
							settings.twl.bootstrapfile = 2;
						}
					}
					break;					
			}
			sfx = sfx_select;
		} else if ((hDown & KEY_DOWN) && cursor_pos[SUBSCREEN_MODE_NTR_TWL] < 7) {
			cursor_pos[SUBSCREEN_MODE_NTR_TWL]++;
			sfx = sfx_select;
		} else if ((hDown & KEY_UP) && cursor_pos[SUBSCREEN_MODE_NTR_TWL] > 0) {
			cursor_pos[SUBSCREEN_MODE_NTR_TWL]--;
			sfx = sfx_select;
		} else if (hDown & KEY_L) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND3;
			sfx = sfx_switch;
		} else if (hDown & KEY_R) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND;
			sfx = sfx_switch;
		} else if (hDown & KEY_X) {
			if (checkWifiStatus()) {
				// Play the sound now instead of waiting.
				if (dspfirmfound && sfx_select) {
					sfx_select->stop();	// Prevent freezing
					sfx_select->play();
				}
				UpdateBootstrapRelease();
			} else {
				// Wi-Fi is not connected.
				sfx = sfx_wrong;
			}
		} else if (hDown & KEY_Y) {
			if (checkWifiStatus()) {
				// Play the sound now instead of waiting.
				if (dspfirmfound && sfx_select) {
					sfx_select->stop();	// Prevent freezing
					sfx_select->play();
				}
				UpdateBootstrapUnofficial();
			} else {
				// Wi-Fi is not connected.
				sfx = sfx_wrong;
			}
		} else if (hDown & KEY_B) {
			titleboxXmovetimer = 1;
			fadeout = true;
			//bgm_settings->stop();
			sfx = sfx_back;
		}
		if(hDown & KEY_TOUCH){
			if (touch.px <= 72 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND3;
				sfx = sfx_switch;
			}
			if (touch.px >= 248 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND;
				sfx = sfx_switch;
			}
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FRONTEND3) {
		if (hDown & (KEY_A | KEY_Y)) {
			switch (cursor_pos[SUBSCREEN_MODE_FRONTEND3]) {
				case 0:
				default:
					if (hDown & KEY_A) {
						settings.ui.showbootscreen++;
						if (settings.ui.showbootscreen > 2) {
							settings.ui.showbootscreen = 0;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.showbootscreen--;
						if (settings.ui.showbootscreen < 0) {
							settings.ui.showbootscreen = 2;
						}
					}
					break;
				case 1: // Rom path
					if (hDown & KEY_A) {
						subscreenmode = SUBSCREEN_MODE_CHANGE_ROM_PATH;
					}
					break;
			}
			sfx = sfx_select;
		} else if ((hDown & KEY_DOWN) && cursor_pos[2] < 2) {
			cursor_pos[2] += 2;
			if (cursor_pos[2] > 1) cursor_pos[2] -= 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_UP) && cursor_pos[2] > 0) {
			cursor_pos[2] -= 2;
			if (cursor_pos[2] < 0) cursor_pos[2] += 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_RIGHT) && cursor_pos[2] < 2) {
			if (cursor_pos[2] == 0
			|| cursor_pos[2] == 2
			|| cursor_pos[2] == 4)
				cursor_pos[2]++;
			sfx = sfx_select;
		} else if ((hDown & KEY_LEFT) && cursor_pos[2] > 0) {
			if (cursor_pos[2] == 1
			|| cursor_pos[2] == 3
			|| cursor_pos[2] == 5)
				cursor_pos[2]--;
			sfx = sfx_select;
		} else 	if (hDown & KEY_L) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND2;
			sfx = sfx_switch;
		} else if (hDown & KEY_R) {
			subscreenmode = SUBSCREEN_MODE_NTR_TWL;
			sfx = sfx_switch;
		} else if (hDown & KEY_B) {
			titleboxXmovetimer = 1;
			fadeout = true;
			sfx = sfx_back;
		}
		if(hDown & KEY_TOUCH){
			if (touch.px <= 72 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND2;
				sfx = sfx_switch;
			}
			if (touch.px >= 248 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_NTR_TWL;
				sfx = sfx_switch;
			}
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FRONTEND2) {
		if (hDown & (KEY_A | KEY_Y)) {
			switch (cursor_pos[SUBSCREEN_MODE_FRONTEND2]) {
				case 0:	// Custom bottom image
				default:
					settings.ui.custombot = !settings.ui.custombot;
					LoadBottomImage();
					break;
				case 1:	// Enable or disable autoupdate TWLoader
					settings.ui.autoupdate_twldr = !settings.ui.autoupdate_twldr;
					break;
				case 2:
					if (!isDemo) {
						if (hDown & KEY_A) {
							settings.ui.filetype++;
							if (settings.ui.filetype > 2) {
								settings.ui.filetype = 0;
							}
						} else if (hDown & KEY_Y) {
							settings.ui.filetype--;
							if (settings.ui.filetype < 0) {
								settings.ui.filetype = 2;
							}
						}
					}
					break;
				case 3:
					if (checkWifiStatus()) {
						if (checkUpdate() == 0) {
							// Play the sound now instead of waiting.
							if (dspfirmfound && sfx_select) {
								sfx_select->stop();	// Prevent freezing
								sfx_select->play();
							}
							DownloadTWLoaderCIAs();
						}
					}
					break;
				case 4:	// Boot screen
					if (hDown & KEY_A) {
						settings.ui.bootscreen++;
						if (settings.ui.bootscreen > 4) {
							settings.ui.bootscreen = 0;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.bootscreen--;
						if (settings.ui.bootscreen < 0) {
							settings.ui.bootscreen = 4;
						}
					}
					break;
				case 5:	// H&S message
					settings.ui.healthsafety = !settings.ui.healthsafety;
					break;
			}
			sfx = sfx_select;
		} else if ((hDown & KEY_DOWN) && cursor_pos[1] < 6) {
			cursor_pos[1] += 2;
			if (cursor_pos[1] > 5) cursor_pos[1] -= 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_UP) && cursor_pos[1] > 0) {
			cursor_pos[1] -= 2;
			if (cursor_pos[1] < 0) cursor_pos[1] += 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_RIGHT) && cursor_pos[1] < 6) {
			if (cursor_pos[1] == 0
			|| cursor_pos[1] == 2
			|| cursor_pos[1] == 4)
				cursor_pos[1]++;
			sfx = sfx_select;
		} else if ((hDown & KEY_LEFT) && cursor_pos[1] > 0) {
			if (cursor_pos[1] == 1
			|| cursor_pos[1] == 3
			|| cursor_pos[1] == 5)
				cursor_pos[1]--;
			sfx = sfx_select;
		} else 	if (hDown & KEY_L) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND;
			sfx = sfx_switch;
		} else if (hDown & KEY_R) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND3;
			sfx = sfx_switch;
		} else if (hDown & KEY_B) {
			titleboxXmovetimer = 1;
			fadeout = true;
			sfx = sfx_back;
		}
		if(hDown & KEY_TOUCH){
			if (touch.px <= 72 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND;
				sfx = sfx_switch;
			}
			if (touch.px >= 248 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND3;
				sfx = sfx_switch;
			}
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FRONTEND) {
		if (hDown & (KEY_A | KEY_Y)) {
			switch (cursor_pos[SUBSCREEN_MODE_FRONTEND]) {
				case 0:	// Language
				default:
					if (hDown & KEY_A) {
						settings.ui.language++;
						if (settings.ui.language > 11) {
							settings.ui.language = -1;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.language--;
						if (settings.ui.language < -1) {
							settings.ui.language = 11;
						}
					}
					langInit();
					break;
				case 1:	// Theme
					if (hDown & KEY_A) {
						settings.ui.subtheme = 0;
						settings.ui.theme++;
						if (settings.ui.theme > 2) {
							settings.ui.theme = 0;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.subtheme = 0;
						settings.ui.theme--;
						if (settings.ui.theme < 0) {
							settings.ui.theme = 2;
						}
					}
					break;
				case 2:	// Color
					if (hDown & KEY_A) {
						settings.ui.color++;
						if (settings.ui.color > 18) {
							settings.ui.color = 0;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.color--;
						if (settings.ui.color < 0) {
							settings.ui.color = 18;
						}
					}
					LoadColor();
					break;
				case 3:	// Menu color
					if (hDown & KEY_A) {
						settings.ui.menucolor++;
						if (settings.ui.menucolor > 16) {
							settings.ui.menucolor = 0;
						}
					} else if (hDown & KEY_Y) {
						settings.ui.menucolor--;
						if (settings.ui.menucolor < 0) {
							settings.ui.menucolor = 16;
						}
					}
					LoadMenuColor();
					break;
				case 4:	// Show filename
					settings.ui.filename = !settings.ui.filename;
					break;
				case 5:	// Game counter
					settings.ui.counter = !settings.ui.counter;
					break;
			}
			sfx = sfx_select;
		} else if ((hDown & KEY_START) && cursor_pos[0] == 1) {
			subscreenmode = SUBSCREEN_MODE_SUB_THEME;
		} else if ((hDown & KEY_DOWN) && cursor_pos[0] < 6) {
			cursor_pos[0] += 2;
			if (cursor_pos[0] > 5) cursor_pos[0] -= 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_UP) && cursor_pos[0] > 0) {
			cursor_pos[0] -= 2;
			if (cursor_pos[0] < 0) cursor_pos[0] += 2;
			sfx = sfx_select;
		} else if ((hDown & KEY_RIGHT) && cursor_pos[0] < 6) {
			if (cursor_pos[0] == 0
			|| cursor_pos[0] == 2
			|| cursor_pos[0] == 4)
				cursor_pos[0]++;
			sfx = sfx_select;
		} else if ((hDown & KEY_LEFT) && cursor_pos[0] > 0) {
			if (cursor_pos[0] == 1
			|| cursor_pos[0] == 3
			|| cursor_pos[0] == 5)
				cursor_pos[0]--;
			sfx = sfx_select;
		} else if (hDown & KEY_L) {
			subscreenmode = SUBSCREEN_MODE_NTR_TWL;
			sfx = sfx_switch;
		} else if (hDown & KEY_R) {
			subscreenmode = SUBSCREEN_MODE_FRONTEND2;
			sfx = sfx_switch;
		} else if (hDown & KEY_B) {
			titleboxXmovetimer = 1;
			fadeout = true;
			sfx = sfx_back;
		}
		if(hDown & KEY_TOUCH){
			if (touch.px <= 72 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_NTR_TWL;
				sfx = sfx_switch;
			}
			if (touch.px >= 248 && touch.py >= 220) {
				subscreenmode = SUBSCREEN_MODE_FRONTEND2;
				sfx = sfx_switch;
			}
		}
	}else if (subscreenmode == SUBSCREEN_MODE_CHANGE_ROM_PATH) {
		if (hDown & KEY_UP) {
			cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH]--;
			if (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] < 0)
				cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] = 0;			
			sfx = sfx_select;
		} else if (hDown & KEY_DOWN) {
			cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH]++;
			if (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] > 1)
				cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] = 1;
			sfx = sfx_select;
		} else if (hDown & KEY_B) {		
			subscreenmode = SUBSCREEN_MODE_FRONTEND3;
			sfx = sfx_select;
		} else if (hDown & KEY_A) {
			std::string oldPath = (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] == 0) ? settings.ui.romfolder : settings.ui.fcromfolder;
			std::wstring widestr = (cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] == 0) ? 
													std::wstring(settings.ui.romfolder.begin(), settings.ui.romfolder.end()) : 
													std::wstring(settings.ui.fcromfolder.begin(), settings.ui.fcromfolder.end());
			
			const wchar_t* currentPath = widestr.c_str();
			std::string newPath = keyboardInput(currentPath);
			
			(cursor_pos[SUBSCREEN_MODE_CHANGE_ROM_PATH] == 0) ? settings.ui.romfolder = newPath : settings.ui.fcromfolder = newPath;			

			if(oldPath != newPath){						
				// Buffers for APT_DoApplicationJump().
				u8 param[0x300];
				u8 hmac[0x20];
				// Clear both buffers
				memset(param, 0, sizeof(param));
				memset(hmac, 0, sizeof(hmac));
				
				APT_PrepareToDoApplicationJump(0, 0x00040000047C4200LL, MEDIATYPE_SD);
				// Tell APT to trigger the app launch and set the status of this app to exit
				SaveSettings();
				APT_DoApplicationJump(param, sizeof(param), hmac);
			}
		}
	}

	// Do we need to play a sound effect?
	if (dspfirmfound && sfx) {
		sfx->stop();	// Prevent freezing
		sfx->play();
	}

	// Bottom screen needs to be redrawn.
	return true;
}

/**
 * Load the primary color from the configuration.
 */
void LoadColor(void) {
	static const ColorData colors[] = {
		{
			"romfs:/graphics/topbg/0-gray.png",
			"romfs:/graphics/dotcircle/0-gray.png",
			"romfs:/graphics/start_border/0-gray.png",
			(u32)RGBA8(99, 127, 127, 255)
		},
		{
			"romfs:/graphics/topbg/1-brown.png",
			"romfs:/graphics/dotcircle/1-brown.png",
			"romfs:/graphics/start_border/1-brown.png",
			(u32)RGBA8(139, 99, 0, 255)
		},
		{
			"romfs:/graphics/topbg/2-red.png",
			"romfs:/graphics/dotcircle/2-red.png",
			"romfs:/graphics/start_border/2-red.png",
			(u32)RGBA8(255, 0, 0, 255)
		},
		{
			"romfs:/graphics/topbg/3-pink.png",
			"romfs:/graphics/dotcircle/3-pink.png",
			"romfs:/graphics/start_border/3-pink.png",
			(u32)RGBA8(255, 127, 127, 255)
		},
		{
			"romfs:/graphics/topbg/4-orange.png",
			"romfs:/graphics/dotcircle/4-orange.png",
			"romfs:/graphics/start_border/4-orange.png",
			(u32)RGBA8(223, 63, 0, 255)
		},
		{
			"romfs:/graphics/topbg/5-yellow.png",
			"romfs:/graphics/dotcircle/5-yellow.png",
			"romfs:/graphics/start_border/5-yellow.png",
			(u32)RGBA8(215, 215, 0, 255)
		},
		{
			"romfs:/graphics/topbg/6-yellowgreen.png",
			"romfs:/graphics/dotcircle/6-yellowgreen.png",
			"romfs:/graphics/start_border/6-yellowgreen.png",
			(u32)RGBA8(215, 255, 0, 255)
		},
		{
			"romfs:/graphics/topbg/7-green1.png",
			"romfs:/graphics/dotcircle/7-green1.png",
			"romfs:/graphics/start_border/7-green1.png",
			(u32)RGBA8(0, 255, 0, 255)
		},
		{
			"romfs:/graphics/topbg/8-green2.png",
			"romfs:/graphics/dotcircle/8-green2.png",
			"romfs:/graphics/start_border/8-green2.png",
			(u32)RGBA8(63, 255, 63, 255)
		},
		{
			"romfs:/graphics/topbg/9-lightgreen.png",
			"romfs:/graphics/dotcircle/9-lightgreen.png",
			"romfs:/graphics/start_border/9-lightgreen.png",
			(u32)RGBA8(31, 231, 31, 255)
		},
		{
			"romfs:/graphics/topbg/10-skyblue.png",
			"romfs:/graphics/dotcircle/10-skyblue.png",
			"romfs:/graphics/start_border/10-skyblue.png",
			(u32)RGBA8(0, 63, 255, 255)
		},
		{
			"romfs:/graphics/topbg/11-lightblue.png",
			"romfs:/graphics/dotcircle/11-lightblue.png",
			"romfs:/graphics/start_border/11-lightblue.png",
			(u32)RGBA8(63, 63, 255, 255)
		},
		{
			"romfs:/graphics/topbg/12-blue.png",
			"romfs:/graphics/dotcircle/12-blue.png",
			"romfs:/graphics/start_border/12-blue.png",
			(u32)RGBA8(0, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/13-violet.png",
			"romfs:/graphics/dotcircle/13-violet.png",
			"romfs:/graphics/start_border/13-violet.png",
			(u32)RGBA8(127, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/14-purple.png",
			"romfs:/graphics/dotcircle/14-purple.png",
			"romfs:/graphics/start_border/14-purple.png",
			(u32)RGBA8(255, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/15-fuchsia.png",
			"romfs:/graphics/dotcircle/15-fuchsia.png",
			"romfs:/graphics/start_border/15-fuchsia.png",
			(u32)RGBA8(255, 0, 127, 255)
		},
		{
			"romfs:/graphics/topbg/16-red_and_blue.png",
			"romfs:/graphics/dotcircle/16-red_and_blue.png",
			"romfs:/graphics/start_border/16-red_and_blue.png",
			(u32)RGBA8(255, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/17-green_and_yellow.png",
			"romfs:/graphics/dotcircle/17-green_and_yellow.png",
			"romfs:/graphics/start_border/17-green_and_yellow.png",
			(u32)RGBA8(215, 215, 0, 255)
		},
		{
			"romfs:/graphics/topbg/18-christmas.png",
			"romfs:/graphics/dotcircle/18-christmas.png",
			"romfs:/graphics/start_border/18-christmas.png",
			(u32)RGBA8(255, 255, 0, 255)
		},
	};

	if (settings.ui.color < 0 || settings.ui.color > 18)
		settings.ui.color = 0;
	color_data = &colors[settings.ui.color];
	if (logEnabled)	LogFM("LoadColor()", "Colors load successfully");
}

/**
 * Load the menu color from the configuration.
 */
void LoadMenuColor(void) {
	static const u32 menu_colors[] = {
		(u32)RGBA8(255, 255, 255, 255),		// White
		(u32)RGBA8(63, 63, 63, 195),		// Black
		(u32)RGBA8(139, 99, 0, 195),		// Brown
		(u32)RGBA8(255, 0, 0, 195),		// Red
		(u32)RGBA8(255, 163, 163, 195),		// Pink
		(u32)RGBA8(255, 127, 0, 223),		// Orange
		(u32)RGBA8(255, 255, 0, 223),		// Yellow
		(u32)RGBA8(215, 255, 0, 223),		// Yellow-Green
		(u32)RGBA8(0, 255, 0, 223),		// Green 1
		(u32)RGBA8(95, 223, 95, 193),		// Green 2
		(u32)RGBA8(127, 231, 127, 223),		// Light Green
		(u32)RGBA8(63, 127, 255, 223),		// Sky Blue
		(u32)RGBA8(127, 127, 255, 223),		// Light Blue
		(u32)RGBA8(0, 0, 255, 195),		// Blue
		(u32)RGBA8(127, 0, 255, 195),		// Violet
		(u32)RGBA8(255, 0, 255, 195),		// Purple
		(u32)RGBA8(255, 63, 127, 195),		// Fuchsia
	};

	if (settings.ui.menucolor < 0 || settings.ui.menucolor > 16)
		settings.ui.menucolor = 0;
	menucolor = menu_colors[settings.ui.menucolor];
	if (logEnabled)	LogFM("LoadMenuColor()", "Menu color load successfully");
}

/**
 * Load the filename of the bottom screen image.
 */
void LoadBottomImage() {
	bottomloc = "romfs:/graphics/bottom.png";

	if (settings.ui.custombot == 1) {
		if( access( "sdmc:/_nds/twloader/bottom.png", F_OK ) != -1 ) {
			bottomloc = "sdmc:/_nds/twloader/bottom.png";
			if (logEnabled)	LogFM("LoadBottomImage()", "Using custom bottom image. Method load successfully");
		} else {
			bottomloc = "romfs:/graphics/bottom.png";
			if (logEnabled)	LogFM("LoadBottomImage()", "Using default bottom image. Method load successfully");
		}
	}
}

/**
 * Remove trailing slashes from a pathname, if present.
 * @param path Pathname to modify.
 */
static void RemoveTrailingSlashes(string& path)
{
	while (!path.empty() && path[path.size()-1] == '/') {
		path.resize(path.size()-1);
	}
}

/**
 * Load settings.
 */
void LoadSettings(void) {
	// UI settings.
	settings.ui.name = settingsini.GetString("FRONTEND", "NAME", "");
	settings.ui.romfolder = settingsini.GetString("FRONTEND", "ROM_FOLDER", "");
	RemoveTrailingSlashes(settings.ui.romfolder);
	settings.ui.fcromfolder = settingsini.GetString("FRONTEND", "FCROM_FOLDER", "");
	RemoveTrailingSlashes(settings.ui.fcromfolder);
	settings.ui.pagenum = settingsini.GetInt("FRONTEND", "PAGE_NUMBER", 0);
	// settings.ui.cursorPosition = settingsini.GetInt("FRONTEND", "CURSOR_POSITION", 0);

	// Customizable UI settings.
	settings.ui.language = settingsini.GetInt("FRONTEND", "LANGUAGE", -1);
	settings.ui.theme = settingsini.GetInt("FRONTEND", "THEME", 0);
	settings.ui.subtheme = settingsini.GetInt("FRONTEND", "SUB_THEME", 0);
	settings.ui.color = settingsini.GetInt("FRONTEND", "COLOR", 0);
	settings.ui.menucolor = settingsini.GetInt("FRONTEND", "MENU_COLOR", 0);
	settings.ui.filename = settingsini.GetInt("FRONTEND", "SHOW_FILENAME", 0);
	settings.ui.topborder = settingsini.GetInt("FRONTEND", "TOP_BORDER", 1);
	settings.ui.iconsize = settingsini.GetInt("FRONTEND", "ICON_SIZE", 0);
	settings.ui.counter = settingsini.GetInt("FRONTEND", "COUNTER", 0);
	settings.ui.custombot = settingsini.GetInt("FRONTEND", "CUSTOM_BOTTOM", 0);
	settings.romselect.toplayout = settingsini.GetInt("FRONTEND", "TOP_LAYOUT", 0);
	if (!isDemo) settings.ui.filetype = settingsini.GetInt("FRONTEND", "FILETYPE", 0);
	else settings.ui.filetype = 1;
	settings.ui.showbootscreen = settingsini.GetInt("FRONTEND", "SHOW_BOOT_ANIMATION", 2);
	settings.ui.bootscreen = settingsini.GetInt("FRONTEND", "BOOT_ANIMATION", 0);
	settings.ui.healthsafety = settingsini.GetInt("FRONTEND", "HEALTH&SAFETY_MSG", 1);
	settings.ui.autoupdate = settingsini.GetInt("FRONTEND", "AUTOUPDATE", 0);
	settings.ui.autoupdate_twldr = settingsini.GetInt("FRONTEND", "AUTODOWNLOAD", 0);
	// romselect_layout = settingsini.GetInt("FRONTEND", "BOTTOM_LAYOUT", 0);

	// TWL settings.
	settings.twl.rainbowled = settingsini.GetInt("TWL-MODE", "RAINBOW_LED", 0);
	settings.twl.cpuspeed = settingsini.GetInt("TWL-MODE", "TWL_CLOCK", 0);
	settings.twl.lockarm9scfgext = settingsini.GetInt("TWL-MODE", "LOCK_ARM9_SCFG_EXT", 0);
	settings.twl.resetslot1 = settingsini.GetInt("TWL-MODE", "RESET_SLOT1", 0);
	settings.twl.enablesd = settingsini.GetInt("TWL-MODE", "SLOT1_ENABLESD", 0);
	settings.twl.forwarder = settingsini.GetInt("TWL-MODE", "FORWARDER", 0);
	settings.twl.flashcard = settingsini.GetInt("TWL-MODE", "FLASHCARD", 0);
	settings.twl.bootstrapfile = settingsini.GetInt("TWL-MODE", "BOOTSTRAP_FILE", 0);
	if (settings.twl.bootstrapfile == 2) fat = "fat:/";
	else fat = "sd:/";

	// TODO: Change the default to -1?
	switch (settingsini.GetInt("TWL-MODE", "DEBUG", 0)) {
		case 1:
			settings.twl.console = 2;
			break;
		case 0:
		default:
			settings.twl.console = 1;
			break;
		case -1:
			settings.twl.console = 0;
			break;
	}
	if (logEnabled)	LogFM("Settings.LoadSettings", "Settings loaded successfully");
}

/**
 * Save settings.
 */
void SaveSettings(void) {
	// UI settings.
	if (!gbarunnervalue) settingsini.SetString("FRONTEND", "ROM_FOLDER", settings.ui.romfolder);
	if (!gbarunnervalue) settingsini.SetString("FRONTEND", "FCROM_FOLDER", settings.ui.fcromfolder);
	settingsini.SetInt("FRONTEND", "PAGE_NUMBER", settings.ui.pagenum);
	// settingsini.SetInt("FRONTEND", "CURSOR_POSITION", settings.ui.cursorPosition);
	settingsini.SetInt("FRONTEND", "LANGUAGE", settings.ui.language);
	settingsini.SetInt("FRONTEND", "THEME", settings.ui.theme);
	settingsini.SetInt("FRONTEND", "SUB_THEME", settings.ui.subtheme);
	settingsini.SetInt("FRONTEND", "COLOR", settings.ui.color);
	settingsini.SetInt("FRONTEND", "MENU_COLOR", settings.ui.menucolor);
	settingsini.SetInt("FRONTEND", "SHOW_FILENAME", settings.ui.filename);
	settingsini.SetInt("FRONTEND", "TOP_BORDER", settings.ui.topborder);
	settingsini.SetInt("FRONTEND", "ICON_SIZE", settings.ui.iconsize);
	settingsini.SetInt("FRONTEND", "COUNTER", settings.ui.counter);
	settingsini.SetInt("FRONTEND", "CUSTOM_BOTTOM", settings.ui.custombot);
	settingsini.SetInt("FRONTEND", "TOP_LAYOUT", settings.romselect.toplayout);
	if (!isDemo) settingsini.SetInt("FRONTEND", "FILETYPE", settings.ui.filetype);
	if (dspfirmfound) settingsini.SetInt("FRONTEND", "SHOW_BOOT_ANIMATION", settings.ui.showbootscreen);
	settingsini.SetInt("FRONTEND", "BOOT_ANIMATION", settings.ui.bootscreen);
	settingsini.SetInt("FRONTEND", "HEALTH&SAFETY_MSG", settings.ui.healthsafety);
	settingsini.SetInt("FRONTEND", "AUTOUPDATE", settings.ui.autoupdate);
	settingsini.SetInt("FRONTEND", "AUTODOWNLOAD", settings.ui.autoupdate_twldr);
	//settingsini.SetInt("FRONTEND", "BOTTOM_LAYOUT", romselect_layout);

	// TWL settings.
	settingsini.SetInt("TWL-MODE", "RAINBOW_LED", settings.twl.rainbowled);
	settingsini.SetInt("TWL-MODE", "TWL_CLOCK", settings.twl.cpuspeed);
	settingsini.SetInt("TWL-MODE", "LOCK_ARM9_SCFG_EXT", settings.twl.lockarm9scfgext);
	settingsini.SetInt("TWL-MODE", "LAUNCH_SLOT1", settings.twl.launchslot1);
	settingsini.SetInt("TWL-MODE", "RESET_SLOT1", settings.twl.resetslot1);
	settingsini.SetInt("TWL-MODE", "SLOT1_ENABLESD", settings.twl.enablesd);
	settingsini.SetInt("TWL-MODE", "SLOT1_KEEPSD", keepsdvalue);
	settingsini.SetInt("TWL-MODE", "BOOTSTRAP_FILE", settings.twl.bootstrapfile);
	if (settings.twl.bootstrapfile == 2) fat = "fat:/";
	else fat = "sd:/";

	// TODO: Change default to 0?
	switch (settings.twl.console) {
		case 0:
			settingsini.SetInt("TWL-MODE", "DEBUG", -1);
			break;
		case 1:
		default:
			settingsini.SetInt("TWL-MODE", "DEBUG", 0);
			break;
		case 2:
			settingsini.SetInt("TWL-MODE", "DEBUG", 1);
			break;
	}

	settingsini.SetInt("TWL-MODE", "FORWARDER", settings.twl.forwarder);
	settingsini.SetInt("TWL-MODE", "FLASHCARD", settings.twl.flashcard);
	settingsini.SetInt("TWL-MODE", "GBARUNNER", gbarunnervalue);
	settingsini.SaveIniFile("sdmc:/_nds/twloader/settings.ini");
}
