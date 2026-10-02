/*
 *  YASAG  -  Yet Another Space Alien Game  -  tunables + internal declarations
 *  ---------------------------------------------------------------------------
 *  Every #define lives here, and so does every forward declaration for
 *  game.c (the `alien_t` model and the ~45 helper prototypes, at the foot of
 *  this file) - so the .c is nothing but function bodies.  Include this after
 *  hal.h in the .c; it pulls in hal.h itself for RGB() and SCREEN_W / SCREEN_H.
 */
#ifndef YASAG_GAME_H
#define YASAG_GAME_H

#include <stdbool.h>      /* bool  - used by the declarations at the foot of this file */
#include <stdint.h>       /* intN_t / uintN_t - ditto                                 */

#include "hal.h"          /* RGB() for the colour defines; SCREEN_W / SCREEN_H */

/* Shown on the splash screen ("<VERSION_STR>  by Thomas Dzubin").  Keep in
 * sync with pico_set_program_version(...) in CMakeLists.txt.
 *
 * Versioning: the V1.xx line is picocalc-space-game - PicoCalc hardware only.
 * The V2.xx line is YASAG: the SAME gameplay, but HARDWARE-INDEPENDENT - every
 * hardware operation goes through the HAL in hal.h, so one C codebase builds
 * for the PicoCalc (RP2040/RP2350), the Pimoroni PicoSystem, and Windows/Linux
 * via SDL2.  The target and screen size are chosen at compile time; nothing in
 * this file or game.c touches a peripheral, a pico/... header, or SDL. */
#define VERSION_STR  "V2.00A"

/* --- F5 screenshot --------------------------------------------------------- *
 *  Built only where CMake defines YASAG_HAS_SCREENSHOT (PicoCalc, SDL).  The
 *  status line below is generic (screen coords + a timeout); the capture
 *  mechanism and its tuning live in the per-target implementation
 *  (hal/screenshot_picocalc.c reads the ST7789 GRAM back over SPI). */
#define SHOT_MSG_MS      2200     /* how long the "Saved ..." / error line shows */
#define SHOT_MSG_COL     2        /* status line, glyph coords, in the box       */
#define SHOT_MSG_ROW     9        /* (drawn only AFTER the capture, so not in it) */

/* =========================================================================
 *  Tunables - change the look and feel here
 * ===================================================================== */

/* --- Play box + screen fit -------------------------------------------- *
 *  The play box is BOX_W x BOX_H at the top-left of the screen; the status
 *  strip (stacked gauges + the Lv / Kills / coord readouts) sits below it, and
 *  is compiled out when SHOW_STATUS is 0.
 *
 *  All geometry from here on is authored for a 320x320 panel and scaled to the
 *  real SCREEN_W x SCREEN_H by the SCALE_* / TEXT_* macros in hal.h:
 *    - BOX_W scales by SCREEN_W, BOX_H by SCREEN_H, so a 4:3 screen (SDL
 *      640x480) gets a proportionally WIDER box - a bigger starfield;
 *    - sprites (ship, aliens, flames, torpedoes) and world speeds scale
 *      uniformly (SCALE_S / SCALE_SF), so squares stay square;
 *    - text scales by the integer YASAG_TEXT_SCALE (1 on the MCUs, 2 on SDL);
 *    - vertical positions keep their fraction of screen height ("same vertical
 *      ratios").
 *  At 320x320 with YASAG_TEXT_SCALE 1 every macro below is the identity, i.e.
 *  its exact V1.00A value - the PicoCalc build is byte-for-byte unchanged.
 *  A _Static_assert in game.c still checks BOX_W/BOX_H fit the screen. */
#define BOX_W          SCALE_X(304)  /* play-box width  (320: 304; wider on a 4:3 screen) */
#define BOX_H          SCALE_Y(228)  /* play-box height (320: 228)                        */
#define SHOW_STATUS       1        /* 1: draw the status strip; 0: starfield fills box */
#define BORDER         SCALE_S(3)            /* border thickness, drawn inside (320: 3) */
#define IN_LO          BORDER                /* first interior pixel  (3)      */
#define IN_HI_X        (BOX_W - BORDER)      /* one past last interior col (301) */
#define IN_HI_Y        (BOX_H - BORDER)      /* one past last interior row (225) */
#define IN_SPAN_X      (IN_HI_X - IN_LO)     /* interior width  (298)          */
#define IN_SPAN_Y      (IN_HI_Y - IN_LO)     /* interior height (222)          */
#define CENTRE_X       (BOX_W / 2)           /* ship anchor x (152)            */
#define CENTRE_Y       (BOX_H / 2)           /* ship anchor y (114)            */

/* --- Motion: real inertia, no drag ------------------------------------
 *  UP and DOWN are mirror images: UP adds THRUST_IMPULSE along the nose, DOWN
 *  adds BRAKE_IMPULSE (half as much) along the OPPOSITE of the nose.  Neither
 *  is a "slow down" key - DOWN is just a half-weight reverse engine.  Nothing
 *  else slows the ship; it coasts.  |v| is capped at SPEED_MAX. */
