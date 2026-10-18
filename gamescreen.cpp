#include "gamescreen.h"

#include <algorithm>
#include <cmath>

#include <prism/numberpopuphandler.h>
#include <prism/animation.h>
#include <prism/screeneffect.h>
#include <prism/soundeffect.h>
#include "storyscreen.h"

static struct
{
    int mLevel = 0;
    int mGameTicks;
    int mScore = 0;
    bool mIsRetryingBoss = false;
    int mBossRetryAmount = 0;
    bool mHasShownTutorial = false;
} gGameScreenData;

class GameScreen
{
public:
    GameScreen() {
        instantiateActor(getPrismNumberPopupHandler());
        load();
    }

    double sfxVol = 0.2;
    double jingleVol = 1.0;

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    void load() {
        loadFiles();
        loadGeneral();
        loadBG();
        loadPlayer();
        loadEnemyHandler();
        loadInterlude();
        loadTransmission();
        loadBoss();
        loadBossHealthBar();
        loadShotHandler();
        loadScoreUI();
        loadGetReady();
        loadVictory();
        loadLoss();
        loadTutorial();
        loadBossRetry();
    }

    void update() {
        updateBG();
        updateTutorial();
        updateGetReady();

        if (!isPaused())
        {
            updateEnemyHandler();
            updateInterlude();
            updateTransmission();
            updateBoss();
            updateShotHandler();
        }

        updatePlayer();
        updateScoreUI();
        updateBossHealthBar();
        updateVictory();
        updateLoss();
    }

    // GENERAL
    void loadGeneral() {
        loadCollisions();
    }
    CollisionListData* playerCollisionListShots;
    CollisionListData* enemyShotCollisionList;
    CollisionListData* enemyCollisionListShots;
    CollisionListData* playerShotCollisionList;
    CollisionListData* shootableEnemyShotCollisionList;

    void loadCollisions() {
        playerCollisionListShots = addCollisionListToHandler();
        enemyShotCollisionList = addCollisionListToHandler();
        enemyCollisionListShots = addCollisionListToHandler();
        playerShotCollisionList = addCollisionListToHandler();
        shootableEnemyShotCollisionList = addCollisionListToHandler();
        addCollisionHandlerCheck(playerCollisionListShots, enemyShotCollisionList);
        addCollisionHandlerCheck(shootableEnemyShotCollisionList, playerShotCollisionList);
        addCollisionHandlerCheck(enemyCollisionListShots, playerShotCollisionList);
        addCollisionHandlerCheck(playerCollisionListShots, enemyCollisionListShots);
    }

    bool isPaused()
    {
        return isShowingLoss || isShowingGetReady || isShowingVictory || isShowingTutorial;
    }

    bool hasPressedContinue()
    {
        return hasPressedStartFlank() || hasPressedAFlank();
    }

    // BG
    std::vector<int> mBGEntities;
    int planetsEntityId;
    double starSpeed = 2.0;
    double planetsStartY = -1680;
    double planetsEndY = 120;
    void loadBG() {
        loadStars();
        loadPlanets();
    }
    void loadStars() {
        mBGEntities.resize(2);
        mBGEntities[0] = addBlitzEntity(Vector3D(0, 0, 1));
        addBlitzMugenAnimationComponent(mBGEntities[0], &mSprites, &mAnimations, 1);
        mBGEntities[1] = addBlitzEntity(Vector3D(0, -240, 1));
        addBlitzMugenAnimationComponent(mBGEntities[1], &mSprites, &mAnimations, 1);
    }
    void loadPlanets() {
        planetsEntityId = addBlitzEntity(Vector3D(0, planetsStartY, 2));
        addBlitzMugenAnimationComponent(planetsEntityId, &mSprites, &mAnimations, 2);
    }
    void updateBG() {
        if (isPaused()) return;
        updateStarMovement();
        updatePlanetsMovement();
    }

    void updateStarMovement() {
        auto pos1 = getBlitzEntityPositionReference(mBGEntities[0]);
        auto pos2 = getBlitzEntityPositionReference(mBGEntities[1]);
        pos1->y += starSpeed;
        pos2->y += starSpeed;
        if (pos1->y >= 240)
        {
            pos1->y -= 480;
            std::swap(mBGEntities[0], mBGEntities[1]);
        }
    }

    void updatePlanetsMovement() {
        auto pos = getBlitzEntityPositionReference(planetsEntityId);
        pos->y = planetsStartY + (planetsEndY - planetsStartY) * getFloodProgress();
    }

    // PLAYER
    int playerEntityId;
    int playerHitboxEntityId;
    int playerCollisionId;
    int shotCooldown = 0;
    int playerShotInterval = 6;
    int playerIsDying = 0;
    void loadPlayer() {
        playerEntityId = addBlitzEntity(Vector3D(160, 200, 10));
        addBlitzMugenAnimationComponent(playerEntityId, &mSprites, &mAnimations, 10);
        playerCollisionId = addBlitzCollisionRect(playerEntityId, playerCollisionListShots, CollisionRect(-2, -2, 4, 4));
        loadPlayerHitboxIndicator();
    }
    void loadPlayerHitboxIndicator() {
        playerHitboxEntityId = addBlitzEntity(Vector3D(160, 200, 11));
        addBlitzMugenAnimationComponent(playerHitboxEntityId, &mSprites, &mAnimations, 14);
        setBlitzMugenAnimationVisibility(playerHitboxEntityId, 0);
    }
    void updatePlayer() {
        if (isPaused()) return;

        gGameScreenData.mGameTicks++;
        updatePlayerMovement();
        updatePlayerHitboxIndicator();
        updatePlayerShoot();
        updatePlayerGetHit();
    }
    void updatePlayerHitboxIndicator() {
        setBlitzEntityPosition(playerHitboxEntityId, getBlitzEntityPosition(playerEntityId).xy().xyz(11));
        setBlitzMugenAnimationVisibility(playerHitboxEntityId, isPlayerHitboxIndicatorVisible());
    }
    bool isPlayerHitboxIndicatorVisible() {
        return isPlayerFocusing() && !playerIsDying;
    }
    void updatePlayerGetHit() {
        if (playerIsDying) return;
        if (bossIsDying) return;
        if (hasBlitzCollidedThisFrame(playerEntityId, playerCollisionId))
        {
            startPlayerDeath();
        }
    }
    void startPlayerDeath() {
        tryPlayMugenSoundAdvanced(&mSounds, 1, 4, sfxVol);
        setBlitzMugenAnimationVisibility(playerEntityId, 0);
        playPlayerExplosion();
        playerIsDying = 1;
    }
    void playPlayerExplosion() {
        auto explosionEntity = addBlitzEntity(getBlitzEntityPosition(playerEntityId).xy().xyz(45));
        addBlitzMugenAnimationComponent(explosionEntity, &mSprites, &mAnimations, 13);
        setBlitzMugenAnimationNoLoop(explosionEntity);
    }

