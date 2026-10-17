// Level collision data: platforms and jump pads, per level.
//
// Rects are world-space bounding boxes in level-image coordinates
// (origin top-left, X right, Y down).

#ifndef LEVELS_H
#define LEVELS_H

typedef struct { int x0, y0, x1, y1; } Rect;

// Level 1 (2077 x 240)
#define LEVEL1_WIDTH 2077

// Goal: the yellow "NEXT" sign at the end of level 1.
// (The other yellow blob near x=1269 is a green pillar, not a goal.)
static const Rect level1_goals[] = {
	{ 1932,   0, 1984,  38},
};
#define LEVEL1_NUM_GOALS (sizeof(level1_goals)/sizeof(level1_goals[0]))
static const Rect level1_platforms[] = {
	{   62, 150,  440, 236},
	{  145,  66,  207, 101},
	{  323,  70,  392, 102},
	{  455, 119,  512, 148},
	{  543, 142,  681, 170},
	{  693,  88, 1073, 122},
	{  737,   1,  802,  41},
	{  812,  41,  878,  72},
	{  899,   1,  957,  37},
	{ 1922,  41, 2000,  87},
	{ 1515,  99, 1906, 160},
	{ 1079, 136, 1139, 160},
	{ 1079, 161, 1906, 214},
	{ 1079, 215, 1530, 231},
	{ 1079, 232, 1139, 236},
	{ 1275,  82, 1333, 159},
};
#define LEVEL1_NUM_PLATFORMS (sizeof(level1_platforms)/sizeof(level1_platforms[0]))
static const Rect level1_jumpPads[] = {
	{  259, 136,  310, 144},
	{ 1433, 147, 1485, 156},
};
#define LEVEL1_NUM_JUMPPADS (sizeof(level1_jumpPads)/sizeof(level1_jumpPads[0]))

// Level 2 (2191 x 240)
#define LEVEL2_WIDTH 2191
static const Rect level2_platforms[] = {
	{    0,  10,   76, 202},
	{    0, 202,  465, 239},
	{  223, 111,  441, 155},
	{  530, 140,  761, 187},
	{  782,  18,  832,  83},
	{  784, 113,  829, 239},
	{  875, 101,  934, 142},
	{  990, 140, 1057, 187},
	{ 1121, 112, 1181, 155},
	{ 1353, 150, 1408, 206},
	{ 1451, 180, 1958, 239},
	{ 2050, 207, 2122, 239},
};
#define LEVEL2_NUM_PLATFORMS (sizeof(level2_platforms)/sizeof(level2_platforms[0]))
// No jump pads. player_set_level() still needs a valid array pointer, so this
// placeholder exists but is never read (the count is 0).
static const Rect level2_jumpPads[1];
#define LEVEL2_NUM_JUMPPADS 0

// Evil Gumba slide zones: each entry is the patrol range the gumba slides
// along, at a fixed y. No gravity.
typedef struct { float x0, x1; float y; } GumbaZone;
static const GumbaZone level2_gumbas[] = {
	{  257.0f,  393.0f,  81.0f},
	{  843.0f,  979.0f, 161.0f},
	{ 1193.0f, 1329.0f, 161.0f},
	{ 1550.0f, 1686.0f, 150.0f},
	{ 1781.0f, 1917.0f, 150.0f},
};
#define LEVEL2_NUM_GUMBAS (sizeof(level2_gumbas)/sizeof(level2_gumbas[0]))

// Goal: the "NEXT" sign at the end of level 2, leading to the boss.
#define LEVEL2_GOAL_X0 2048
#define LEVEL2_GOAL_Y0 148
#define LEVEL2_GOAL_X1 2125
#define LEVEL2_GOAL_Y1 206

// Level 3: BOSS (1257 x 240)
#define LEVEL3_WIDTH 1257
static const Rect level3_platforms[] = {
	{   0,   9,   79, 175},   // left wall
	{   0, 175, 1106, 240},   // floor
};
#define LEVEL3_NUM_PLATFORMS (sizeof(level3_platforms)/sizeof(level3_platforms[0]))
// No jump pads. See level2_jumpPads.
static const Rect level3_jumpPads[1];
#define LEVEL3_NUM_JUMPPADS 0

// Big Gumba: its patrol range is 390,20 -> 810,129 in level coordinates.
// x0/x1 is the patrol range; the boss slides along it at y, floating.
#define LEVEL3_BOSS_X0 390.0f
#define LEVEL3_BOSS_X1 810.0f
#define LEVEL3_BOSS_Y   20.0f
#define LEVEL3_BOSS_H 109.0f          // height of the red box
#define LEVEL3_BOSS_SCALE (LEVEL3_BOSS_H / 43.0f)  // gumba.png is 60x43

#endif // LEVELS_H