#define THRUST_IMPULSE   SCALE_SF(1.2f)  /* speed vector added per distinct UP key-down (320: 1.2) */
#define BRAKE_IMPULSE    (THRUST_IMPULSE * 0.5f)  /* ... and per DOWN, reversed   */
#define SPEED_MAX        SCALE_SF(4.0f)  /* hard cap on ship speed (vector magnitude) (320: 4.0)   */
#define ROT_STEP         0.060f  /* radians per frame while LEFT/RIGHT held (angular - unscaled)   */

/* --- Flip-and-burn (ENTER): autopilot that turns the ship to face retrograde
 *  then burns the main engine to kill velocity.  Takes real time - manual
 *  controls are locked out during it and an alien can still hit you. */
#define FNB_ROT_STEP     ROT_STEP  /* flip turns at the manual rate            */
#define FNB_BURN_FRAMES  12        /* burn frames once the flip completes      */
#define FNB_MIN_SPEED    SCALE_SF(0.15f)  /* below this there is nothing to flip for (320: 0.15) */
#define FNB_OFF  0
#define FNB_FLIP 1
#define FNB_BURN 2

/* --- AUTOPILOT: a "hop" autopilot.  A toggles it on/off; M also drops back to
 *  MANUAL navigation (the default when autopilot is off).  In autopilot each
 *  ARROW key launches a hop in that screen direction - the ship turns to face
 *  that cardinal, accelerates to AUTO_CRUISE ("fire twice"), coasts, then
 *  retro-burns to a dead stop exactly AUTO_DIST px away (about one play-box)
 *  and snaps onto that line.  J / K hand-rotate the hull CCW / CW between hops.
 *  Manual flight keys are locked while a hop runs. */
#define AUTO_DIST       BOX_H                  /* one hop ~= one play-box (px) *
                                               * (shorter box dim: fits any dir) */
#define AUTO_CRUISE     (THRUST_IMPULSE * 2.0f) /* cruise speed = two UP pulses */
#define AUTO_IDLE  0
#define AUTO_TURN  1     /* rotating to the screen cardinal                    */
#define AUTO_MOVE  2     /* accelerate to cruise, then coast (main engine)     */
#define AUTO_BRAKE 3     /* retro-pulse to a stop (front engine only)          */
#define MANUAL_TXT_MS  1500   /* "MANUAL" flashes this long after leaving autopilot */

/* --- Starfield: continuous per-star depth --------------------------- */
/* STAR_COUNT scales with box area so a bigger starfield keeps its density.
 * 320x320 -> 165 exactly; SDL 640x480 -> ~495.  (static array in game.c) */
#define STAR_COUNT       (165 * SCREEN_W * SCREEN_H / (320 * 320))
#define DEPTH_MIN        0.30f   /* nearest star = 1.0; smaller = farther/slower */
#define STAR_LUM_MIN     80      /* luminance of the farthest stars (0..255)     */
#define STAR_FAR_TINT    18      /* extra blue on the farthest stars             */

/* --- Speed streaks: fast stars stretch from a dot into a short line -- */
#define STREAK_MIN       SCALE_SF(1.8f)   /* px/frame of travel before a star streaks (320: 1.8) */
#define STREAK_GAIN      1.8f    /* streak length as a multiple of that travel (ratio - unscaled) */
#define STREAK_MAX       SCALE_SF(18.0f)  /* longest streak, px (320: 18)                        */

/* --- Ship (filled triangle, local coords, nose towards -y) ------------- */
#define SHIP_NOSE      SCALE_S(11)     /* nose distance ahead of centre (320: 11)   */
#define SHIP_TAIL      SCALE_S(7)      /* tail distance behind centre   (320: 7)    */
#define SHIP_HALF      SCALE_S(7)      /* half the tail width           (320: 7)    */

/* --- Engine flames --------------------------------------------------- */
#define FLAME_HOLD_MS  500    /* main flame stays lit this long after an UP  */
#define FLAME_HALF     SCALE_S(6)      /* half-width of the main flame base  (320: 6)  */
#define FLAME_LEN      SCALE_S(20)     /* main flame length past the tail    (320: 20) */
#define FLAME_FLICKER  SCALE_S(6)      /* extra 0..N px on the main flame each frame   */
#define BRAKE_HOLD_MS  500    /* front flames stay lit this long after DOWN  */
#define BRAKE_OFF_X    SCALE_S(4)      /* front-flame lateral offset from centre       */
#define BRAKE_OFF_Y    SCALE_S(-4)     /* front-flame anchor, ahead of centre (-y)     */
#define BRAKE_HALF     SCALE_S(2)      /* half-width of a front-flame base             */
#define BRAKE_LEN      SCALE_S(8)      /* front-flame length, pointing forward         */

/* --- Attitude (RCS) jet: a small side flare while LEFT / RIGHT turns the
 *  hull.  It shows on the side OPPOSITE the turn (turn right -> port jet) and
 *  points outward, so its reaction yaws the nose the way you asked. */
#define RCS_OFF_X      SCALE_S(5)      /* jet base: lateral offset from centreline     */
#define RCS_OFF_Y      SCALE_S(-3)     /* jet base: just ahead of centre (-y)          */
#define RCS_HALF       SCALE_S(2)      /* half-height of the jet base (along the hull) */
#define RCS_LEN        SCALE_S(7)      /* how far the flare sticks out sideways        */

