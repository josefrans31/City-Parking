# Adding real cars, buses and textures

The game builds its city out of coloured boxes so it always runs. You can swap in real models and
textures without touching any code: download them from **Fab** (Epic's free asset store, built into
Unreal) and pick them in **Project Settings**. Anything you leave empty stays a box.

Your laptop has built-in Intel graphics, so stick to **low-poly** models and **low-resolution**
textures. The tips below keep it smooth.

## 1. Get the new code

1. Download the ZIP of the repo and copy its `Source` folder over your project's `Source` folder.
2. Close Unreal, double-click `CityParking.uproject`, and say **Yes** when it asks to rebuild.

(This update also makes the steering slower and calmer.)

## 2. Download models from Fab

1. In the Unreal editor, open Fab: **Window > Fab** (or the Fab button in the Content Browser's
   **+ Add** menu).
2. Search for each of these and tick the **Free** filter:
   - `low poly car` (get 2 to 4 different ones, they become the parked cars)
   - `low poly van` (optional, for the delivery vans)
   - `low poly bus`
   - one more car you like for **your** car
3. Pick models that are **one piece** (body and wheels together) and say "low poly" or show a small
   triangle count (under about 20,000). Avoid "high detail", "Nanite" or "cinematic" cars.
4. Click **Add to Project**. They land in the Content Browser, usually under `Fab`.

## 3. Download textures (Megascans)

Megascans surfaces are free on Fab for Unreal projects.

1. In Fab, search **Megascans** surfaces: `asphalt`, `concrete pavement` (sidewalk), and 2 or 3 walls
   like `brick wall`, `painted concrete`, `stone facade`.
2. When it asks for quality, pick **Low** or **Medium** (1K or 2K). High (4K/8K) will be slow on your laptop.
3. **Add to Project**. Each one gives you a material whose name starts with `MI_`.

## 4. Pick them in Project Settings

1. **Edit > Project Settings**, then on the left under **Game** click **City Parking Art**.
2. **Vehicles**
   - **Player Car**: click the box next to *Mesh* and pick your car.
   - **Parked Cars**: click **+** once per car model and pick a mesh in each.
   - **Vans**: same, optional.
   - **Bus**: pick the bus.
3. **Textures**
   - **Street**: pick the asphalt `MI_` material.
   - **Sidewalk**: pick the concrete one.
   - **Building Walls**: click **+** for each wall material.
4. Settings save by themselves. Press **Play**.

## 5. If something looks off

| What you see | What to do |
| --- | --- |
| A car drives backwards | Tick **Turn Around** for that model. |
| Tyres float or sink into the street | Change **Raise By** (cm). Negative moves it down. |
| A texture looks stretched or blurry | Make **Tile Size Cm** smaller (try 200). If nothing changes, open the `MI_` material, find the setting that controls repeats (often *Tiling*), and type its exact name into **Tiling Parameter Name**. |
| Only the car body shows, no wheels | The model came in pieces. Pick a one-piece model, or tell Claude. |
| It runs slowly | Use fewer car models, pick Low texture quality, or empty the Building Walls list. |

Models are resized to the game's sizes (cars 4.5 m, vans 6 m, the bus 12 m), so you do not need to
scale them. You still bump into the same invisible boxes as before, so the driving and scoring do not
change.
