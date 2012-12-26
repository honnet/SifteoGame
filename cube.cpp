#include "cube.h"

CubeID Cube::IDcounter = 0; // static, to construct successive IDs:

// this constructor gives a unique consecutive ID to each object
Cube::Cube() : dot_n(0), id(IDcounter++), lifeCounter(100) { };

// initialize the background and the life counter display
void Cube::initDisplay() {
    vid.initMode(BG0_SPR_BG1);
    vid.attach(id);
    vid.bg0.image(vec(0,0), ConanIMG);
    // draw 10 band aid (100% life)
    vid.bg0.fill(vec(3, 15), vec(10, 1), BandIMG);
    System::paint();
}

// try to recycle a hidden dot or make a new one
void Cube::newDot(const Float2 p, const Float2 v) {
    bool noRecycle = true; // can a hidden dot be recycled?
    for (uint8_t d=0; d<dot_n && noRecycle; d++) {
        if ( vid.sprites[d].isHidden() ) {
            initDot(d, p, v);
            noRecycle = false;
        }
    }
    if (noRecycle && dot_n<maxDots) {
        initDot(dot_n, p, v);
        ++dot_n;
    }
}

// initialize dots as not hidden (+ position & velocity)
void Cube::initDot(const uint8_t d, const Float2 p, const Float2 v) {
    vid.sprites[d].setHeight(SPR_size.y); // "un-hide"
    dots[d].pos = p;
    dots[d].vel = v;
}

// compute velocity & position (cube included) and draw dot (exploded or not)
void Cube::animate(Cube* cubes, const float dts) {
    static const float deadzone = 2.0f;
    static const float accelScale = 0.6f;
    static const float damping = 0.995f;

    if (lifeCounter <= 0)
        return;

    for (uint8_t d=0; d<dot_n; d++) {
        // if dot neither hidden nor exploded, animate it:
        if (not (vid.sprites[d].isHidden() || dots[d].exp)) {
            Float2 accel = id.accel().xy();
            if (accel.len2() > deadzone * deadzone)
                dots[d].vel += accel * accelScale;
            dots[d].vel *= damping;
            Float2 step = dots[d].vel * dts;
            Float2 candidate = dots[d].pos + step;

            // Does this dot want to leave the cube ?
            Side mySide = (candidate.y < minPos.y) ? TOP    : //  0
                          (candidate.x < minPos.x) ? LEFT   : //  1
                          (candidate.y > maxPos.y) ? BOTTOM : //  2
                          (candidate.x > maxPos.x) ? RIGHT  : //  3
                                                     NO_SIDE; // -1
            bool stayHere = true;   // default
            CubeID hisID;           // neighbor CubeID
            Side hisSide = NO_SIDE; // his side number that touches mySide

            if (mySide != NO_SIDE) {          // avoids cubeAt(NO_SIDE) error
                Neighborhood myNbh(id);
                hisID = myNbh.cubeAt(mySide);
                if (hisID.isDefined()) {      // ...same with sideOf(NO_SIDE)
                    Neighborhood hisNbh(hisID);
                    hisSide = hisNbh.sideOf(id);
                    stayHere = cubes[hisID].isFull();
                }
            }
            if (stayHere) {                         // will this dot stay?
                if (mySide==TOP || mySide==BOTTOM)  // bounce vertically?
                    dots[d].vel.y = -dots[d].vel.y;
                if (mySide==LEFT || mySide==RIGHT)  // ...horizontally?
                    dots[d].vel.x = -dots[d].vel.x;

                dots[d].pos += dots[d].vel * dts;   // finish calculation
                vid.sprites[d].setImage(DotIMG);    // draw the dot
                vid.sprites[d].move(dots[d].pos);
            } else {                                // move to neighbor
                dots[d].pos += step;                // finish calculation
                moveDot(d, cubes[hisID], mySide, hisSide);
            }
        }
    }
    collisionCheck();
    displayDamages();
}

// check if maximum capacity is reached or if dots can be recycled
bool Cube::isFull() {
    if (lifeCounter <= 0)               // cube in "game over" mode
        return true;
    if (dot_n < maxDots)                // is maximum reached ?
        return false;                   // not full
    for (uint8_t d=0; d<dot_n; d++)     // or can a hidden dot be recycled ?
        if (vid.sprites[d].isHidden())
            return false;               // not full
    return true;                        // full (no recycle or max reached)
}

// create a new dot at the good border of the good cube
void Cube::moveDot(const uint8_t d, Cube& c,
                   const Side mine, const Side his) {
    // move the dot to the good border:
    if      (mine == TOP   ) dots[d].pos.y = maxPos.y;
    else if (mine == LEFT  ) dots[d].pos.x = maxPos.x;
    else if (mine == BOTTOM) dots[d].pos.y = minPos.y+SPR_size.y;
    else if (mine == RIGHT ) dots[d].pos.x = minPos.x+SPR_size.x;

    // compute the discrete rotation to perform if needed:
    uint8_t angleI = umod((mine-his-2), 4);
    if (angleI) {
        dots[d].vel = dots[d].vel.rotateI(angleI);    // velocity rotation
        dots[d].pos = rotatePos(dots[d].pos, angleI); // position rotation
    }
    c.newDot(dots[d].pos, dots[d].vel);               // move to good cube
    vid.sprites[d].hide();                            // ...not here !
}