/* --- Torpedoes --------------------------------------------------------- */
#define MAX_TORPEDOES    20      /* hard cap on player torpedoes alive at once  */
#define TORPEDO_SPEED   SCALE_SF(12.0f)  /* px/frame - MUST stay above SPEED_MAX (320: 12) */
#define TORPEDO_LEN     SCALE_S(4)       /* tracer streak length, px (320: 4)              */
#define AMMO_MAX       300     /* rounds carried; SPACE spends one at a time */

/* --- Alien torpedoes: only 1-in-ALIEN_SHOOTER_EVERY aliens carry any.  A
 *  shooter is STATIONARY while it has torpedoes (it can move again once out).
 *  After its square first comes on the play box it needs ALIEN_ARM_MS to "load
 *  up" before the first shot, then fires no faster than the re-fire gap:
 *  ALIEN_FIRE_MS at level 3, doubled (half the rate) at levels 1-2.  Each
 *  shot is re-aimed at the ship's position right then and flies a fixed grid
 *  line - so it is dodgeable by moving.  Slower than player fire; the shield
 *  stops it.  The fire "pew" is the player one played in reverse.
 *  A `rookie` shooter is a bad shot that improves: round 1 aims ALIEN_ROOKIE_MISS
 *  px below the ship, the miss shrinking linearly to 0 by round
 *  ALIEN_ROOKIE_SHOTS - every shot from then on is dead on.  The ramp counts
 *  the alien's own `shots`, so it does not depend on clip size or reloads.
 *  Any shooter that empties its clip (shots > 0, torpedoes == 0) then charges
 *  the ship to ram it, at ANY difficulty. */
#define ALIEN_SHOOTER_EVERY 4   /* every Nth created alien is a shooter (25%) */
#define ALIEN_TORPEDOES       7   /* a shooter starts with this many           */
#define ALIEN_TORPEDO_MAX    16  /* alien tracers alive at once (array size)  */
#define ATORPEDO_SPEED     SCALE_SF(6.0f)  /* alien torpedo px/frame (< TORPEDO_SPEED) (320: 6) */
#define ALIEN_FIRE_MS    1000   /* re-fire gap, ms (x2 at levels 1-2)        */
#define ALIEN_ARM_MS     2000   /* ... but not until this long after it is   *
                                 *     first seen on the box (loading up)    */
#define ALIEN_ROOKIE_MISS  SCALE_S(20)  /* a rookie's 1st shot aims this many px low (320: 20) */
#define ALIEN_ROOKIE_SHOTS  5   /* ... shrinking to 0 by this shot (dead on after) */

/* --- Shields: a 0..100 level (percent).  S raises it to SHIELD_START and
 *  spends SHIELD_PWR_COST power (so it can't go up without that much power);
 *  there is no timer and no per-second drain.  It only goes DOWN, by damage:
 *  SHIELD_HIT_TORPEDO per alien torpedo blocked, SHIELD_HIT_RAM per alien that
 *  rams the ship.  At 0 it is down and hull DAMAGE (below) starts accruing.
 *  Shown as its own cyan gauge. */
#define SHIELD_R          SCALE_S(16)   /* hex-ring radius around the ship, px (320: 16) */
#define SHIELD_START      100  /* level when raised                         */
#define SHIELD_PWR_COST   20   /* Pwr spent to raise it                     */
#define SHIELD_HIT_TORPEDO 1    /* level lost per alien torpedo blocked       */
#define SHIELD_HIT_RAM    20   /* level lost per alien that rams the ship   */

/* --- Hull DAMAGE: a separate 0..100 metric (NOT the shield), starts at 0.
 *  Only accrues while the shield is DOWN: SHIP_DMG_TORPEDO per alien torpedo
 *  through the hull, SHIP_DMG_RAM per ram (the alien is wrecked in the crash
 *  too).  The crew mends DMG_REPAIR_STEP every DMG_REPAIR_MS.  At >= DMG_MAX
 *  the rocket is destroyed.  (Power reaching 0 also ends the game - life
 *  support.)  Shown as its own gauge (5th bar) whose fill RISES with the damage.
 *  The hull is drawn COL_SHIP_WOUND (yellow) - unless the shield is up - once
 *  damage passes SHIP_WOUND_ON OR power drops below SHIP_WOUND_ON percent. */
#define SHIP_DMG_TORPEDO   10
#define SHIP_DMG_RAM      40
#define DMG_REPAIR_STEP    1     /* damage points mended per repair tick        */
#define DMG_REPAIR_MS      2000  /* ms between repair ticks (crews work slowly) */
#define SHIP_WOUND_ON     33     /* hull yellow once damage > this, or power < this% */
#define DMG_MAX          100

/* --- Alien internal damage: each alien carries its own 0..100.  A player
 *  torpedo does ALIEN_HIT_DMG; at >= ALIEN_DMG_MAX the alien is destroyed (so
 *  it takes two hits).  Over ALIEN_WOUND_ON it turns yellow ("wounded"); it
 *  goes back to red only once repair brings it below ALIEN_WOUND_OFF
 *  (hysteresis).  Heals at the same rate as the hull (DMG_REPAIR_STEP every
 *  DMG_REPAIR_MS). */
