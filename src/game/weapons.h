enum
{
    WEAP_MELEE = 0, WEAP_PISTOL, WEAP_OFFSET, // end of unselectable weapon set
    WEAP_SWORD = WEAP_OFFSET, WEAP_SHOTGUN, WEAP_SMG, WEAP_FLAMER, WEAP_PLASMA, WEAP_RIFLE, WEAP_ITEM,
    WEAP_GRENADE = WEAP_ITEM, WEAP_ROCKET, // end of item weapon set
    WEAP_MAX
};
#define isweap(a)       (a >= 0 && a < WEAP_MAX)

enum { WEAP_F_NONE = 0, WEAP_F_FORCED = 1<<0 };
enum { WEAP_S_IDLE = 0, WEAP_S_PRIMARY, WEAP_S_SECONDARY, WEAP_S_RELOAD, WEAP_S_POWER, WEAP_S_SWITCH, WEAP_S_PICKUP, WEAP_S_WAIT };

enum
{
    IMPACT_GEOM = 1<<0, BOUNCE_GEOM = 1<<1, IMPACT_PLAYER = 1<<2, BOUNCE_PLAYER = 1<<3, RADIAL_PLAYER = 1<<4,
    COLLIDE_TRACE = 1<<5, COLLIDE_OWNER = 1<<6, COLLIDE_CONT = 1<<7, COLLIDE_STICK = 1<<8,
    COLLIDE_GEOM = IMPACT_GEOM|BOUNCE_GEOM, COLLIDE_PLAYER = IMPACT_PLAYER|BOUNCE_PLAYER, HIT_PLAYER = IMPACT_PLAYER|BOUNCE_PLAYER|RADIAL_PLAYER
};

enum
{
    HIT_NONE = 0, HIT_ALT = 1<<0, HIT_LEGS = 1<<1, HIT_TORSO = 1<<2, HIT_HEAD = 1<<3, HIT_FULL = 1<<4, HIT_PROJ = 1<<5,
    HIT_EXPLODE = 1<<6, HIT_BURN = 1<<7, HIT_MELT = 1<<8, HIT_DEATH = 1<<9, HIT_WATER = 1<<10, HIT_WAVE = 1<<11, HIT_SPAWN = 1<<12,
    HIT_LOST = 1<<13, HIT_KILL = 1<<14, HIT_CRIT = 1<<15, HIT_SFLAGS = HIT_KILL|HIT_CRIT
};

#define hithurts(x)     (x&HIT_BURN || x&HIT_EXPLODE || x&HIT_PROJ || x&HIT_MELT || x&HIT_DEATH || x&HIT_WATER)
#define doesburn(x,y)   (isweap(x) && WEAP2(x, burns, y&HIT_ALT) && y&HIT_FULL)