    void updatePlayerMovement() {
        if (playerIsDying) return;
        auto pos = getBlitzEntityPositionReference(playerEntityId);
        Vector2D dir = Vector2D(0, 0);

        if (hasPressedLeft())
        {
            dir.x = -1;
        }
        if (hasPressedRight())
        {
            dir.x = 1;
        }
        if (hasPressedUp())
        {
            dir.y = -1;
        }
        if (hasPressedDown())
        {
            dir.y = 1;
        }

        if (dir.x == 0)
        {
            changeBlitzMugenAnimationIfDifferent(playerEntityId, 10);
        }
        else if (dir.x < 0)
        {
            changeBlitzMugenAnimationIfDifferent(playerEntityId, 11);
        }
        else
        {
            changeBlitzMugenAnimationIfDifferent(playerEntityId, 12);
        }

        if (dir.x == 0 && dir.y == 0)
        {
            return;
        }

        *pos += vecNormalize(dir) * getPlayerMovementSpeed();
        *pos = clampPositionToGeoRectangle(*pos, GeoRectangle2D(8, 24, 304, 208));
    }
    double getPlayerMovementSpeed() {
        return isPlayerFocusing() ? 1.0 : 2.4;
    }
    bool isPlayerFocusing() {
        return hasPressedRawKeyboardKey(KEYBOARD_SHIFT_LEFT_PRISM) || hasPressedL() || hasPressedR();
    }
    void updatePlayerShoot()
    {
        if (playerIsDying) return;
        if (shotCooldown)
        {
            shotCooldown--;
            return;
        }

        if (hasPressedA())
        {
            addPlayerShot();
            tryPlayMugenSoundAdvanced(&mSounds, 1, 1, sfxVol);
            shotCooldown = playerShotInterval;
        }
    }

    // ENEMY FLOOD SCHEDULE
    struct EnemySpawn
    {
        int tick;
        Vector2D position;
        Vector2D velocity;
    };
    std::vector<EnemySpawn> mFloodSchedule;
    size_t nextFloodSpawn = 0;
    int floodTicks = 0;
    int floodEndTick = 1;

    void loadFloodSchedule() {
        addWallWave(60);
        addWallWave(140);
        addWallWave(220);
        addWallWave(300);
        addRushWave(460, 50, 6, 1.5, 2.4);
        finalizeFloodSchedule();
    }
    void addEnemySpawn(int tTick, const Vector2D& tPosition, const Vector2D& tVelocity) {
        mFloodSchedule.push_back(EnemySpawn{ tTick, tPosition, tVelocity });
    }
    void addWallWave(int tStartTick) {
        const auto amount = 9;
        const auto spacing = 320.0 / amount;
        for (int i = 0; i < amount; i++)
        {
            addEnemySpawn(tStartTick, Vector2D(spacing / 2 + i * spacing, -30), Vector2D(0, 0.9));
        }
    }
    void addRushWave(int tStartTick, int tAmount, int tInterval, double tMinSpeed, double tMaxSpeed) {
        for (int i = 0; i < tAmount; i++)
        {
            addEnemySpawn(tStartTick + i * tInterval, Vector2D(randfrom(24, 296), -30), Vector2D(0, randfrom(tMinSpeed, tMaxSpeed)));
        }
    }
    void finalizeFloodSchedule() {
        std::stable_sort(mFloodSchedule.begin(), mFloodSchedule.end(), [](const EnemySpawn& a, const EnemySpawn& b) { return a.tick < b.tick; });
        floodEndTick = mFloodSchedule.back().tick;
    }
    bool hasSpawnedWholeFlood() {
        return nextFloodSpawn >= mFloodSchedule.size();
    }
    double getFloodProgress() {
        return std::min(double(1.0), double(floodTicks) / floodEndTick);
    }

    // ENEMY HANDLER
    struct Enemy
    {
        int entityId;
        int activeCollision;
        Vector2D spawnPosition;
        Vector2D velocity;
        int ticks = 0;
        bool isToBeRemoved = false;
    };
    std::map<int, Enemy> mEnemies;

