# City Parking

A 3D driving game made with **Unreal Engine 5.8**. You drive round a busy city block of one-way
streets, squeeze past double-parked cars, buses pulling out, garbage bags and people on the
crosswalks, and park in the one open space before the clock runs out.

Everything in the game (streets, buildings, cars, buses, people) is built by the code out of
simple shapes when you press Play. There are no art files yet, so the project is small and
you can swap in real models later.

## What you need (once)

1. **Epic Games Launcher**, then install **Unreal Engine 5.8** (the yellow *Install Engine* button in the Unreal Engine tab).
   (Other 5.x versions should also work: right-click `CityParking.uproject` and pick *Switch Unreal Engine version*.)
2. **Visual Studio 2022** (the free Community edition). When it asks what to install, tick
   **Game development with C++**. This is what turns the code into the game.

On a Mac you need Xcode instead of Visual Studio.

## First time opening the project

1. Download this folder (or clone the repo) somewhere like `Documents\Unreal Projects\CityParking`.
2. Double-click **CityParking.uproject**.
3. Unreal says the *CityParking* module is missing and asks to rebuild it. Click **Yes**.
   The first build takes a few minutes. If it fails, right-click `CityParking.uproject`,
   pick *Generate Visual Studio project files*, open `CityParking.sln`, and press **Build**
   (Ctrl+Shift+B). The error list there says what went wrong.
4. The editor opens. Make the level the game plays in:
   - **File > New Level > Basic**, click **Create**.
   - **File > Save Current Level As**, make a folder called `Maps`, name the level `Block`, Save.
5. Press the green **Play** button. The city appears around you.

From then on, step 2 and Play is all you need. After changing code, close the editor and open it
again (it rebuilds), or press the **Live Coding** button (Ctrl+Alt+F11) while it is open.

## How to play

| Key | Gamepad | What it does |
| --- | --- | --- |
| W / Up | Right trigger | Drive forward |
| S / Down | Left trigger | Brake, then reverse |
| A, D / Left, Right | Left stick | Steer |
| Space | A | Handbrake |
| C | Y | Switch between chase camera and high camera (good for parking) |
| R | Menu | Restart the level |
| N | B | Next level (after you park) |
| Tab | View (left small button) | Settings: change how far and how fast the car steers |

- Follow the one-way arrows. Driving forward against them costs points ("WRONG WAY!").
  Backing up into a space is fine.
- The green arrow at the top of the screen and the green beam point to the open space.
- Park with the whole car inside the green box, straight, and stop. Hold still for a
  second and a half.
- Bumping cars, buses and curbs adds damage and costs points. At 100% damage the car is totaled.
  People on the crosswalk stop if you are in their way, but bumping them costs 250.
- Running out of time gets you a parking ticket.
- Score when you park: driving score + time left + no-scratch bonus + neat parking + a bonus for
  facing the way traffic goes. 1 to 3 stars.

Levels get harder: the space gets shorter, more cars double-park, buses start pulling out,
a garbage truck with workers shows up on Avenue A, and on level 3 road works narrow 11th Street.
After level 3 each level is a new random block with the spot somewhere different.

## How the code is organised

```
CityParking.uproject          the project file Unreal opens
Config/DefaultEngine.ini      which level and game mode start, lighting settings
Config/DefaultInput.ini       turns on Enhanced Input (keyboard and gamepad)
Source/CityParking/
    ParkingGameMode           the rules: clock, score, damage, wrong way, when you have parked
    ParkingCar                your car: driving feel, steering, bumping into things, camera
    CityBlockBuilder          builds the block: streets, sidewalks, buildings, parked cars,
                              double-parked cars, buses, garbage, people, road works
    MovingObstacle            anything that moves back and forth (people, buses pulling out)
    ParkingSpot               the green space: checks the car is inside, straight and stopped
    ParkingHUD                everything on screen: clock, score, arrow, mini map, end card
    CityShapes                helper that makes coloured boxes, cylinders and spheres,
                              and fits real models and textures in their place
    CityArtSettings           the City Parking Art page in Project Settings
```

### Real cars and textures

Swap the boxes for real low-poly cars, a bus, and Megascans textures from Fab, without code:
see [docs/ADDING_REAL_ART.md](docs/ADDING_REAL_ART.md). You pick them in
**Edit > Project Settings > Game > City Parking Art**.

### Changing things without code

The driving numbers (top speed, acceleration, steering) are settings on the car. To tweak them in
the editor: Content Browser > right-click > **Blueprint Class** > search *ParkingCar* > name it
`BP_MyCar`. Open it, change the **Driving** numbers, then in **World Settings** set
*Default Pawn Class* to `BP_MyCar` (you may need to make a Blueprint of *ParkingGameMode* too
and pick it as *GameMode Override*).

## If something looks wrong

- **Everything is white or grey:** the colours come from the engine's basic shape material.
  The game still works; tell Claude and it will switch to its own material.
- **Black sky / dark:** the level is empty. Make it from the **Basic** template (step 4 above).
- **R does nothing:** the level has not been saved yet. Save it as `Maps/Block`.
- **Car falls or floats:** the city is built at height 0. Delete any floor you added that sits higher.