// check distance between dots and mark them as exploded if too close
void Cube::collisionCheck() {
    for (uint8_t i=0; i<dot_n; i++)
        if (not vid.sprites[i].isHidden() && !dots[i].exp)
            for (uint8_t j=i+1; j<dot_n; j++)
                if (not vid.sprites[j].isHidden() && !dots[i].exp)  {
                    Float2 dif = dots[i].pos - dots[j].pos;
                    // are the dots too close ?
                    if (dif.len2() < SPR_size.len2()/4) {
                        dots[i].pos -= dif/2; // bring dots closer
                        dots[j].pos += dif/2; // to each other
                        dots[i].exp = exploDuration;
                        dots[j].exp = exploDuration;
#if ACCURATE == 1
                        // use a binary map to check if we hit or miss Conan:
                        dots[i].bnd = isHit(dots[i].pos);
                        dots[j].bnd = isHit(dots[j].pos);
#else
                        // ...or use a FlatAssetImage but it's less accurate:
                        Float2 o = SPR_size/2; // offset to sprite center
                        uint16_t linPosI = uint16_t( (dots[i].pos.x+o.x)/8 ) +
                                           uint16_t( (dots[i].pos.y+o.y)/8 ) *
                                           ConanIMG.tileWidth() ;
                        uint16_t linPosJ = uint16_t( (dots[j].pos.x+o.x)/8 ) +
                                           uint16_t( (dots[j].pos.y+o.y)/8 ) *
                                           ConanIMG.tileWidth() ;
                        // D'oh! need band aid ?
                        dots[i].bnd = (ConanIMG.tile(linPosI) != whiteTile);
                        dots[j].bnd = (ConanIMG.tile(linPosJ) != whiteTile &&
                                                     linPosJ  != linPosI);
#endif
                        // D'oh! need band aid ?
                        if (dots[i].bnd)
                            lifeCounter -= 5;
                        if (dots[j].bnd)
                            lifeCounter -= 5;
                    }
                }

    if (lifeCounter <= 0)
        win();
}

// (dis)play explosions and damages on Conan
void Cube::displayDamages() {
    bool displayDoh = false;
    const uint8_t limit = 3*exploDuration/4;

    for (uint8_t d=0; d<dot_n; d++) {
        if (dots[d].exp) {                              // is it exploded ?
            --dots[d].exp;
            if (dots[d].exp == exploDuration-1) {
                playSample(*ExploSND[r.chance(0.5)]);   // play an explosion sound
                if (dots[d].bnd)
                    displayDoh = true;                  // will display D'oh !
            }

            if (dots[d].exp > limit)                    // explosion animation?
                vid.sprites[d].setImage(*ExplosionIMG[dots[d].exp & 0x1]);
            else if (dots[d].bnd && dots[d].exp == limit)
                vid.sprites[d].setImage(BandIMG);       // display band aid
            else if (dots[d].exp == 0 || !dots[d].bnd) {
                vid.sprites[d].hide();                  // end of damages
                vid.bg0.image(vec(10,1), DohIMG.tileSize(),
                              ConanIMG, vec(10,1));
                dots[d].exp = 0;
                dots[d].bnd = 0;
            }
        }
    }

    if (displayDoh) {
        vid.bg0.image(vec(10,1), DohIMG);
        // remove a band aid each time 10% life is lost
        UInt2 size = {10-lifeCounter/10, 1};
        vid.bg0.fill(vec(3,15), size, whiteTile);
        playSample(*DohSND[r.chance(.5)]); // play one of the D'oh! sounds
        System::paint();
    }
}

// display a life counter with band aids at the bottom of the screen
void Cube::displayCounter() {
    UInt2 size = {10-lifeCounter/10, 1}; // maximum = 10 band aids (100%)
    vid.bg0.fill(vec(3,15), size, whiteTile);
    System::paint();
}

// Game Over: Conan The Simpsonian is dead !
void Cube::win() {
    playSample(WinSND);
    if (--IDcounter == 0)
        AudioChannel(0).pause(); // loop

    reset();
    vid.bg0.fill(vec(0,0), LCD_size/8, whiteTile);
    System::paint();
    vid.bg0.image(vec(2,2), EndIMG);
    System::paint();

    String<16> message;
    vid.initMode(BG0_ROM);
    message << "Touch: restart";
    vid.bg0rom.text(vec(1,0), message);
    vid.setWindow(4, 8);
    System::paint();
}

// used to reset all dots after a win condition (or not)
void Cube::reset() {
    for (uint8_t d=0; d<dot_n; d++) {
        vid.sprites[d].hide();
        dots[d].exp = 0;                // no explosion
        dots[d].bnd = 0;                // no band aid
        dots[d].vel = vec(0,0);         // null velocity
        dots[d].pos = LCD_size;         // out of display
        vid.sprites[d].move(LCD_size);
    }
    dot_n = 0;
}

// called by touching the screen (see main.c)
void Cube::restart() {
    lifeCounter = 100;
    playSample(StartSND);
    initDisplay();
    if (IDcounter++ == 0)
        AudioChannel(0).resume(); // loop
}

