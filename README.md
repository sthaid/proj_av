# OVERVIEW

This program simulates autonomous vehicles driving in a world.

The world is made up of square foot elements, which are:
- green grass
- black road surface
- yellow center line
- red stop line
- blue vehicle
- pink failed vehicle
- white headlights
- orange taillights

The autonomous vehicle simulation uses the following inputs
to determine the vehicle's steering and speed controls.
- view from the driver's seat out the front of the autonomous vehicle
- the autonomous vehicle's current location, direction, and speed 

# LIMITATIONS

The autonomous vehicle will only make turns when continuing from a stop line. 

When proceeding from a stop line the autonomous vehicle does not check to the left and right before proceeding.

# BUILD

I build on Fedora 20.

In addition to Development Tools, the following packages must be installed:
- SDL2-devel
- SDL2_ttf-devel
- SDL2_mixer-devel
- libpng-devel

Run make to build the autonomous vehicle simulation program (av), and the 
world editor program (edw).

# AV PROGRAM USAGE

Av is the autonomous vehicle simulation program.

Synopsis:  av [-n num_vehicles] [world_filename]

Display:
- the left side of the display shows the world
- the upper 3/4 of the right side of the display shows the view and 
  dashboard of one of the autonomous vehicles; the upper half of the
  dashboard is provided by the base car class, and the lower half of the dashboard
  is provided by the derived autonomous_car class
- the lower 1/4 of the right side of the display provides program control and status

Controls:
- to pan the world view: left click and drag
- to zoom the world view: use the mouse wheel
- to select a vehicle: 
  - right click the desired vehicle in the world, or
  - right click the vehicle view or dashboard
- RUN: starts the simulation
- STOP: stops the simulation
- LAUNCH: creates a new autonomous vehicle, at the launch point near the center of the world
- STEP: step the simulation 50 milliseconds
- DEL: delete the currently selected autonomous vehicle
- TURBO; activate turbo mode, speeds up the simulation
- OFF: deactivate turbo mode

# EDW PROGRAM USAGE

Edw is the world editor program. 
Running this program is not required, as a sample world.dat is provided.

Synopsis:  edw [world_filename]

Display: 
- the left side of the display shows the world
- the right side of the display contains the editor controls

The world view can be panned and zoomed, same as in the av program.

First use CREATE_ROADS mode to create a network of roads. 
Then use the EDIT_PIXELS mode to adjust the roads by removing center line segments, 
adding stop lines, and rounding corners.

When in CREATE_ROADS mode, the direction is controlled either by selecting 1 to 9, or by ROL/ROR (rotate left and right): 
- '1' is sharp bend to the left
- '5' is straight
- '9' is sharp bend to the right
- ROL and ROR provide a 1 degree decrement/increment of the direction.  The keyboard left and right arrows are equivalent to ROL and ROR.

When in EDIT_PIXELS mode, select the pixel (world element) to be modified by right clicking the location on the world view. A block of pixels can be modified by right click and drag.

# DESIGN

The av program is comprised of four classes: display, world, car, and autonomous_car. 
These are described below.

The display class extends the Simple DirectMedia Layer, https://www.libsdl.org/. The display class supports
dividing the display window into sections, called panes. The display class also extends SDL's event handling.

The world class provides static world elements; such as roads, grass, center line, and stop lines. The
static world elements are read from a file, such as world.dat, during program initialization. The world
class also provides support for overlaying dynamic world objects, and drawing the world. And finally,
the world class provides support for returning a view of the world from a specified direction and orientation.

The car class supports updating the position and orientation of the car based on speed and steering
control settings.

The autonomous_car class is derived from the car class, and provides the update_controls procedure. The update_controls procedure
calls world::get_view to obtain a view of the world from the perspective of the front of the car. This procedure
then analyzes the view, and then calls the car::set_speed_ctl and car::set_steer_ctl procedures to update
the steering and speed of the car.



