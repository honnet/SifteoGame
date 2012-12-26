Welcome !
=========

Thanks for visiting the repo of my 1st Sifteo game: Conan The Simpsonian !  
To use it you will need the Sifteo sdk: http://developers.sifteo.com.  
Simply place this repository in the example folder and follow the instructions:


Simulation:
-----------

If your environment variables are already set, simply use the comand:

    make simu

A more accurate but less space-saving version is available with:

    make -d ACCURATE=1 run

More details are given in the comments and in the SDK makefiles.
Enjoy ;)


Scenario:
---------

- Dots appear on random cubes in a random location at random times.
- For any cube neighbored to another, dots will travel continuously from one
  cube to another, each traveling to a random location on the neighbored cube.
- Colliding dots explode, destroying all colliding dots.
- Tilting a cube causes dots to fall in the direction of the tilt.
- The background of one cube should always show Conan the Barbarian.
    - Explosions on top of Conan damage him by an amount of your choice that is
    less than 100.
    - When Conan has received 100 points of damage, go to win condition, and
    start again.