    void loadEnemyHandler() {
        loadFloodSchedule();
    }
    void updateEnemyHandler() {
        updateEnemySpawning();
        updateEnemyHandlerActive();
    }
    void updateEnemySpawning() {
        if (bossStarted) return;

        floodTicks++;
        while (!hasSpawnedWholeFlood() && mFloodSchedule[nextFloodSpawn].tick <= floodTicks)
        {
            addEnemy(mFloodSchedule[nextFloodSpawn]);
            nextFloodSpawn++;
        }
    }
    void updateEnemyHandlerActive() {
        auto it = mEnemies.begin();
        while (it != mEnemies.end())
        {
            updateSingleEnemy(it->second);
            if (it->second.isToBeRemoved)
            {
                unloadEnemy(it->second);
                it = mEnemies.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void removeAllEnemies()
    {
        auto it = mEnemies.begin();
        while (it != mEnemies.end())
        {
            unloadEnemy(it->second);
            it = mEnemies.erase(it);
        }
    }

    void setPlayerIdle()
    {
        changeBlitzMugenAnimationIfDifferent(playerEntityId, 10);
    }

    // SINGLE ENEMY
    void addEnemy(const EnemySpawn& tSpawn) {
        auto entityId = addBlitzEntity(tSpawn.position.xyz(15));
        addBlitzMugenAnimationComponent(entityId, &mSprites, &mAnimations, 30);
        auto activeCollision = addBlitzCollisionRect(entityId, enemyCollisionListShots, CollisionRect(-12, -12, 24, 24));
        Enemy e;
        e.entityId = entityId;
        e.activeCollision = activeCollision;
        e.spawnPosition = tSpawn.position;
        e.velocity = tSpawn.velocity;
        mEnemies[entityId] = e;
    }
    void unloadEnemy(Enemy& e) {
        removeBlitzEntity(e.entityId);
    }
    void updateSingleEnemy(Enemy& e) {
        updateEnemyMovement(e);
        updateEnemyRemove(e);
    }

    void updateEnemyMovement(Enemy& e) {
        e.ticks++;
        auto pos = getBlitzEntityPositionReference(e.entityId);
        *pos = (e.spawnPosition + e.velocity * e.ticks).xyz(pos->z);
    }

    void updateEnemyRemove(Enemy& e)
    {
        auto pos = getBlitzEntityPositionReference(e.entityId);
        if (hasBlitzCollidedThisFrame(e.entityId, e.activeCollision))
        {
            destroyEnemy(e, pos->xy());
            return;
        }

        if (isEnemyOffScreen(pos->xy()))
        {
            e.isToBeRemoved = true;
        }
    }
    void destroyEnemy(Enemy& e, const Vector2D& tPosition) {
        tryPlayMugenSoundAdvanced(&mSounds, 1, 3, sfxVol);
        gGameScreenData.mScore += 300;
        addPrismNumberPopup(300, tPosition.xyz(50), 1, Vector3D(0, -1, 0), 2.0, 2, 20);
        playExplosion(tPosition, 1.0);
        e.isToBeRemoved = true;
    }
    bool isEnemyOffScreen(const Vector2D& tPosition) {
        return tPosition.y > 270 || tPosition.x < -60 || tPosition.x > 380;
    }

    // INTERLUDE
    enum class InterludePhase { WAITING, FADING_IN, WRITING, READING, FADING_OUT, OVER };
    InterludePhase interludePhase = InterludePhase::WAITING;
    MugenAnimationHandlerElement* interludeBoxElement;
    int interludeTextId = -1;
    int interludePhaseTicks = 0;
    int interludeFadeTicks = 20;
    int interludeReadingTicks = 150;
    int interludeLetterDuration = 2;
    Vector3D interludeBoxPosition = Vector3D(160, 110, 52);
    void loadInterlude() {
        interludeBoxElement = addMugenAnimation(getMugenAnimation(&mAnimations, 68), &mSprites, interludeBoxPosition);
        setMugenAnimationTransparency(interludeBoxElement, 0);
    }
    void updateInterlude() {
        interludePhaseTicks++;
        updateInterludeStart();
        updateInterludeFadeIn();
        updateInterludeWriting();
        updateInterludeReading();
        updateInterludeFadeOut();
    }
    void updateInterludeStart() {
        if (interludePhase != InterludePhase::WAITING) return;
        if (!hasSpawnedWholeFlood()) return;
        if (!mEnemies.empty()) return;
        setInterludePhase(InterludePhase::FADING_IN);
    }
    void updateInterludeFadeIn() {
        if (interludePhase != InterludePhase::FADING_IN) return;
        setMugenAnimationTransparency(interludeBoxElement, getInterludeFadeProgress());
        if (interludePhaseTicks < interludeFadeTicks) return;
        addInterludeText();
        setInterludePhase(InterludePhase::WRITING);
    }
    void addInterludeText() {
        interludeTextId = addMugenText("WE SURRENDER, YOU ARE JUST TOO RED FOR US TO BEAT! WE LEARNED ENGLISH JUST TO SAY THIS!", getInterludeTextPosition(), 1);
        setMugenTextColor(interludeTextId, COLOR_BLACK);
        setMugenTextTextBoxWidth(interludeTextId, 206);
        setMugenTextBuildup(interludeTextId, interludeLetterDuration);
    }
    Vector3D getInterludeTextPosition() {
        return interludeBoxPosition + Vector3D(-103, -4, 1);
    }
    void updateInterludeWriting() {
        if (interludePhase != InterludePhase::WRITING) return;
        if (!isMugenTextBuiltUp(interludeTextId)) return;
        setInterludePhase(InterludePhase::READING);
    }
    void updateInterludeReading() {
        if (interludePhase != InterludePhase::READING) return;
        if (interludePhaseTicks < interludeReadingTicks) return;
        removeMugenText(interludeTextId);
        setInterludePhase(InterludePhase::FADING_OUT);
    }
    void updateInterludeFadeOut() {
        if (interludePhase != InterludePhase::FADING_OUT) return;
        setMugenAnimationTransparency(interludeBoxElement, 1.0 - getInterludeFadeProgress());
        if (interludePhaseTicks < interludeFadeTicks) return;
        setInterludePhase(InterludePhase::OVER);
    }
    double getInterludeFadeProgress() {
        return std::min(double(1.0), double(interludePhaseTicks) / interludeFadeTicks);
    }
    void setInterludePhase(InterludePhase tPhase) {
        interludePhase = tPhase;
        interludePhaseTicks = 0;
    }
    bool isInterludeOver() {
        return interludePhase == InterludePhase::OVER;
    }

    // BLUE PEASANT
    enum class TransmissionPhase { WAITING, FADING_IN, TALKING, FADING_OUT, OVER };
    TransmissionPhase transmissionPhase = TransmissionPhase::WAITING;
    MugenSounds mTransmissionSounds;
    MugenAnimationHandlerElement* mugshotElement;
    std::vector<int> subtitleLineTextIds;
    int speakerNameTextId = -1;
    int transmissionVoiceChannel = -1;
    int transmissionPhaseTicks = 0;
    int transmissionFadeTicks = 20;
    int transmissionMinimumTalkingTicks = 550;
    Vector3D mugshotPosition = Vector3D(160, 100, 52);
    Vector3D subtitleFirstLinePosition = Vector3D(160, 190, 53);
    double subtitleLineHeight = 11;
    Vector3D speakerNamePosition = Vector3D(160, 167, 53);
    double speakerNameScale = 2;
    size_t subtitleMaximumLineLength = 33;
    void loadTransmission() {
        mTransmissionSounds = loadMugenSoundFile(isOnDreamcast() ? "game/dreamcast/BLUE_PEASANT.snd" : "game/BLUE_PEASANT.snd");
        mugshotElement = addMugenAnimation(getMugenAnimation(&mAnimations, 69), &mSprites, mugshotPosition);
        setMugenAnimationTransparency(mugshotElement, 0);
    }
    void updateTransmission() {
        transmissionPhaseTicks++;
        updateTransmissionStart();
        updateTransmissionFadeIn();
        updateTransmissionTalking();
        updateTransmissionFadeOut();
    }
    void updateTransmissionStart() {
        if (transmissionPhase != TransmissionPhase::WAITING) return;
        if (!isInterludeOver()) return;
        stopStreamingMusicFile();
        setTransmissionPhase(TransmissionPhase::FADING_IN);
    }
    void updateTransmissionFadeIn() {
        if (transmissionPhase != TransmissionPhase::FADING_IN) return;
        setMugenAnimationTransparency(mugshotElement, getTransmissionFadeProgress());
        if (transmissionPhaseTicks < transmissionFadeTicks) return;
        startTransmissionVoice();
        addSpeakerName();
        addSubtitles();
        setTransmissionPhase(TransmissionPhase::TALKING);
    }
    void startTransmissionVoice() {
        transmissionVoiceChannel = playMugenSoundAdvanced(&mTransmissionSounds, 1, 0, jingleVol, -1, 1.0, 0, 0.0);
    }
    void addSpeakerName() {
        speakerNameTextId = addMugenText("BLUE PEASANT", speakerNamePosition, 2);
        setMugenTextScale(speakerNameTextId, speakerNameScale);
        setMugenTextColorRGB(speakerNameTextId, 0.3, 0.45, 1.0);
        setMugenTextAlignment(speakerNameTextId, MUGEN_TEXT_ALIGNMENT_CENTER);
    }
    void addSubtitles() {
        const auto lines = wrapSubtitleText("<< Probably cool rival ace banter, but you can't really make it out, but just imagine it, it's really really cool probably >>");
        for (size_t i = 0; i < lines.size(); i++)
        {
            subtitleLineTextIds.push_back(addCenteredSubtitleLine(lines[i], i));
        }
    }
    std::vector<std::string> wrapSubtitleText(const std::string& tText) {
        std::vector<std::string> lines;
        std::string line;
        for (const auto& word : splitSubtitleWords(tText))
        {
            if (!line.empty() && line.size() + 1 + word.size() > subtitleMaximumLineLength)
            {
                lines.push_back(line);
                line.clear();
            }
            line += (line.empty() ? "" : " ") + word;
        }
        lines.push_back(line);
        return lines;
    }
    std::vector<std::string> splitSubtitleWords(const std::string& tText) {
        std::vector<std::string> words;
        size_t start = 0;
        while (start < tText.size())
        {
            const auto end = std::min(tText.find(' ', start), tText.size());
            if (end > start) words.push_back(tText.substr(start, end - start));
            start = end + 1;
        }
        return words;
    }
    int addCenteredSubtitleLine(const std::string& tLine, size_t tIndex) {
        const auto id = addMugenText(tLine.c_str(), subtitleFirstLinePosition + Vector3D(0, tIndex * subtitleLineHeight, 0), 2);
        setMugenTextColorRGB(id, 0.45, 0.6, 1.0);
        setMugenTextAlignment(id, MUGEN_TEXT_ALIGNMENT_CENTER);
        return id;
    }
    void removeSubtitles() {
        for (auto id : subtitleLineTextIds)
        {
            removeMugenText(id);
        }
        subtitleLineTextIds.clear();
    }
    void updateTransmissionTalking() {
        if (transmissionPhase != TransmissionPhase::TALKING) return;
        if (transmissionPhaseTicks < transmissionMinimumTalkingTicks) return;
        if (isTransmissionVoicePlaying()) return;
        removeMugenText(speakerNameTextId);
        removeSubtitles();
        setTransmissionPhase(TransmissionPhase::FADING_OUT);
    }
    bool isTransmissionVoicePlaying() {
        return transmissionVoiceChannel != -1 && isSoundEffectPlayingOnChannel(transmissionVoiceChannel);
    }
    void updateTransmissionFadeOut() {
        if (transmissionPhase != TransmissionPhase::FADING_OUT) return;
        setMugenAnimationTransparency(mugshotElement, 1.0 - getTransmissionFadeProgress());
        if (transmissionPhaseTicks < transmissionFadeTicks) return;
        setTransmissionPhase(TransmissionPhase::OVER);
    }
    double getTransmissionFadeProgress() {
        return std::min(double(1.0), double(transmissionPhaseTicks) / transmissionFadeTicks);
    }
    void setTransmissionPhase(TransmissionPhase tPhase) {
        transmissionPhase = tPhase;
        transmissionPhaseTicks = 0;
    }
    bool isTransmissionOver() {
        return transmissionPhase == TransmissionPhase::OVER;
    }

    // BOSS
    int bossEntityId;
    int bossCollisionId;
    int bossMaxHealth = 250;
    int bossHealth = 250;
    bool bossStarted = false;
    double bossTargetY = 55;
    double bossVelX = 0.8;
    int bossShootTimer = 60;
    int bossPatternIndex = 0;
    int bossHitFlashTicks = 0;

    int explosionCount = 12;
    bool bossIsDying = false;
    bool bossIsDead = false;
    int bossDeathTimer = 0;
    int bossExplosionsDone = 0;
    void loadBoss() {
        bossEntityId = addBlitzEntity(Vector3D(160, -70, 20));
        addBlitzMugenAnimationComponent(bossEntityId, &mSprites, &mAnimations, 50);
        setBlitzMugenAnimationVisibility(bossEntityId, 0);
    }
    void updateBoss() {
        updateBossStart();
        updateBossMovement();
        updateBossGetHit();
        updateBossHitFlash();
        updateBossShooting();
        updateBossDeath();
    }

    void updateBossStart() {
        if (bossStarted) return;
        if (!isTransmissionOver()) return;

        bossStarted = true;
        setBlitzMugenAnimationVisibility(bossEntityId, 1);
        bossCollisionId = addBlitzCollisionRect(bossEntityId, enemyCollisionListShots, CollisionRect(-30, -40, 60, 80));
        startBossMusic();
    }
    void startBossMusic() {
        if (gGameScreenData.mIsRetryingBoss) return;
        streamMusicFile("game/BOSS.ogg");
    }
    void updateBossMovement() {
        if (!bossStarted) return;
        if (isBossDefeated()) return;

        auto pos = getBlitzEntityPositionReference(bossEntityId);
        if (isBossEntering())
        {
            pos->y += 0.7;
            return;
        }

        pos->x += bossVelX * getBossPhaseSpeedFactor();
        if (pos->x < 60) { pos->x = 60; bossVelX = fabs(bossVelX); }
        if (pos->x > 260) { pos->x = 260; bossVelX = -fabs(bossVelX); }
    }
    bool isBossEntering() {
        return getBlitzEntityPosition(bossEntityId).y < bossTargetY;
    }
    bool isBossInSecondPhase() {
        return bossHealth * 2 < bossMaxHealth;
    }
    double getBossPhaseSpeedFactor() {
        return isBossInSecondPhase() ? 1.6 : 1.0;
    }
    void updateBossGetHit() {
        if (!bossStarted) return;
        if (isBossDefeated()) return;
        if (isBossEntering()) return;

        if (hasBlitzCollidedThisFrame(bossEntityId, bossCollisionId))
        {
            damageBoss();
        }
    }
    void damageBoss() {
        bossHealth--;
        gGameScreenData.mScore += 50;
        startBossHitFlash();
        if (bossHealth <= 0)
        {
            bossHealth = 0;
            startBossDeath();
        }
    }
    void startBossHitFlash() {
        bossHitFlashTicks = 3;
        setBlitzMugenAnimationColor(bossEntityId, 1.0, 0.4, 0.4);
    }
    void updateBossHitFlash() {
        if (!bossHitFlashTicks) return;
        bossHitFlashTicks--;
        if (!bossHitFlashTicks)
        {
            setBlitzMugenAnimationColor(bossEntityId, 1.0, 1.0, 1.0);
        }
    }
    void updateBossShooting() {
        if (!bossStarted) return;
        if (isBossDefeated()) return;
        if (isBossEntering()) return;

        bossShootTimer--;
        if (bossShootTimer <= 0)
        {
            fireBossPattern(getBossShotOrigin());
            tryPlayMugenSoundAdvanced(&mSounds, 1, 5, sfxVol);
            bossShootTimer = getRetryAdjustedShootInterval(isBossInSecondPhase() ? 34 : 50);
        }
    }
    Vector2D getBossShotOrigin() {
        return getBlitzEntityPosition(bossEntityId).xy() + Vector2D(0, 30);
    }
    void fireBossPattern(const Vector2D& origin) {
        if (bossPatternIndex == 0)
        {
            fireShotRing(origin, getRetryAdjustedShotAmount(18), 1.5, patternAngle);
            advancePatternAngle(0.17);
        }
        else if (bossPatternIndex == 1)
        {
            fireShotFanAtPlayer(origin, getRetryAdjustedShotAmount(5), 0.6, 2.4);
        }
        else if (bossPatternIndex == 2)
        {
            fireShotFanWithRandomOffset(origin, Vector2D(0, 1), getRetryAdjustedShotAmount(9), 1.6, 1.7);
        }
        else
        {
            fireShotRing(origin, getRetryAdjustedShotAmount(24), 1.2, patternAngle);
            advancePatternAngle(0.13);
        }
        bossPatternIndex = (bossPatternIndex + 1) % getBossPatternAmount();
    }
    int getBossPatternAmount() {
        return isBossInSecondPhase() ? 4 : 3;
    }

    void playExplosion(const Vector2D& pos, double scale) {
        auto e = addMugenAnimation(getMugenAnimation(&mAnimations, 51), &mSprites, pos.xyz(45));
        setMugenAnimationNoLoop(e);
        setMugenAnimationBaseDrawScale(e, scale);
    }
    void startBossDeath() {
        removeAllShots();
        changeBlitzMugenAnimation(bossEntityId, 52);
        setBlitzMugenAnimationColor(bossEntityId, 1.0, 1.0, 1.0);
        bossIsDying = true;
        bossExplosionsDone = 0;
        bossDeathTimer = 0;
    }
    void updateBossDeath() {
        if (!bossIsDying) return;
        if (bossIsDead) return;

        updateBossFalling();
        if (bossDeathTimer > 0)
        {
            bossDeathTimer--;
            return;
        }

        if (bossExplosionsDone < explosionCount)
        {
            playNextBossDeathExplosion();
        }
        else
        {
            finishBossDeath();
        }
    }
    void updateBossFalling() {
        getBlitzEntityPositionReference(bossEntityId)->y += 0.4;
    }
    void playNextBossDeathExplosion() {
        Vector2D bossPos = getBlitzEntityPosition(bossEntityId).xy();
        Vector2D offset = Vector2D(randfrom(-50, 50), randfrom(-40, 40));
        playExplosion(bossPos + offset, 1.0);
        tryPlayMugenSoundAdvanced(&mSounds, 1, 6, sfxVol);
        bossExplosionsDone++;
        int interval = 16 - bossExplosionsDone;
        if (interval < 3) interval = 3;
        bossDeathTimer = interval;
    }
    void finishBossDeath() {
        playExplosion(getBlitzEntityPosition(bossEntityId).xy(), 4.0);
        tryPlayMugenSoundAdvanced(&mSounds, 1, 6, jingleVol);
        setBlitzMugenAnimationVisibility(bossEntityId, 0);
        bossIsDead = true;
    }
    bool isBossDefeated() { return bossHealth == 0; }

    // BOSS HEALTH BAR
    AnimationHandlerElement* bossHealthBarBackground;
    AnimationHandlerElement* bossHealthBarForeground;
    int bossNameTextId;
    Vector3D bossHealthBarPosition = Vector3D(8, 13, 52);
    Vector2D bossHealthBarSize = Vector2D(190, 5);
    void loadBossHealthBar() {
        bossHealthBarBackground = addBossHealthBarRectangle(bossHealthBarPosition, Vector3D(0.8, 0.1, 0.1));
        bossHealthBarForeground = addBossHealthBarRectangle(bossHealthBarPosition + Vector3D(0, 0, 1), Vector3D(0.15, 0.35, 1.0));
        bossNameTextId = addMugenText("BLUE PEASANT", Vector3D(8, 10, 53), 2);
        setMugenTextColor(bossNameTextId, COLOR_WHITE);
        setBossHealthBarVisibility(false);
    }
    AnimationHandlerElement* addBossHealthBarRectangle(const Vector3D& tPosition, const Vector3D& tColor) {
        auto element = playOneFrameAnimationLoop(tPosition, getEmptyWhiteTextureReference());
        setAnimationColor(element, tColor.x, tColor.y, tColor.z);
        setAnimationSize(element, bossHealthBarSize.xyz(1), Vector3D(0, 0, 0));
        return element;
    }
    void updateBossHealthBar() {
        setBossHealthBarVisibility(isBossHealthBarVisible());
        setAnimationSize(bossHealthBarForeground, getBossHealthBarForegroundSize(), Vector3D(0, 0, 0));
    }
    Vector3D getBossHealthBarForegroundSize() {
        return Vector3D(bossHealthBarSize.x * bossHealth / bossMaxHealth, bossHealthBarSize.y, 1);
    }
    bool isBossHealthBarVisible() {
        return bossStarted && !bossIsDead && !isShowingLoss;
    }
    void setBossHealthBarVisibility(bool tIsVisible) {
        setAnimationVisibility(bossHealthBarBackground, tIsVisible);
        setAnimationVisibility(bossHealthBarForeground, tIsVisible);
        setMugenTextVisibility(bossNameTextId, tIsVisible);
    }

    // SHOT HANDLER
    struct Shot
    {
        int entityId;
        Vector2D direction;
        double speed;
        int collisionAttack;
        int hitAnimationNo;
        int shootableCollision = -1;
        bool isToBeRemoved = false;
    };
    std::map<int, Shot> mShots;
    void loadShotHandler() {}
    void updateShotHandler() {
        updateShotHandlerActive();
    }
    void updateShotHandlerActive() {
        auto it = mShots.begin();
        while (it != mShots.end())
        {
            updateSingleShot(it->second);
            if (it->second.isToBeRemoved)
            {
                unloadGeneralShot(it->second);
                it = mShots.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void removeAllShots()
    {
        auto it = mShots.begin();
        while (it != mShots.end())
        {
            unloadGeneralShot(it->second);
            it = mShots.erase(it);
        }
    }

    // GENERAL SHOTS
    double spritePointingUpRotation = M_PI / 2;
    double spritePointingDownRotation = -M_PI / 2;
    int addGeneralShot(const Vector2D& pos, CollisionListData* collisionList, int animationNo, int hitAnimationNo, double z, double speed, const Vector2D& direction, double spriteRotation)
    {
        auto entityId = addBlitzEntity(pos.xyz(z));
        addBlitzMugenAnimationComponent(entityId, &mSprites, &mAnimations, animationNo);
        setBlitzEntityRotationZ(entityId, getAngleFromDirection(direction) + spriteRotation);

        auto collisionAttack = addBlitzCollisionRect(entityId, collisionList, CollisionRect(-3, -3, 6, 6));
        mShots[entityId] = Shot{ entityId, direction, speed, collisionAttack, hitAnimationNo };
        return entityId;
    }
    void unloadGeneralShot(Shot& e)
    {
        removeBlitzEntity(e.entityId);
    }
    void updateSingleShot(Shot& e) {
        updateSingleShotMovement(e);
        updateSingleShotGetHit(e);
        updateSingleShotOver(e);
    }
    void updateSingleShotMovement(Shot& e) {
        auto pos = getBlitzEntityPositionReference(e.entityId);
        *pos += e.direction * e.speed;
    }
    void updateSingleShotGetHit(Shot& e) {
        if (hasShotCollidedThisFrame(e))
        {
            playShotHit(e);
            e.isToBeRemoved = true;
        }
    }
    bool hasShotCollidedThisFrame(Shot& e) {
        if (hasBlitzCollidedThisFrame(e.entityId, e.collisionAttack)) return true;
        return e.shootableCollision != -1 && hasBlitzCollidedThisFrame(e.entityId, e.shootableCollision);
    }
    void playShotHit(Shot& e) {
        auto hitEntity = addBlitzEntity(getBlitzEntityPosition(e.entityId).xy().xyz(45));
        addBlitzMugenAnimationComponent(hitEntity, &mSprites, &mAnimations, e.hitAnimationNo);
        setBlitzMugenAnimationNoLoop(hitEntity);
    }
    void updateSingleShotOver(Shot& e) {
        auto pos = getBlitzEntityPositionReference(e.entityId);
        if (pos->x < -10 || pos->x > 330 || pos->y < -10 || pos->y > 250)
        {
            e.isToBeRemoved = true;
        }
    }

    // PLAYER SHOTS
    void addPlayerShot() {
        auto pos = getBlitzEntityPositionReference(playerEntityId);
        addGeneralShot(pos->xy() + Vector2D(-4, -10), playerShotCollisionList, 20, 21, 9, 7.0, Vector2D(0, -1), spritePointingUpRotation);
        addGeneralShot(pos->xy() + Vector2D(2, -10), playerShotCollisionList, 20, 21, 9, 7.0, Vector2D(0, -1), spritePointingUpRotation);
    }

    // BOSS SHOTS
    void addBossShot(const Vector2D& pos, const Vector2D& direction, double speed) {
        const auto entityId = addGeneralShot(pos, enemyShotCollisionList, 43, 45, 9, speed, direction, spritePointingDownRotation);
        makeShotShootable(mShots[entityId]);
    }
    void makeShotShootable(Shot& e) {
        e.shootableCollision = addBlitzCollisionRect(e.entityId, shootableEnemyShotCollisionList, CollisionRect(-5, -7, 10, 14));
    }

    // SHOT PATTERNS
    double patternAngle = 0.0;

    void advancePatternAngle(double tAngleStep) {
        patternAngle += tAngleStep;
    }
    Vector2D getDirectionTowardsPlayer(const Vector2D& tOrigin) {
        return vecNormalize(getBlitzEntityPosition(playerEntityId).xy() - tOrigin);
    }

    void fireShotRing(const Vector2D& tOrigin, int tShotAmount, double tSpeed, double tAngleOffset) {
        for (int i = 0; i < tShotAmount; i++)
        {
            const auto angle = tAngleOffset + (2.0 * M_PI * i) / tShotAmount;
            addBossShot(tOrigin, Vector2D(cos(angle), sin(angle)), tSpeed);
        }
    }
    void fireShotFan(const Vector2D& tOrigin, const Vector2D& tDirection, int tShotAmount, double tSpreadAngle, double tSpeed) {
        for (int i = 0; i < tShotAmount; i++)
        {
            addBossShot(tOrigin, vecRotateZ2D(tDirection, getFanShotAngle(i, tShotAmount, tSpreadAngle)), tSpeed);
        }
    }
    double getFanShotAngle(int tShotIndex, int tShotAmount, double tSpreadAngle) {
        if (tShotAmount == 1) return 0;
        return -tSpreadAngle / 2 + (tSpreadAngle * tShotIndex) / (tShotAmount - 1);
    }
    void fireShotFanAtPlayer(const Vector2D& tOrigin, int tShotAmount, double tSpreadAngle, double tSpeed) {
        fireShotFan(tOrigin, getDirectionTowardsPlayer(tOrigin), tShotAmount, tSpreadAngle, tSpeed);
    }
    void fireShotFanWithRandomOffset(const Vector2D& tOrigin, const Vector2D& tDirection, int tShotAmount, double tSpreadAngle, double tSpeed) {
        const auto direction = vecRotateZ2D(tDirection, getRandomFanAngleOffset(tSpreadAngle, tShotAmount));
        fireShotFan(tOrigin, direction, tShotAmount, tSpreadAngle, tSpeed);
    }
    double getRandomFanAngleOffset(double tSpreadAngle, int tShotAmount) {
        const auto angleBetweenShots = tSpreadAngle / std::max(1, tShotAmount - 1);
        return randfrom(-angleBetweenShots / 2, angleBetweenShots / 2);
    }

    // SCORE UI
    MugenAnimationHandlerElement* uiBgElement;
    int scoreTextId;
    int scorePerTick = 13;
    void loadScoreUI() {
        uiBgElement = addMugenAnimation(getMugenAnimation(&mAnimations, 60), &mSprites, Vector3D(0, 0, 50));
        scoreTextId = addMugenTextMugenStyle("SCORE: 0", Vector3D(214, 11, 51), Vector3DI(-1, 0, 1));
        setMugenAnimationVisibility(uiBgElement, false);
        setMugenTextVisibility(scoreTextId, false);
    }
    void updateScoreUI() {
        if (isPaused()) return;
        gGameScreenData.mScore += scorePerTick;
        std::string newScoreText = std::string("SCORE: ") + std::to_string(gGameScreenData.mScore);
        changeMugenText(scoreTextId, newScoreText.c_str());
    }

    void setUIVisibility(bool isVisible)
    {
        setMugenAnimationVisibility(uiBgElement, isVisible);
        setMugenTextVisibility(scoreTextId, isVisible);
    }

    // GET READY SCREEN
    MugenAnimationHandlerElement* getReadyElement;
    bool hasShownGetReady = false;
    bool isShowingGetReady = false;
    int uiAnimationTicks = 0;
    void loadGetReady() {
        getReadyElement = addMugenAnimation(getMugenAnimation(&mAnimations, 64), &mSprites, Vector3D(0, 0, 50));
        setMugenAnimationVisibility(getReadyElement, false);
    }
    void updateGetReady() {
        updateGetReadyStart();
        updateGetReadyOver();
    }
    void updateGetReadyStart() {
        if (isShowingTutorial) return;
        if (hasShownGetReady) return;

        setMugenAnimationVisibility(getReadyElement, 1);
        streamMusicFile(getGetReadyMusicPath());

        uiAnimationTicks = 0;
        hasShownGetReady = true;
        isShowingGetReady = true;
    }
    const char* getGetReadyMusicPath() {
        return gGameScreenData.mIsRetryingBoss ? "game/BOSS.ogg" : "game/GAME.ogg";
    }
    void updateGetReadyOver() {
        if (!isShowingGetReady) return;

        uiAnimationTicks++;
        if (uiAnimationTicks > 120 || (uiAnimationTicks > 1 && hasPressedStartFlank()))
        {
            setMugenAnimationVisibility(getReadyElement, 0);
            isShowingGetReady = false;
            setUIVisibility(true);
        }
    }

    // VICTORY SCREEN
    MugenAnimationHandlerElement* victoryElement;
    bool hasShownVictory = false;
    bool isShowingVictory = false;
    int minimumResultScreenTicks = 60;
    void loadVictory() {
        victoryElement = addMugenAnimation(getMugenAnimation(&mAnimations, 66), &mSprites, Vector3D(0, 0, 50));
        setMugenAnimationVisibility(victoryElement, false);
    }
    void updateVictory() {
        updateVictoryStart();
        updateVictoryOver();
    }
    void updateVictoryStart() {
        if (hasShownVictory) return;
        if (!bossIsDead) return;

        setUIVisibility(false);
        tryPlayMugenSoundAdvanced(&mSounds, 100, 1, jingleVol);
        setMugenAnimationVisibility(victoryElement, 1);
        pauseMusic();
        removeAllShots();
        removeAllEnemies();
        setPlayerIdle();
        uiAnimationTicks = 0;
        hasShownVictory = true;
        isShowingVictory = true;
    }

    void screenWon() {
        setCurrentStoryDefinitionFile("game/OUTRO.def", 0);
        stopStreamingMusicFile();
        setNewScreen(getStoryScreen());
    }
    void resetScreen()
    {
        setNewScreen(getGameScreen());
    }

    void updateVictoryOver() {
        if (!isShowingVictory) return;

        uiAnimationTicks++;
        if (uiAnimationTicks > minimumResultScreenTicks && hasPressedContinue())
        {
            setMugenAnimationVisibility(victoryElement, 0);
            isShowingVictory = false;
            screenWon();
        }
    }

    // LOSS SCREEN
    MugenAnimationHandlerElement* lossElement;
    bool hasShownLoss = false;
    bool isShowingLoss = false;
    int playerDeathTicks = 0;
    int playerDeathDelayTicks = 60;
    void loadLoss() {
        lossElement = addMugenAnimation(getMugenAnimation(&mAnimations, 65), &mSprites, Vector3D(0, 0, 50));
        setMugenAnimationVisibility(lossElement, false);
    }
    void updateLoss() {
        updateLossStart();
        updateLossOver();
    }
    void updateLossStart() {
        if (hasShownLoss) return;
        if (!playerIsDying) return;
        if (++playerDeathTicks < playerDeathDelayTicks) return;

        rememberBossRetry();
        setUIVisibility(false);
        setBlitzMugenAnimationVisibility(bossEntityId, 0);
        setMugenAnimationVisibility(lossElement, 1);
        pauseMusic();
        removeAllShots();
        removeAllEnemies();
        tryPlayMugenSoundAdvanced(&mSounds, 100, 2, jingleVol);
        uiAnimationTicks = 0;
        hasShownLoss = true;
        isShowingLoss = true;
    }
    void rememberBossRetry() {
        if (!bossStarted) return;
        gGameScreenData.mIsRetryingBoss = true;
        gGameScreenData.mBossRetryAmount++;
    }
    void updateLossOver() {
        if (!isShowingLoss) return;

        uiAnimationTicks++;
        if (uiAnimationTicks > minimumResultScreenTicks && hasPressedContinue())
        {
            isShowingLoss = false;
            resetScreen();
        }
    }

    // TUTORIAL
    MugenAnimationHandlerElement* tutorialElement;
    bool isShowingTutorial = false;
    int tutorialTicks = 0;
    void loadTutorial() {
        tutorialElement = addMugenAnimation(getMugenAnimation(&mAnimations, 67), &mSprites, Vector3D(0, 0, 50));
        setMugenAnimationVisibility(tutorialElement, false);
    }
    void updateTutorial() {
        updateTutorialStart();
        updateTutorialOver();
    }
    void updateTutorialStart() {
        if (gGameScreenData.mHasShownTutorial) return;

        setMugenAnimationVisibility(tutorialElement, 1);
        tutorialTicks = 0;
        gGameScreenData.mHasShownTutorial = true;
        isShowingTutorial = true;
    }
    void updateTutorialOver() {
        if (!isShowingTutorial) return;

        tutorialTicks++;
        if (tutorialTicks > minimumResultScreenTicks && hasPressedContinue())
        {
            setMugenAnimationVisibility(tutorialElement, 0);
            isShowingTutorial = false;
        }
    }

    // BOSS RETRY
    double maximumBossShootInterval = 3600;
    void loadBossRetry() {
        if (!gGameScreenData.mIsRetryingBoss) return;
        skipToBossFight();
    }
    void skipToBossFight() {
        nextFloodSpawn = mFloodSchedule.size();
        floodTicks = floodEndTick;
        setInterludePhase(InterludePhase::OVER);
        setTransmissionPhase(TransmissionPhase::OVER);
        updatePlanetsMovement();
    }
    double getBossRetryDivisor() {
        return std::pow(2.0, gGameScreenData.mBossRetryAmount);
    }
    int getRetryAdjustedShotAmount(int tShotAmount) {
        return std::max(1, int(tShotAmount / getBossRetryDivisor()));
    }
    int getRetryAdjustedShootInterval(int tInterval) {
        return int(std::min(tInterval * getBossRetryDivisor(), maximumBossShootInterval));
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
    gGameScreenData.mGameTicks = 0;
    gGameScreenData.mScore = 0;
    gGameScreenData.mIsRetryingBoss = false;
    gGameScreenData.mBossRetryAmount = 0;
}

std::string getSpeedRunString() {
    int totalSeconds = gGameScreenData.mGameTicks / 60;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int milliseconds = (gGameScreenData.mGameTicks % 60) * 1000 / 60;
    return std::to_string(minutes) + "m " + std::to_string(seconds) + "s " + std::to_string(milliseconds) + "ms. Your hyper score is " + std::to_string(gGameScreenData.mScore) + ".";
}
