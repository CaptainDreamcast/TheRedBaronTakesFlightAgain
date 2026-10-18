#include "creditsscreen.h"

#include <prism/blitz.h>
#include <prism/soundeffect.h>

#include "storyscreen.h"

using namespace prism;

static void goToIntro(void* tCaller);

class CreditsScreen
{
public:
    const std::vector<std::string> mCreditLines = {
        "Credits:",
        "",
        "Red Baron - You",
        "Blue Peasant - Guy you don't like",
        "Aliens - Don't worry about it",
        "",
        "Amazing Player - You",
    };
    const int creditsFont = 2;
    const double lineHeight = 16;
    const double creditsVoiceDurationSeconds = 18.85;
    const double firstLineStartY = 248;
    const double lastLineEndY = -16;
    const Vector3D finPosition = Vector3D(312, 230, 10);
    const int minimumTicksBeforeLeaving = 30;

    MugenSounds mVoiceSounds;
    std::vector<int> mLineTextIds;
    double mScrollOffset = 0;
    double mScrollSpeed = 0;
    int mTicks = 0;
    bool mIsCrawlOver = false;
    bool mIsLeaving = false;

    CreditsScreen() {
        load();
    }

    void load() {
        loadVoice();
        loadLines();
        loadScrollSpeed();
        startCreditsAudio();
    }

    void update() {
        mTicks++;
        updateScrolling();
        updateCrawlOver();
        updateLeaving();
    }

    // LOADING
    void loadVoice() {
        mVoiceSounds = loadMugenSoundFile(isOnDreamcast() ? "game/dreamcast/CREDITS.snd" : "game/CREDITS.snd");
    }

    void loadLines() {
        for (size_t i = 0; i < mCreditLines.size(); i++)
        {
            mLineTextIds.push_back(addCenteredCreditLine(mCreditLines[i], i));
        }
    }
    int addCenteredCreditLine(const std::string& tLine, size_t tIndex) {
        const auto id = addMugenText(tLine.c_str(), getCreditLinePosition(tIndex), creditsFont);
        setMugenTextAlignment(id, MUGEN_TEXT_ALIGNMENT_CENTER);
        return id;
    }

    void loadScrollSpeed() {
        mScrollSpeed = getTotalScrollDistance() / getCreditsDurationTicks();
    }
    double getTotalScrollDistance() {
        return getUnscrolledLineY(mCreditLines.size() - 1) - lastLineEndY;
    }
    double getCreditsDurationTicks() {
        return creditsVoiceDurationSeconds * 60;
    }

    void startCreditsAudio() {
        setSoundEffectVolume(1.0);
        tryPlayMugenSound(&mVoiceSounds, 1, 0);
    }

    // SCROLLING
    void updateScrolling() {
        if (mIsCrawlOver) return;
        mScrollOffset += mScrollSpeed;
        for (size_t i = 0; i < mLineTextIds.size(); i++)
        {
            setMugenTextPosition(mLineTextIds[i], getCreditLinePosition(i));
        }
    }
    Vector3D getCreditLinePosition(size_t tIndex) {
        return Vector3D(160, getUnscrolledLineY(tIndex) - mScrollOffset, 10);
    }
    double getUnscrolledLineY(size_t tIndex) {
        return firstLineStartY + tIndex * lineHeight;
    }

    // ENDING
    void updateCrawlOver() {
        if (mIsCrawlOver) return;
        if (mScrollOffset < getTotalScrollDistance()) return;
        mIsCrawlOver = true;
        showFin();
    }
    void showFin() {
        const auto id = addMugenText("FIN", finPosition, creditsFont);
        setMugenTextAlignment(id, MUGEN_TEXT_ALIGNMENT_RIGHT);
    }
    void updateLeaving() {
        if (mIsLeaving) return;
        if (mTicks < minimumTicksBeforeLeaving) return;
        if (!hasPressedStartFlank()) return;
        mIsLeaving = true;
        addFadeOut(30, goToIntro, nullptr);
    }
};

EXPORT_SCREEN_CLASS(CreditsScreen);

static void goToIntro(void* tCaller) {
    (void)tCaller;
    stopAllSoundEffects();
    stopStreamingMusicFile();
    setCurrentStoryDefinitionFile("game/INTRO.def", 0);
    setNewScreen(getStoryScreen());
}
