#include <sifteo.h>
#include "assets.gen.h"
#include "cube.h"
using namespace Sifteo;

const uint8_t numCubes = 3; //CUBE_ALLOCATION;
static Cube cubes[numCubes];

static AssetSlot slot = AssetSlot::allocate().bootstrap(BootstrapGroup);
static ScopedAssetLoader assetLoader;
static Metadata M = Metadata()
    .title("Conan The Simpsonian")
    .package("com.sifteo.sdk.conan_the_simpsonian", "1.0")
    .icon(Icon)
    .cubeRange(0, numCubes);


// initialize the welcome+loop sound and the background
void init(Cube* cubes) {
    AudioChannel(0).play(loopSND, AudioChannel::REPEAT);
    AudioChannel(1).play(StartSND, AudioChannel::ONCE);

    // allow fast display:
    assetLoader.init();
    AssetConfiguration<1> config;
    config.append(slot, ConanIMG.assetGroup());

    // initialize the background:
    for (CubeID c : CubeSet::connected()) {
        cubes[c].initDisplay();
        assetLoader.start(config, c);
    }
    System::paint();
}

// generates random durations and tell when a time out occurred
static bool randomTimeDone(CubeID c) {
    static Random r;
    static SystemTime random[numCubes];
    static bool firstTime = true;

    if (firstTime) { // we don't want to wait too much the 1st time:
        for (CubeID c : CubeSet::connected())
            random[c] = TimeDelta(r.uniform(0.f, 1.f)) + SystemTime::now();
        return (firstTime = false);
    }

    if (random[c].inPast()) {
        random[c] = TimeDelta(r.uniform(1.f, 2.f)) + SystemTime::now();
        return true;
    }
    return false;
}

// generate random dots on random places...
void main() {
    TimeStep ts;
    init(cubes);

    while (1) {
        ts.next();
        float dts = float(ts.delta());

        for (CubeID c : CubeSet::connected()) { //

            // add a random dot at a random time:
            if ( randomTimeDone(c) )
                cubes[c].newDot();
            cubes[c].animate(cubes, dts);

            // restart if we touch the screen:
            if (c.isTouching())
                cubes[c].restart();

            System::paint();
        }
    }
}