#define ALIEN_HIT_DMG     60
#define ALIEN_DMG_MAX    100
#define ALIEN_WOUND_ON    50   /* dmg > this  -> turn yellow                */
#define ALIEN_WOUND_OFF   39   /* dmg < this  -> back to red (can eat a hit) */

/* --- Enemy aliens ---------------------------------------------------- *
 *  Spawn ALIEN_SPAWN_MIN..alien_spawn_max px from the ship on a random heading
 *  and just drift.  The frame an alien's square touches the play box ("it can
 *  see us") it stops for SPOT_PAUSE frames, then charges the ship for good.  A
 *  drifter that never gets seen and wanders past ALIEN_CULL_DIST is recycled.
 *  ALIEN_SPAWN_MIN is > the box's corner-to-centre distance (190 px) so a
 *  fresh alien is never placed inside the visible screen.
 *
 *  alien_max / alien_speed / alien_spawn_max are runtime VARIABLES (defined in
 *  game.c), not #defines.  run_game() sets alien_max from the chosen
 *  difficulty (ALIEN_MAX_L1/L2/L3); the rest keep their *_INIT start values. */
#define ALIEN_MAX_INIT        50   /* alien_max fallback (level 0 / unknown)   */
#define ALIEN_MAX_L1          20   /* "Easy Aliens"    - total that ever spawn */
#define ALIEN_MAX_L2          40   /* "Regular Aliens"                         */
#define ALIEN_MAX_L3          60   /* "Hard Aliens"                            */
#define ALIEN_SPEED_INIT      BRAKE_IMPULSE   /* alien_speed start: the rocket's crawl */
#define ALIEN_SPAWN_MAX_INIT  SCALE_SF(1000.0f)  /* alien_spawn_max start (320: 1000)  */
#define ALIEN_ONSCREEN  8      /* how many exist at once (array size)       */
#define ALIEN_SIZE      SCALE_S(14)    /* square edge, about the ship's size (320: 14) */
#define ALIEN_HALF      SCALE_S(7)
#define ALIEN_SPAWN_MIN SCALE_SF(200.0f)  /* fresh alien this far off - always off-screen (320: 200) */
#define ALIEN_CULL_DIST SCALE_SF(1400.0f) /* an unseen drifter past this is recycled (320: 1400)     */
#define ALIEN_INTRO_X   SCALE_S(110)   /* levels 1-3: seed one alien this far RIGHT of
                                          the box centre so the player sees a target
                                          from the first frame ...  (320: 110)        */
#define ALIEN_INTRO_YMIN SCALE_S(40)   /* ... at a random height |dy| in this band     */
#define ALIEN_INTRO_YMAX SCALE_S(60)   /*     above OR below centre (never ship-level)  */
#define IDLE_ROOKIE_MS  30000  /* levels 1-3: no key for this long -> stop the *
                                *  ship (flip-and-burn) then spawn / reload a   *
                                *  rookie shooter to nudge the player           */
#define SPOT_PAUSE      6      /* frames an alien freezes when it spots you  */
#define ALIEN_FLASH     5      /* frames of bright flash before it's removed */
#define GAMEOVER_FLASH  8      /* frames the rocket flashes before it's gone */
#define LRS_LEN        SCALE_S(5)     /* Long Range Sensors blip: length toward centre (320: 5) */
#define LRS_HALF       SCALE_S(3)     /* ... half its base width                       (320: 3) */

/* --- Fuel: spent on manoeuvring; at 0 NOTHING responds (no thrust,
 *     no brake, no turning) ----------------------------------------------- */
#define FUEL_MAX       500     /* full tank                                 */
#define FUEL_COST_FWD  2       /* spent per UP (forward) thrust / burn frame */
#define FUEL_COST_REV  1       /* spent per DOWN (reverse) thrust           */
#define FUEL_ROT_FRAMES 6      /* -1 fuel per this many frames of continuous
                                * turning (manual OR flip-and-burn): ~5/sec at
                                * 30 fps, ~4-5 for a 90 deg turn.  Frame-counted,
                                * so the cost no longer rides on the keyboard's
                                * key-repeat rate. */

/* --- Power (Power gauge): spent to raise the shield (SHIELD_PWR_COST, one-off)
 *     and drained by the cloak (CLOAK_DRAIN per CLOAK_DRAIN_MS) ------------- */
#define POWER_MAX      300     /* full charge                               */

/* --- Cloak (C): the ship vanishes and aliens lose it, but it can't fire,
 *     can't raise shields, and thrust is capped at the slowest crawl ----- */
#define CLOAK_DRAIN_MS 1000    /* cloak spends CLOAK_DRAIN per this long     */
#define CLOAK_DRAIN    2       /* power units per CLOAK_DRAIN_MS while cloaked */

