#pragma once
#include <sifteo.h>
#include "assets.gen.h"
#if ACCURATE == 1         // It takes more space but we can use
# include "conan_map.gen.h" // bin_map[] for more accuracy to detect
#endif                      // if an explosion was on Conan or not.
using namespace Sifteo;

/*
 * Cube class: takes care of dots (position, collision...)
 */

static const Float2   Z = {0,0};
static const UInt2    SPR_size = DotIMG.pixelSize();
static const Float2   minPos = Z;
static const Float2   maxPos = LCD_size-SPR_size;
static const uint8_t  maxDots = 8;
static const uint8_t  exploDuration = 40;
static const PinnedAssetImage* ExplosionIMG[] = {&Exp1IMG, &Exp2IMG};
static const AssetAudio* DohSND[] = {&Doh1SND, &Doh2SND};
static const AssetAudio* ExploSND[] = {&Exp1SND, &Exp2SND};
static const uint16_t whiteTile = ConanIMG.tile(vec(0,0));

static Random r;
// gives a random position somewhere in the available locations:
static Float2 randPos() {
    return vec( r.randrange(maxPos.x), r.randrange(maxPos.y) );
}
// gives a random velocity between 90 and 120:
static Float2 randVel() {
    return polar(r.uniform(0, M_PI*2), r.uniform(90.f, 120.f));
}
// home made position rotator (using discrete times 90° angles)
static Float2 rotatePos(const Float2 pos, const uint8_t angleI) {
    Float2 a1 = { LCD_size.y - pos.y, pos.x      + 0     };
    Float2 a2 = { LCD_size.x - pos.x, LCD_size.y - pos.y };
    Float2 a3 = {          0 + pos.y, LCD_size.x - pos.x };
    switch (angleI) {
        default: ASSERT(0);
        case 0: return pos;
        case 1: return a1;
        case 2: return a2;
        case 3: return a3;
    }
}
#if ACCURATE == 1
// get a bit from a "map" of 128 lines of 16 bytes
inline bool isHit(const Float2 pos) {
    const uint8_t x = uint8_t(pos.x);
    const uint8_t y = uint8_t(pos.y);
    // 8 bits per word and 16 words per line:
    const uint16_t idx = x/8 + y*16 ;
    return ( bin_map[idx] >> (x%8) ) & 0x1;
}
#endif
// scan for a free audio channel and play a sample if possible
inline void playSample(const AssetAudio &AA) {
    for (int i=1; i<8; i++)
        if (!AudioChannel(i).isPlaying()){
            AudioChannel(i).play(AA, AudioChannel::ONCE);
            return;
        }
}


class Cube {
private:
    struct Dot {
        Float2  pos; // position
        Float2  vel; // velocity
        uint8_t exp; // exploded if > 0
        bool    bnd; // band aid
        Dot() : pos(Z), vel(Z), exp(0), bnd(0) { }
    };
    Dot           dots[maxDots]; // max 8 sprites per sifteo
    uint8_t       dot_n;         // number of active dots
    CubeID        id;            // ID of this cube
    static CubeID IDcounter;     // to construct successive IDs
    int8_t        lifeCounter;   // to know when Conan dies !
    VideoBuffer   vid;

    bool isFull();
    void initDot(const uint8_t d, const Float2 p, const Float2 v);
    void moveDot(const uint8_t whichDot, Cube& toWhichCube,
                 const Side mySide, const Side hisSide);
    void collisionCheck();
    void displayDamages();
    void displayCounter();
    void win();
    void reset();

public:
    Cube();
    void initDisplay();
    void newDot(const Float2 p=randPos(), const Float2 v=randVel());
    void animate(Cube cubes[], const float dts);
    void restart();
};

