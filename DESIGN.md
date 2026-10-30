# City Parking: design and plan

## The idea

You have a few minutes to find parking on a crowded city block. The streets are one-way and
narrow. Cars are double-parked with their blinkers on, buses pull out of their stops, garbage
bags spill off the curb, delivery vans block half the lane and people cross wherever they like.
Get round the block, find the open space, and parallel park without scratching anything.

Original game. The menu, career and garage parts are modelled on Jose's own Event Horizon
Takeover (timed levels, cash, unlockable vehicles, drivers with bonuses, online rank).

## Core loop

1. Start at the end of 11th Street with the clock running.
2. Drive the loop of one-way streets (always right turns), following the green arrow.
3. Squeeze past obstacles. Every bump costs points and adds damage.
4. Find the green space and parallel park: inside, straight, stopped for 1.5 s.
5. Score card with 1 to 3 stars. Next level or retry.

## Obstacles (in the game now)

| Obstacle | What it does | From level |
| --- | --- | --- |
| Parked cars on both curbs | Line every street, packed tight round the space | 1 |
| Double-parked cars and vans | Stick into the lane with hazards on | 1 (more each level) |
| Garbage bag piles | Physics bags at the curb; knock them flying, -20 each | 1 |
| People on crosswalks | Walk across; they wait if you block them | 1 (more each level) |
| Buses at stops | Level 1 parked; level 2+ pull out into the lane and back | 1 / 2 |
| Garbage truck with workers | Blocks half of Avenue A, workers walk out behind it | 2 |
| Road works | Barrier and cones narrow 11th Street | 3 |

## Scoring

- Start at 1000. Bump: up to -240 depending on speed (curbs cost less). Pedestrian bump: -250.
  Garbage: -20. Wrong way: -25 per second after the first second.
- Park bonus: time left x 10, (100 - damage) x 5, up to 200 for neat parking, 150 for facing traffic.
- Stars: 3 = under 10% damage and 30% of the time left. 2 = under 40% damage. 1 = parked.
- Fail: time runs out ("parking ticket") or 100% damage ("totaled").

## Technical choices

- **Unreal Engine 5.8, C++ with room for Blueprints.** The rules, car and city are C++ so they
  can be written and reviewed as text. Every tuning number is exposed so a Blueprint child can
  change it in the editor.
- **Arcade car, not Chaos Vehicles.** A simple steering model that moves the car with sweeps
  and slides along whatever it hits. Exact, predictable steering suits tight squeezes and
  parallel parking, and it needs no wheel skeleton or vehicle art. Chaos Vehicles can come later
  if we want body roll and real suspension.
- **City built from code.** The block is generated from a seed per level, out of engine basic
  shapes. No art is needed to play; real models replace the shapes later.
- **HUD drawn with Canvas.** No widget assets yet. Moves to UMG when menus arrive.
- **Enhanced Input set up in code**, keyboard and gamepad.

## Plan

**Phase 1, playable block (this first version)**
One block, 4 one-way streets, all obstacles above, parking check, score, levels, HUD, mini map.
Jose builds and plays it, and we tune the driving feel and difficulty.

**Phase 2, feel and look**
- Sounds: engine, horn (honk to make people hurry), bumps, city noise, music.
- Real models for the car, traffic and buildings (Fab / Marketplace city packs), windows, shop signs.
- Traffic that drives the loop with you, cabs that cut in, a bus that stops in front of you.
- Smoke and sparks on bumps, brake lights, blinkers that blink.

**Phase 3, game around it (from Event Horizon Takeover)**
- Home menu, Career and Quick Play, pause menu, settings.
- Cash per level, a garage of cars to buy (small hatchback to big SUV that barely fits).
- Drivers with small bonuses, like the pilots in Event Horizon Takeover.
- Saved progress and high scores, then the online Rank.
- More blocks: an open grid of several blocks, rush hour, night, rain and snow.

**Phase 4, platforms**
Windows first. Then Mac, consoles or phones (phones need touch controls and lighter graphics).

## What has to happen on Jose's computer

Unreal cannot run in Claude's cloud machine, so the code has not been compiled yet. The first
build on Jose's PC will show any compile errors; paste them back to Claude and they get fixed.