/* --- Status strip (screen coords, in the strip below the box) ---------- *
 *  Compiled out entirely when SHOW_STATUS == 0.  All Y positions are BOX_H-relative
 *  (STATUS_BY) so the strip rides up tight under the box for any BOX_H.  FIVE
 *  short bars stacked FLUSH (no gap) below the box - Fuel, Power, Ammo, Shield,
 *  Damage - each with a readout to its right ("Fuel nn%" / "Power nn%" /
 *  "Ammo cur/max" / "Shield nn%" / "Damage nn%"); then the "Lv N" /
 *  "Kills NN/MM" (or "No Aliens" at level 0) / "CLOAKED" readouts in a column to
 *  the right of those; the coord readout below the stack.  (No speed bar.)
 *  GAUGE_H is also the row pitch (= TEXT_CH); STATUS_BY is rounded so every
 *  "(STATUS_BY + n*GAUGE_H + 1) / TEXT_CH" readout lands on the glyph grid.
 *  At 320x320, TEXT_CH is 10 and every value below is its V1.00A number. */
/* Start-screen body: a fixed 9-row block (version .. prompt) whose top row is
 * MENU_R0.  16 on the 320 panel (V1.00A exactly); on a shorter screen it is
 * pulled up just enough to keep the last row on-panel.  Rows are then
 * consecutive (R0, R0+2, R0+3.. , R0+8) so they never collide. */
#define MENU_MIN(a, b)  ((a) < (b) ? (a) : (b))
#define MENU_R0         MENU_MIN(16, SCREEN_H / TEXT_CH - 9)

#define STATUS_BY      (((BOX_H + TEXT_CH - 1) / TEXT_CH) * TEXT_CH + (TEXT_CH - 1))
#define GAUGE_X        SCALE_X(6)    /* stacked bars: outline left edge    (320: 6)  */
#define GAUGE_W        SCALE_X(75)   /* stacked bars: outline width        (320: 75) */
#define GAUGE_H        TEXT_CH       /* bar height AND stack pitch = a text row (320: 10) */
#define GAUGE_TXT_COL  12      /* stacked readouts: glyph col, just past the bar */
#define FUEL_GAUGE_Y   STATUS_BY                       /* slot 0 (top)          */
#define FUEL_TXT_ROW   ((STATUS_BY + 1) / TEXT_CH)
#define PWR_GAUGE_Y    (STATUS_BY + GAUGE_H)           /* slot 1               */
#define PWR_TXT_ROW    ((STATUS_BY + GAUGE_H + 1) / TEXT_CH)
#define AMMO_GAUGE_Y   (STATUS_BY + 2 * GAUGE_H)       /* slot 2               */
#define AMMO_TXT_ROW   ((STATUS_BY + 2 * GAUGE_H + 1) / TEXT_CH)
#define SHIELD_GAUGE_Y (STATUS_BY + 3 * GAUGE_H)       /* slot 3               */
#define SHIELD_GTXT_ROW ((STATUS_BY + 3 * GAUGE_H + 1) / TEXT_CH)
#define DMG_GAUGE_Y    (STATUS_BY + 4 * GAUGE_H)       /* slot 4 (bottom)      */
#define DMG_GTXT_ROW   ((STATUS_BY + 4 * GAUGE_H + 1) / TEXT_CH)
#define KILLS_TXT_COL  26      /* Lv / Kills / CLOAKED column,               */
                               /* a couple of blanks right of the readouts  */
#define LVL_TXT_ROW    FUEL_TXT_ROW                    /* "Lv N"    - top    */
#define CLOCK_TXT_COL  (KILLS_TXT_COL + 5)             /* "MM:SS" just right of "Lv N" */
#define CLOCK_TXT_ROW  LVL_TXT_ROW
#define GAME_LEN_S     900     /* hard time limit, ALL levels: the MM:SS clock
                                * counts this DOWN from 15:00; at 00:00 it is
                                * GAME OVER (15 * 60 = 900 s)                    */
#define SCORE_VAL_ROW  ((STATUS_BY + GAUGE_H + 1) / TEXT_CH)   /* "Kills.." / "No Aliens" */
#define CLK_TXT_COL    KILLS_TXT_COL
#define CLK_TXT_ROW    ((STATUS_BY + 2 * GAUGE_H + 1) / TEXT_CH)  /* "CLOAKED"       */
#define UNIV_TXT_COL   21      /* universal-coords readout glyph column     */
#define UNIV_TXT_ROW   ((STATUS_BY + 5 * GAUGE_H + 1) / TEXT_CH)  /* below the stack */
/* "GAME OVER" (9 glyphs) + "press any key to restart" (24), centred in the
 * current BOX_W x BOX_H play box (glyph cell TEXT_CW x TEXT_CH). */
#define GO_TXT_COL     (((BOX_W / TEXT_CW) - 9) / 2)
#define GO_TXT_ROW     (((BOX_H / TEXT_CH) / 2) - 1)
#define GO_MSG_COL     (((BOX_W / TEXT_CW) - 24) / 2)
#define GO_MSG_ROW     (GO_TXT_ROW + 2)
#define QUIT_TXT_COL   9       /* Q-quit confirmation, two centred lines    */
#define QUIT_TXT_ROW   13
#define AUTO_TXT_COL    4       /* the one AUTOPILOT / MANUAL banner, two    */
#define AUTO_TXT_ROW    2       /* ... lines near the top of the box         */

/* --- "YOU WON IN <MM:SS>" screen (levels > 0, every alien killed).  big_text()
 *  draws at scale * YASAG_TEXT_SCALE px, so the centring terms carry that too.
 *  (Both WIN_L*_X are the V1.00A value at 320x320 / TEXT_SCALE 1.) */
