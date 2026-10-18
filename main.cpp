#include <prism/framerateselectscreen.h>
#include <prism/windowfocusscreen.h>
#include <prism/physics.h>
#include <prism/file.h>
#include <prism/drawing.h>
#include <prism/log.h>
#include <prism/wrapper.h>
#include <prism/system.h>
#include <prism/stagehandler.h>
#include <prism/mugentexthandler.h>
#include <prism/debug.h>
#include <prism/soundeffect.h>
#include <prism/sound.h>

#include "gamescreen.h"
#include "storyscreen.h"

#ifdef DREAMCAST
KOS_INIT_FLAGS(INIT_DEFAULT);

#endif

// #define DEVELOP

void exitGame() {
	shutdownPrismWrapper();

#ifdef DEVELOP
	if (isOnDreamcast()) {
		abortSystem();
	}
	else {
		returnToMenu();
	}
#else
	returnToMenu();
#endif
}

void setGameVolumes() {
	if (isOnDreamcast())
	{
		setSoundEffectVolume(1.0);
	}
	else
	{
		setVolume(0.25);
		setSoundEffectVolume(0.5);
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	#ifdef DEVELOP
	setDevelopMode();
	#endif

	setGameName("The Red Baron Takes Flight Again");
	setScreenSize(320, 240);
	
	initPrismWrapperWithConfigFile("data/config.cfg");
	setFont("$/rd/fonts/segoe.hdr", "$/rd/fonts/segoe.pkg");

	addMugenFont(-1, "font/f4x6.fnt");
	addMugenFont(1, "font/f6x9.fnt");
	addMugenFont(2, "font/jg.fnt");

	logg("Check framerate");
	if (selectFramerate() == FRAMERATE_SCREEN_RETURN_ABORT) {
		exitGame();
	}

	if(isInDevelopMode()) {
		disableWrapperErrorRecovery();	
		setMinimumLogType(LOG_TYPE_NORMAL);
	}
	else {
		setMinimumLogType(LOG_TYPE_NONE);
	}

	setGameVolumes();
	setCurrentStoryDefinitionFile("game/INTRO.def", 0);
	resetGame();
	const auto startScreen = getStoryScreen();
#ifdef __EMSCRIPTEN__
	setWindowFocusScreenNextScreen(startScreen);
	startScreenHandling(getWindowFocusScreen());
#else
	startScreenHandling(startScreen);
#endif

	exitGame();
	
	return 0;
}


