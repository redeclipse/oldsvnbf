#ifdef STANDALONE
SVARG(defaultmap, "eight");
VARG(defaultmode, G_LOBBY, G_DEATHMATCH, G_MAX-1);
#else
SVARG(defaultmap, "overseer");
VARG(defaultmode, G_LOBBY, G_LOBBY, G_MAX-1);
#endif
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);

VARG(itemsallowed, 0, 1, 2); // 0 = never, 1 = all but instagib, 2 = always
VARG(itemdropping, 0, 2, 2); // 0 = never, 1 = yes but not kamakaze, 2 = yes with kamakaze
VARG(itemspawntime, 1, 30, 3600); // secs when items respawn
VARG(timelimit, 0, 15, 60);
VARG(votelimit, 0, 30, 120); // secs before vote passes by default

VARG(teamdamage, 0, 1, 1);
VARG(ctflimit, 0, 20, 100);
VARG(stflimit, 0, 0, 1);

VARG(spawngun, 0, GUN_PLASMA, GUN_MAX-1);
VARG(instaspawngun, 0, GUN_RIFLE, GUN_MAX-1);

VARG(botbalance, 0, 4, 32);
VARG(botminskill, 0, 60, 100);
VARG(botmaxskill, 0, 90, 100);

FVARG(damagescale, 0.1f, 1.f, 10);
FVARG(gravityscale, 0.1f, 1.f, 10);
FVARG(jumpscale, 0.1f, 1.f, 10);
FVARG(speedscale, 0.1f, 1.f, 10);