#define WIN_SCALE_TXT  3      /* "YOU WON IN" block scale                       */
#define WIN_SCALE_CLK  4      /* the MM:SS clock, bigger                        */
#define WIN_L1_X       ((SCREEN_W - 10 * 8 * WIN_SCALE_TXT * YASAG_TEXT_SCALE) / 2)  /* 10 glyphs */
#define WIN_L1_Y       SCALE_Y(80)
#define WIN_L2_X       ((SCREEN_W - 5 * 8 * WIN_SCALE_CLK * YASAG_TEXT_SCALE) / 2)   /* "MM:SS" = 5 */
#define WIN_L2_Y       SCALE_Y(140)
#define WIN_MSG_COL    7      /* "Press any key to continue" (plain text)       */
#define WIN_MSG_ROW    (SCALE_Y(260) / TEXT_CH)    /* 320: row 26                */
#define WIN_SONG_MS    5000   /* target length of the victory jingle           */

/* --- HINTS.  Only the two "with hints" difficulties, 0 ("No aliens") and 1
 *  ("Easy Aliens (with hints)"), ever draw these.  Each is a big_text caption in
 *  COL_HINT (grey), shown for HINT_MS, along the bottom of the box; the "next
 *  alien" hint (level 1) also draws a grey isosceles triangle centred on the
 *  box, apex HINT_ARROW_TIP px toward the nearest off-box alien, base
 *  HINT_ARROW_BACK px the other way, 2*HINT_ARROW_HALF wide.  One caption row;
 *  priority move > next-alien > shields > fire > quit:
 *    - "Arrow Keys to Move":  levels 0 and 1, once as the game starts;
 *    - "Next Alien This Way": level 1, the box is clear of aliens but some
 *      remain (or a flip-and-burn just ended with the box clear);
 *    - "Use S for Shields":   level 1, the hull just became "wounded" (see
 *      SHIP_WOUND_ON) and no shield is up;
 *    - "SPACE key to fire":   level 1, HINT_FIRE_AFTER_MS of play, no torpedo fired;
 *    - "Press Q to Quit":     level 0, at the 1-minute and 2-minute marks. */
#define HINT_MS         2000
#define HINT_FIRE_AFTER_MS 30000   /* no shot fired by this much play time -> nag */
#define HINT_Q_AT_1_MS  60000      /* level 0: "Press Q to Quit" at the 1-min mark */
#define HINT_Q_AT_2_MS  120000     /* ... and once more at the 2-min mark          */
/* HINT_TXT_SCALE is a big_text() scale, so it renders at HINT_TXT_SCALE *
 * YASAG_TEXT_SCALE px; the glyph terms below carry that.  Identity at 320/TS1. */
#define HINT_TXT_SCALE  2
#define HINT_TXT_PX     (HINT_TXT_SCALE * YASAG_TEXT_SCALE)         /* actual px per glyph unit */
#define HINT_TXT_Y      (BOX_H - BORDER - 10 * HINT_TXT_PX - 1)     /* just inside the bottom   */
#define HINT_NA_TXT_X   SCALE_X(2)   /* "Next Alien This Way" (19 glyphs)              */
#define HINT_S_TXT_X    ((BOX_W - 16 * 8 * HINT_TXT_PX) / 2)   /* "Use S for Shields" (16 glyphs) centred */
#define HINT_FIRE_TXT_X ((BOX_W - 17 * 8 * HINT_TXT_PX) / 2)   /* "SPACE key to fire" (17 glyphs) centred */
#define HINT_MOVE_TXT_X ((BOX_W - 18 * 8 * HINT_TXT_PX) / 2)   /* "Arrow Keys to Move" (18 glyphs) centred */
#define HINT_Q_TXT_X    ((BOX_W - 15 * 8 * HINT_TXT_PX) / 2)   /* "Press Q to Quit" (15 glyphs) centred    */
#define HINT_ARROW_TIP  SCALE_S(80)   /* arrow apex, px toward the nearest off-box alien (320: 80) */
#define HINT_ARROW_BACK SCALE_S(22)   /* ... base this far the other way          (320: 22) */
#define HINT_ARROW_HALF SCALE_S(24)   /* ... half the base width                  (320: 24) */

/* --- SHOW_STATUS == 0 (small-screen) low-resource warnings: 8-char lines stacked
 *  down the box's RIGHT edge, over the starfield.  fuel / power / ammo / shield
 *  warn YELLOW under 20% and RED under 10% (shield only while actually raised);
 *  hull damage warns YELLOW over 70% and RED over 85%. */
#define WARN_TXT_COL   ((IN_HI_X / TEXT_CW) - 9)   /* ~8 glyphs in from the right edge (320: 28) */
#define WARN_TXT_ROW0  2                     /* first of the 5 stacked rows      */