#define WEAPON(name, \
    w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, wa, wb1, wb2, wc, wd, we1, we2, we3, we4, we5, we6, \
    wf, wg, wh, wi, wj, wk, wl, wm, wn, wo, wp, wq, \
    x21, x22, x31, x32, x33, x34, x4, x5, x6, x7, x81, x82, x9, xa, xb, xc, xd, xe, \
    t0, t1, t2, t3, y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, ya, yb, yc, yd, ye1, ye2, yf1, yf2, yg, yh, \
    yi, yj, yk, yl, ym, yn, yo, yp, yq, yr, ys, yt \
 ) \
    GVAR(0, name##add, 1, w0, 10000);                   GVAR(0, name##max, 1, w1, 10000); \
    GVAR(0, name##sub1, 0, w2, 10000);                  GVAR(0, name##sub2, 0, w3, 10000); \
    GVAR(0, name##adelay1, 20, w4, 10000);              GVAR(0, name##adelay2, 20, w5, 10000);          GVAR(0, name##rdelay, 50, w6, 10000); \
    GVAR(0, name##damage1, -10000, w7, 10000);          GVAR(0, name##damage2, -10000, w8, 10000); \
    GVAR(0, name##speed1, -10000, w9, 10000);           GVAR(0, name##speed2, -10000, wa, 10000); \
    GVAR(0, name##power1, 0, wb1, 10000);               GVAR(0, name##power2, 0, wb2, 10000); \
    GVAR(0, name##time1, 0, wc, 10000);                 GVAR(0, name##time2, 0, wd, 10000); \
    GVAR(0, name##pdelay1, 0, we1, 10000);              GVAR(0, name##pdelay2, 0, we2, 10000); \
    GVAR(0, name##gdelay1, 0, we3, 10000);              GVAR(0, name##gdelay2, 0, we4, 10000); \
    GVAR(0, name##edelay1, 0, we5, 10000);              GVAR(0, name##edelay2, 0, we6, 10000); \
    GVAR(0, name##explode1, 0, wf, 10000);              GVAR(0, name##explode2, 0, wg, 10000); \
    GVAR(0, name##rays1, 1, wh, 100);                   GVAR(0, name##rays2, 1, wi, 100); \
    GVAR(0, name##spread1, 0, wj, 10000);               GVAR(0, name##spread2, 0, wk, 10000); \
    GVAR(0, name##zdiv1, 0, wl, 10000);                 GVAR(0, name##zdiv2, 0, wm, 10000); \
    GVAR(0, name##aiskew1, 0, wn, 10000);               GVAR(0, name##aiskew2, 0, wo, 10000); \
    GVAR(0, name##collide1, 0, wp, INT_MAX-1);          GVAR(0, name##collide2, 0, wq, INT_MAX-1); \
    GVAR(0, name##extinguish1, 0, x21, 2);              GVAR(0, name##extinguish2, 0, x22, 2); \
    GVAR(0, name##cooked1, 0, x31, 5);                  GVAR(0, name##cooked2, 0, x32, 5); \
    GVAR(0, name##guided1, 0, x33, 2);                  GVAR(0, name##guided2, 0, x34, 2); \
    GVAR(0, name##radial1, 0, x4, 1);                   GVAR(0, name##radial2, 0, x5, 1); \
    GVAR(0, name##burns1, 0, x6, 1);                    GVAR(0, name##burns2, 0, x7, 1); \
    GVAR(0, name##reloads, 0, x81, 1);                  GVAR(0, name##carried, 0, x82, 1);              GVAR(0, name##zooms, 0, x9, 1); \
    GVAR(0, name##fullauto1, 0, xa, 1);                 GVAR(0, name##fullauto2, 0, xb, 1);             GVAR(0, name##allowed, 0, xc, 3); \
    GVAR(0, name##critdash1, 0, xd, 10000);             GVAR(0, name##critdash2, 0, xe, 10000); \
    GFVAR(0, name##taper1, 0, t0, 1);                   GFVAR(0, name##taper2, 0, t1, 1); \
    GFVAR(0, name##taperspan1, 0, t2, 1);               GFVAR(0, name##taperspan2, 0, t3, 1); \
    GFVAR(0, name##elasticity1, -10000, y0, 10000);     GFVAR(0, name##elasticity2, -10000, y1, 10000); \
    GFVAR(0, name##reflectivity1, 0, y2, 360);          GFVAR(0, name##reflectivity2, 0, y3, 360); \
    GFVAR(0, name##relativity1, -10000, y4, 10000);     GFVAR(0, name##relativity2, -10000, y5, 10000); \
    GFVAR(0, name##waterfric1, 0, y6, 10000);           GFVAR(0, name##waterfric2, 0, y7, 10000); \
    GFVAR(0, name##weight1, -10000, y8, 10000);         GFVAR(0, name##weight2, -10000, y9, 10000); \
    GFVAR(0, name##radius1, 1, ya, 10000);              GFVAR(0, name##radius2, 1, yb, 10000); \
    GFVAR(0, name##kickpush1, -10000, yc, 10000);       GFVAR(0, name##kickpush2, -10000, yd, 10000); \
    GFVAR(0, name##hitpush1, -10000, ye1, 10000);       GFVAR(0, name##hitpush2, -10000, ye2, 10000); \
    GFVAR(0, name##slow1, 0, yf1, 10000);               GFVAR(0, name##slow2, 0, yf2, 10000); \
    GFVAR(0, name##aidist1, 0, yg, 10000);              GFVAR(0, name##aidist2, 0, yh, 10000); \
    GFVAR(0, name##partsize1, 0, yi, 10000);            GFVAR(0, name##partsize2, 0, yj, 10000); \
    GFVAR(0, name##partlen1, 0, yk, 10000);             GFVAR(0, name##partlen2, 0, yl, 10000); \
    GFVAR(0, name##frequency, 0, ym, 10000);            GFVAR(0, name##pusharea, 0, yn, 10000); \
    GFVAR(0, name##critmult, 0, yo, 10000);             GFVAR(0, name##critdist, 0, yp, 10000); \
    GFVAR(0, name##delta1, 1, yq, 10000);               GFVAR(0, name##delta2, 1, yr, 10000); \
    GFVAR(0, name##tracemult1, 1e-3f, ys, 10000);       GFVAR(0, name##tracemult2, 1e-3f, yt, 10000);

//  add     max     sub1    sub2    adly1   adly2   rdly    dam1    dam2    spd1    spd2     pow1   pow2    time1   time2
//  pdly1   pdly2   gdly1   gdly2   edly1   edly2   expl1   expl2   rays1   rays2   sprd1   sprd2
//  zdiv1   zdiv2   aiskew1 aiskew2
//  collide1                                                                collide2
//  ext1    ext2    cook1   cook2   guide1  guide2  radl1   radl2   brn1    brn2    rlds    crd     zooms   fa1     fa2
//  allw    cdash1  cdash2
//  tpr1    tpr2    tspan1  tspan2  elas1   elas2   rflt1   rflt2   relt1   relt2   wfrc1   wfrc2   wght1   wght2   rads1   rads2
//  kpsh1   kpsh2   hpsh1   hpsh2   slow1   slow2   aidst1  aidst2  psz1    psz2    plen1   plen2   freq    push
//  cmult   cdist   dlta1   dlta2   tmult1  tmult2
WEAPON(melee,
    1,      1,      0,      0,      500,    750,    0,      20,     40,     0,      0,      0,      0,      100,    80,
    20,     0,      0,      0,      40,     40,     0,      0,      1,      1,      1,      1,
    1,      1,      1,      1,
    IMPACT_PLAYER|COLLIDE_TRACE,                                            IMPACT_PLAYER|COLLIDE_TRACE,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,
    2,      500,    500,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,
    -1,     -1,     250,    250,    0.15f,  0.15f,  24,     24,     1,      2,      0,      0,      0,      1,
    2,      0,      10,     10,     2,      4
);
WEAPON(pistol,
    10,     10,     1,      1,      150,    300,    1000,   40,     40,     2000,   2000,   0,      0,      2000,   2000,
    0,      0,      0,      0,      40,     40,     0,      0,      1,      1,      2,      2,
    1,      1,      100,    100,
    IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,                                IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      0,      0,      0,      1,
    2,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0.05f,  0.05f,  2,      2,      0,      0,      1,      1,
    2,      2,      150,    150,    0.05f,  0.05f,  300,    300,    1,      1,      10,     10,     1,      1,
    4,      16,     10,     10,     1,      1
);
WEAPON(sword,
    1,      1,      0,      0,      500,    750,    50,     50,     100,    0,      0,      0,      0,      300,    300,
    100,    100,    0,      0,      40,     40,     0,      0,      1,      1,      1,      1,
    1,      1,      1,      1,
    IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_CONT,                               IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_CONT,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      0,      1,      1,
    2,      500,    500,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,
    -1,     -1,     250,    500,    0.25f,  0.5f,   32,     32,     1,      1.25f,  0,      0,      1,      1,
    2,      0,      10,     10,     2,      1.5f
);
WEAPON(shotgun,
    1,      8,      1,      2,      400,    800,    600,    10,     10,     1500,   1500,   0,      0,      500,    200,
    0,      0,      0,      0,      40,     20,     0,      16,     10,     15,     40,     40,
    1,      1,      10,     10,
    BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_OWNER,                  IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_OWNER,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      0,      0,      0,
    2,      0,      0,
    0.15f,  0.15f,  0,      0,      0.5f,   0.35f,  50,     50,     0.05f,  0.05f,  2,      2,      25,     25,     1,      1,
    15,     15,     25,     20,     0.2f,   0.2f,   150,    150,    1,      1,      50,     50,     1,      1.5f,
    2,      6,      10,     10,     1,      1
);
WEAPON(smg,
    40,     40,     1,      5,      75,     250,    1500,   20,     20,     5000,   3000,   0,      0,      750,    500,
    0,      0,      0,      0,      40,     40,     0,      0,      1,      5,      5,      5,
    4,      1,      20,     20,
    BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_OWNER,                  IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      0,      1,      1,
    2,      0,      0,
    0.15f,  0.15f,  0,      0,      0.75f,  0.5f,   30,     30,     0.05f,  0.05f,  2,      2,      0,      0,      1,      1,
    0.5f,   3,      50,     60,     0.15f,  0.15f,  300,    300,    1,      1,      40,     40,     1,      1.5f,
    3,      12,     10,     10,     1,      1
);
WEAPON(flamer,
    50,     50,     1,      10,     100,    500,    2000,   8,      16,     200,    200,    0,      500,    250,    750,
    0,      50,     0,      0,      40,     40,     12,     24,     1,      5,      10,     5,
    0,      0,      10,     10,
    BOUNCE_GEOM|IMPACT_PLAYER,                                              IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_OWNER,
    2,      2,      0,      1,      0,      0,      1,      1,      1,      1,      1,      1,      0,      1,      0,
    2,      0,      0,
    0,      0,      0,      0,      0.15f,  0,      45,     0,      0.95f,  0.5f,   1,      1,      -300,   50,     1,      1,
    0.25f,  1,      20,     40,     0,      0,      20,     60,     12,     24,     0,      5,      2,      1.5f,
    5,      12,     10,     10,     1,      1
);
WEAPON(plasma,
    20,     20,     1,      20,     500,    1000,   2000,   25,     10,     1500,   35,     0,      2000,   500,    5000,
    0,      100,    0,      0,      40,     200,    18,     48,     1,      1,      5,      1,
    0,      0,      50,     10,
    IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_OWNER,                                IMPACT_GEOM|RADIAL_PLAYER|COLLIDE_OWNER|COLLIDE_STICK,
    1,      0,      0,      1,      0,      0,      1,      1,      0,      0,      1,      1,      0,      1,      0,
    2,      0,      0,
    0.5f,   0.75f,  0,      0.25f,  0,      0,      0,      0,      0.125f, 0.175f, 1,      1,      0,      0,      4,      2,
    3,      6,      50,     -50,    0.1f,   0.1f,   200,    50,     18,     44,     0,      0,      2,      1.5f,
    3,      12,     10,     10,     1,      1
);
WEAPON(rifle,
    5,      5,      1,      1,      750,    500,    1500,   50,     100,    2500,   5000,   0,      0,      5000,   5000,
    0,      0,      0,      0,      40,     40,     24,     0,      1,      1,      1,      0,
    0,      0,      40,     40,
    IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_OWNER|COLLIDE_TRACE,                  IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_CONT,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      1,      0,      0,
    2,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      1,      0,      2,      2,      0,      0,      1,      1,
    5,      0,      100,    200,    0.1f,   0.25f,  600,    0,      1,      2,      256,    512,    1,      1.5f,
    2,      12,     10,     10,     1,      1
);
WEAPON(grenade,
    1,      2,      1,      1,      1000,   1000,   1500,   150,    150,    250,    250,    2500,   2500,   5000,   5000,
    200,    200,    0,      0,      40,     40,     48,     48,     1,      1,      1,      1,
    0,      0,      5,      5,
    BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER,                                IMPACT_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER|COLLIDE_STICK,
    0,      0,      2,      2,      0,      0,      0,      0,      1,      1,      0,      0,      0,      0,      0,
    3,      0,      0,
    0,      0,      0,      0,      0.5f,   0,      0,      0,      1,      1,      2,      2,      64,     64,     1,      1,
    5,      5,      500,    500,    0.1f,   0.1f,   400,    400,    2,      2,      0,      0,      2,      2,
    2,      0,      10,     10,     1,      1
);
WEAPON(rocket,
    1,      1,      1,      1,      1000,   1000,   1500,   150,     150,    1000,   250,    2500,   2500,  5000,   5000,
    0,      0,      0,      0,      40,     40,     64,     64,      1,      1,      1,      1,
    0,      0,      10,     10,
    IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_OWNER,                                IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_OWNER,
    0,      0,      2,      2,      0,      1,      0,      0,      1,       1,      0,      1,      0,      0,      0,
    1,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      2,      2,      0,      0,      1,      1,
    15,     15,     500,    500,    0.25f,  0.25f,  400,    400,    3,      3,      0,      0,      3,      3,
    2,      0,      10,     10,     1,      1
);

struct weaptypes
{
    int     anim,               colour,         sound,      esound,     fsound,     rsound,         espeed;
    bool    melee,      traced,     muzzle,     eject;
    float   thrown[2],              halo,       esize;
    const char *name,       *text,  *item,                      *vwep, *hwep,                     *proj,                  *eprj;
};
#ifdef GAMESERVER
weaptypes weaptype[] =
{
    {
            ANIM_MELEE,         0x444444,       S_MELEE,    -1,          -1,         -1,             1,
            true,       true,       false,      false,
            { 0, 0 },               1,          0,
            "melee",        "\fd",  "",                         "", "",                        "",                     ""
    },
    {
            ANIM_LIGHT,         0x888888,       S_PISTOL,   S_BZAP,     S_WHIZZ,    -1,             10,
            false,      false,      true,       true,
            { 0, 0 },               4,          0.35f,
            "pistol",       "\fa",  "weapons/pistol/item",      "weapons/pistol/vwep", "weapons/pistol/hwep",     "",                     "projs/cartridge"
    },
    {
            ANIM_WIELD,          0x2222FF,      S_SWORD,    -1,          -1,         -1,             1,
            true,       true,       true,       false,
            { 0, 0 },               14,         0,
            "sword",     "\fb",     "weapons/sword/item",       "weapons/sword/vwep", "weapons/sword/hwep",    "",                     ""
    },
    {
            ANIM_HEAVY,         0xFFFF22,       S_SHOTGUN,  S_BZAP,     S_WHIZZ,    S_RICOCHET,     10,
            false,      false,      true,       true,
            { 0, 0 },               6,          0.45f,
            "shotgun",      "\fy",  "weapons/shotgun/item",     "weapons/shotgun/vwep", "weapons/shotgun/hwep",    "",                     "projs/shell"
    },
    {
            ANIM_LIGHT,         0xFF8822,       S_SMG,      S_BZAP,     S_WHIZZ,    S_RICOCHET,     20,
            false,      false,      true,       true,
            { 0, 0 },               5.5f,       0.35f,
            "smg",          "\fo",  "weapons/smg/item",         "weapons/smg/vwep", "weapons/smg/hwep",        "",                     "projs/cartridge"
    },
    {
            ANIM_HEAVY,         0xFF2222,       S_FLAMER,   S_BURN,     S_BURNING,  -1,             1,
            false,      false,      true,       false,
            { 0, 0 },               7,          0,
            "flamer",       "\fr",  "weapons/flamer/item",      "weapons/flamer/vwep", "weapons/flamer/hwep",     "",                     ""
    },
    {
            ANIM_LIGHT,         0x22FFFF,       S_PLASMA,   S_ENERGY,   S_HUM,      -1,             1,
            false,      false,      true,       false,
            { 0, 0 },               5,          0,
            "plasma",       "\fc",  "weapons/plasma/item",      "weapons/plasma/vwep", "weapons/plasma/hwep",     "",                     ""
    },
    {
            ANIM_HEAVY,         0xAA66FF,       S_RIFLE,    S_ENERGY,   S_BZZT,     -1,             1,
            false,      false,      true,       false,
            { 0, 0 },               7,          0,
            "rifle",        "\fv",  "weapons/rifle/item",       "weapons/rifle/vwep", "weapons/rifle/hwep",      "",                     ""
    },
    {
            ANIM_GRASP,         0x22FF22,       S_GRENADE,  S_EXPLODE,  S_BEEP, S_TINK,             1,
            false,      false,      false,      false,
            { 0.0625f, 0.0625f },   3,          0,
            "grenade",      "\fg",  "weapons/grenade/item",     "weapons/grenade/vwep", "weapons/grenade/hwep",    "weapons/grenade/proj", ""
    },
    {
            ANIM_HEAVY,         0x993311,       S_ROCKET,   S_EXPLODE,  S_WHIZZ,    -1,             1,
            false,      false,      true,      false,
            { 0, 0 },               8,          0,
            "rocket",      "\fn",  "weapons/rocket/item",       "weapons/rocket/vwep", "weapons/rocket/hwep",     "weapons/rocket/proj",  ""
    }
};
#define WEAPDEF(proto,name)     proto *sv_weap_stat_##name[] = {&sv_melee##name, &sv_pistol##name, &sv_sword##name, &sv_shotgun##name, &sv_smg##name, &sv_flamer##name, &sv_plasma##name, &sv_rifle##name, &sv_grenade##name, &sv_rocket##name };
#define WEAPDEF2(proto,name)    proto *sv_weap_stat_##name[][2] = {{&sv_melee##name##1,&sv_melee##name##2}, {&sv_pistol##name##1,&sv_pistol##name##2}, {&sv_sword##name##1,&sv_sword##name##2}, {&sv_shotgun##name##1,&sv_shotgun##name##2}, {&sv_smg##name##1,&sv_smg##name##2}, {&sv_flamer##name##1,&sv_flamer##name##2}, {&sv_plasma##name##1,&sv_plasma##name##2}, {&sv_rifle##name##1,&sv_rifle##name##2}, {&sv_grenade##name##1,&sv_grenade##name##2}, {&sv_rocket##name##1,&sv_rocket##name##2} };
#define WEAP(weap,name)         (*sv_weap_stat_##name[weap])
#define WEAP2(weap,name,second) (*sv_weap_stat_##name[weap][second?1:0])
#define WEAPSTR(a,weap,attr)    defformatstring(a)("sv_%s%s", weaptype[weap].name, #attr)
#else
extern weaptypes weaptype[];
#ifdef GAMEWORLD
#define WEAPDEF(proto,name)     proto *weap_stat_##name[] = {&melee##name, &pistol##name, &sword##name, &shotgun##name, &smg##name, &flamer##name, &plasma##name, &rifle##name, &grenade##name, &rocket##name };
#define WEAPDEF2(proto,name)    proto *weap_stat_##name[][2] = {{&melee##name##1,&melee##name##2}, {&pistol##name##1,&pistol##name##2}, {&sword##name##1,&sword##name##2}, {&shotgun##name##1,&shotgun##name##2}, {&smg##name##1,&smg##name##2}, {&flamer##name##1,&flamer##name##2}, {&plasma##name##1,&plasma##name##2}, {&rifle##name##1,&rifle##name##2}, {&grenade##name##1,&grenade##name##2}, {&rocket##name##1,&rocket##name##2} };
#else
#define WEAPDEF(proto,name)     extern proto *weap_stat_##name[];
#define WEAPDEF2(proto,name)    extern proto *weap_stat_##name[][2];
#endif
#define WEAP(weap,name)         (*weap_stat_##name[weap])
#define WEAP2(weap,name,second) (*weap_stat_##name[weap][second?1:0])
#define WEAPSTR(a,weap,attr)    defformatstring(a)("%s%s", weaptype[weap].name, #attr)
#endif
#define WEAPLM(a,b,c)           (a*(m_limited(b, c) || m_arena(b, c) ? GAME(limitedscale) : GAME(normalscale)))
#define WEAPEX(a,b,c,d,e)       (!m_insta(c, d) || m_arena(c, d) || a != WEAP_RIFLE ? int(ceilf(WEAPLM(WEAP2(a, explode, b)*e, c, d))) : 0)
#define WEAPSP(a,b,c,d,e)       (!m_insta(c, d) || m_arena(c, d) || a != WEAP_RIFLE ? WEAP2(a, spread, b)+(int(min(WEAP2(a, spread, b), 1)*e)) : 0)

WEAPDEF(int, add);
WEAPDEF(int, max);
WEAPDEF2(int, sub);
WEAPDEF2(int, adelay);
WEAPDEF(int, rdelay);
WEAPDEF2(int, damage);
WEAPDEF2(int, speed);
WEAPDEF2(int, power);
WEAPDEF2(int, time);
WEAPDEF2(int, pdelay);
WEAPDEF2(int, gdelay);
WEAPDEF2(int, edelay);
WEAPDEF2(int, explode);
WEAPDEF2(int, rays);
WEAPDEF2(int, spread);
WEAPDEF2(int, zdiv);
WEAPDEF2(int, aiskew);
WEAPDEF2(int, collide);
WEAPDEF2(int, extinguish);
WEAPDEF2(int, cooked);
WEAPDEF2(int, guided);
WEAPDEF2(int, radial);
WEAPDEF2(int, burns);
WEAPDEF(int, reloads);
WEAPDEF(int, carried);
WEAPDEF(int, zooms);
WEAPDEF2(int, fullauto);
WEAPDEF(int, allowed);
WEAPDEF2(int, critdash);
WEAPDEF2(float, taper);
WEAPDEF2(float, taperspan);
WEAPDEF2(float, elasticity);
WEAPDEF2(float, reflectivity);
WEAPDEF2(float, relativity);
WEAPDEF2(float, waterfric);
WEAPDEF2(float, weight);
WEAPDEF2(float, radius);
WEAPDEF2(float, kickpush);
WEAPDEF2(float, hitpush);
WEAPDEF2(float, slow);
WEAPDEF2(float, aidist);
WEAPDEF2(float, partsize);
WEAPDEF2(float, partlen);
WEAPDEF(float, frequency);
WEAPDEF(float, pusharea);
WEAPDEF(float, critmult);
WEAPDEF(float, critdist);
WEAPDEF2(float, delta);
WEAPDEF2(float, tracemult);