/* --- Universe: one big fixed pixel grid the whole game lives on ------ *
 *  The ship is screen-locked at the box centre; what really moves is its
 *  position on this grid, by the velocity vector each frame (the starfield
 *  scrolls the opposite way to show it).  Everything with a place in the
 *  world - the ship now, alien ships later - is addressed by a univ_x / univ_y
 *  pair, each wrapped to 0 .. UNIVERSE_SIZE-1.  An object's screen position is
 *  then box_centre plus its univ pos minus the ship's univ pos.  The grid
 *  wraps like a torus.  Coordinates are int32_t, so the grid could be as large
 *  as ~1e9 px; the real ceiling is univ_delta() returning the offset as a
 *  float (exact only to 2^24 ~ 16.7M) - past that, widen those to double or
 *  fixed-point, not the coordinate ints. */
#define UNIVERSE_SIZE  10000                /* pixels per axis (10 thousand)  */
#define UNIVERSE_HALF  (UNIVERSE_SIZE / 2)  /* ship starts dead centre       */

/* --- Loop -------------------------------------------------------------- */
#define FRAME_MS         33   /* ~30 frames per second                     */
#define INPUT_DRAIN_MAX  8    /* key events consumed per frame (I2C-bound) */

/* --- Audio (one square-wave voice per channel, 100..2115 Hz, no volume) -
 *  Three sounds, all short (highest priority wins the one voice):
 *    - the "alien destroyed" chime: a rising 3-note major arpeggio, played
 *      when a player torpedo kills an alien.  Beats everything for ~CHIME_MS;
 *    - the torpedo "pew": a quick downward sweep that owns the voice for ~PEW_MS
 *      (wins over the hiss);
 *    - the thruster "hiss": a burst of per-frame random high pitch (fake white
 *      noise on a square voice) that plays while UP / DOWN / a turn is firing a
 *      jet, held ~HISS_MS past the last such frame.
 *  Silent otherwise. */
#define PEW_MS         110   /* length of the torpedo "pew" chirp           */
#define PEW_HI         1900  /* pew start pitch, Hz (sweeps down to PEW_LO) */
#define PEW_LO         350   /* pew end pitch, Hz                          */
#define HISS_MS        130   /* thruster/turn hiss burst length, ms        */
#define HISS_LO        1750  /* hiss random pitch band, Hz ...             */
#define HISS_HI        2115  /* ... up to the driver's ceiling             */
#define CHIME_MS       220   /* "alien destroyed" chime, total length, ms   */
#define CHIME_N1       1047  /* rising major arpeggio: C6 ...               */
#define CHIME_N2       1319  /* ... E6 ...                                  */
#define CHIME_N3       1568  /* ... G6                                      */

/* --- Colours (RGB565) ------------------------------------------------- */
#define COL_SPACE     RGB(0, 0, 0)          /* interior background / erase   */
#define COL_BORDER    RGB(60, 90, 140)      /* box outline                  */
#define COL_SHIP      RGB(255, 255, 255)    /* ship hull (undamaged, shields off) */
#define COL_SHIP_WOUND RGB(255, 210, 0)     /* ship hull: damage > SHIP_WOUND_ON or power < that % (yellow) */
#define COL_SHIELD    RGB(90, 200, 255)     /* ship wireframe + ring, shields on */
#define COL_TORPEDO    RGB(120, 255, 120)    /* player torpedo tracer (green) */
#define COL_ATORPEDO   RGB(255, 255, 255)    /* alien torpedo tracer (white - *
                                             * red was too faint on the LCD) */
#define COL_BRAKE     RGB(235, 245, 255)    /* retro-brake flames (white)   */
#define COL_ALIEN       RGB(255, 70, 70)    /* enemy square (red, <= WOUND_OFF) */
#define COL_ALIEN_WOUND RGB(255, 210, 0)    /* enemy square, > 50% damaged (yellow) */
#define COL_ALIEN_FLASH RGB(255, 255, 190)  /* alien hit flash (bright)     */
#define COL_SHIP_FLASH  RGB(255, 90, 90)    /* rocket death flash           */
#define COL_GAUGE_FRAME RGB(90, 120, 160)   /* every gauge outline          */
#define COL_GAUGE_DRAIN RGB(24, 32, 28)     /* every gauge's spent portion  */
#define COL_GAUGE_FUEL  RGB(0, 210, 0)      /* fuel bar: remaining (green)  */
#define COL_GAUGE_AMMO  RGB(235, 195, 40)   /* ammo bar: remaining (amber)  */
#define COL_GAUGE_POWER RGB(180, 90, 235)   /* power bar: remaining (purple) */
#define COL_GAUGE_SHIELD RGB(90, 200, 255)  /* shield bar: remaining (cyan) */
#define COL_GAUGE_DMG   RGB(235, 80, 40)    /* damage bar: FILLS as you take hits (red-orange) */
#define COL_STATUS_TXT     RGB(210, 235, 210)  /* all gauge readout text       */
#define COL_HINT       RGB(150, 150, 150)   /* level-1 hint text + arrow (grey) */
#define COL_WARN_LOW    RGB(255, 220, 0)    /* SHOW_STATUS 0: "getting low" warning (yellow) */
#define COL_WARN_CRIT   RGB(255, 40, 40)    /* SHOW_STATUS 0: "critical" warning (red)       */

/* =========================================================================
 *  Internal declarations - the forward view of game.c
 *  -------------------------------------------------------------------------
 *  Everything game.c defines is declared here so that file can be pure
 *  function bodies, in this order: main(), then run_game() (the whole
 *  playthrough), then the helpers.  game.c is the only file that
 *  defines any of these; other translation units that include this header
 *  just don't reference them.
 * ===================================================================== */

/* The three top-level screens (defined near the end of game.c). */
int  splash_screen(void);
bool controls_screen(void);
void run_game(int level);

/* Enemy-alien model.  `aliens[]` + `aliens_spawned` are defined in
 * game.c; the behaviour and drawing live in its "Enemy aliens" section. */
typedef struct {
    int32_t ux, uy;         /* universal position of the square's centre   */
    float   fx, fy;          /* sub-pixel carry for the move step           */
    float   dirx, diry;      /* unit heading while drifting (pre-spot)      */
    float   speed;           /* px/frame this alien moves at; set from the   *
                            *  alien_speed default at spawn - a future      *
                            *  difficulty can make individual aliens faster */
    bool    hunting;         /* has it seen the ship and locked on?         */
    int     spot_pause;      /* frames frozen right after spotting the ship */
    int     state;           /* 0 empty, 1 alive, 2 flashing (about to die) */
    int     flash;           /* frames left in the death flash              */
    int     px, py;          /* last drawn top-left on screen (for erase)   */
    bool    pdrawn;          /* was it on screen last frame?                */
    int     lrs_x, lrs_y, lrs_dir;  /* last Long Range Sensors blip: apex + dir
                                    (0=none, 1=up, 2=down, 3=left, 4=right)   */
    bool    lrs_drawn;
    int     torpedoes;        /* alien torpedoes left; > 0 => stays put & shoots  *
                            *  (unless alien_ram_and_fire) - see game.c    */
    int     shots;           /* torpedoes it has fired: ramps a rookie's aim,    *
                            *  and > 0 with torpedoes == 0 marks a spent shooter */
    uint32_t next_fire_ms;  /* earliest boot-ms it may fire again              */
    uint32_t seen_ms;       /* boot-ms its square first came on the box (0 =   *
                            *  not yet); it can't fire until ALIEN_ARM_MS on   */
    bool    rookie;         /* deliberate bad shot: aims low, tightening to    *
                            *  dead-on by its ALIEN_ROOKIE_SHOTS-th round      */
    int     dmg;            /* internal damage 0..100; >= ALIEN_DMG_MAX = dead */
    bool    wounded;        /* true once dmg > ALIEN_WOUND_ON, false again     *
                            *  once repair drops it < ALIEN_WOUND_OFF (yellow) */
} alien_t;

/* Helpers, in roughly the order run_game() first reaches for them. */
int32_t univ_wrap(int32_t v);
float   univ_delta(int32_t a, int32_t b);

void stars_init(void);
void stars_update(float scroll_x, float scroll_y);

void ship_verts(float angle, int vx[3], int vy[3]);
void flame_verts(float angle, int len, int vx[3], int vy[3]);
void brake_verts(float angle, int side, int vx[3], int vy[3]);
void rcs_verts(float angle, int side, int vx[3], int vy[3]);
void tri_outline(const int vx[3], const int vy[3], uint16_t colour);
void tri_fill(const int vx[3], const int vy[3], uint16_t colour);
void shield_verts(float angle, int vx[6], int vy[6]);
void shield_draw(const int vx[6], const int vy[6], uint16_t colour);

void torpedoes_init(void);
bool torpedo_fire(float angle);
void torpedoes_update(void);
void torpedoes_hit_aliens(int32_t sux, int32_t suy,
                          uint32_t now_ms, uint32_t *chime_until_ms);

void atorpedoes_init(void);
void atorpedoes_update(int32_t sux, int32_t suy);
void atorpedoes_hit_ship(int32_t sux, int32_t suy,
                         int *shield_lvl, int *damage,
                         bool cloaked, bool *ship_dead);

void aliens_init(int level, int32_t sux, int32_t suy);
void alien_spawn_rookie(int32_t sux, int32_t suy, bool count);
void aliens_update(int32_t sux, int32_t suy, int level,
                   int *shield_lvl, int *damage, bool cloaked,
                   int *score, bool *ship_dead);
void aliens_heal(void);
void aliens_fire(int32_t sux, int32_t suy, int level, bool cloaked,
                 uint32_t now_ms, uint32_t *apew_until_ms);
void aliens_erase(void);
void aliens_respawn(int32_t sux, int32_t suy, int level);
void aliens_draw(int32_t sux, int32_t suy);

void lrs_erase(void);
void lrs_draw(int32_t sux, int32_t suy);

void sound_silence(void);
void sound_pew(int age_ms);
void sound_apew(int age_ms);
void sound_chime(int age_ms);
void sound_hiss(void);
void play_win_song(void);

void draw_border(void);
void draw_status_frames(void);
void gauge_draw(int y, int txt_row, uint16_t fill_col,
                const char *label, int value, int maxv, bool as_pct);

void go_bootsel(void);
uint8_t wait_key(void);
void big_text(int x, int y, int scale, uint16_t col, const char *s);
void big_text_centered(int y, int scale, uint16_t col, const char *s);
void draw_key_legend(void);

#endif /* YASAG_GAME_H */
