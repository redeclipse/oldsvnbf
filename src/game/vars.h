VARG(serverdebug, 0, 0, 3);

#ifdef STANDALONE
SVARG(defaultmap, "eight");
VARG(defaultmode, G_LOBBY, G_DEATHMATCH, G_MAX-1);
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);
#else
SVARG(defaultmap, "overseer");
VARG(defaultmode, G_LOBBY, G_LOBBY, G_MAX-1);
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);
#endif

SVARG(lobbymaps, "eight citadel gladiator refuge rivals smouldering tower warehouse warground overseer 4square siege exoticbase");
SVARG(missionmaps, "mpspbf1 mpspbf2");
SVARG(mainmaps, "eight citadel gladiator refuge rivals smouldering tower warehouse warground");
SVARG(ctfmaps, "citadel gladiator refuge rivals smouldering tower warehouse warground 4square");
SVARG(mctfmaps, "refuge 4square");
SVARG(stfmaps, "citadel gladiator refuge rivals smouldering tower warehouse warground 4square");
VARG(maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random
VARG(spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random

VARG(maxhealth, 0, 100, INT_MAX-1);
VARG(maxcarry, 0, 2, WEAPON_MAX-1);

VARG(regendelay, 0, 3, INT_MAX-1);
VARG(regentime, 0, 1, INT_MAX-1);
VARG(regenhealth, 0, 10, INT_MAX-1);
VARG(spawnprotecttime, 0, 3, INT_MAX-1);

VARG(itemsallowed, 0, 1, 2); // 0 = never, 1 = all but instagib, 2 = always
VARG(itemdropping, 0, 1, 1); // 0 = never, 1 = yes
VARG(itemspawntime, 1, 30, INT_MAX-1); // secs when items respawn
VARG(itemspawndelay, 0, 3, INT_MAX-1); // secs after map start items first spawn
VARG(kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenades, 3 = always

VARG(timelimit, 0, 10, INT_MAX-1);
VARG(intermlimit, 0, 10, INT_MAX-1); // secs before vote menu comes up
VARG(votelimit, 0, 20, INT_MAX-1); // secs before vote passes by default
VARG(duellimit, 0, 5, INT_MAX-1); // secs before duel goes to next round
VARG(duelclear, 0, 0, 1); // clear items in duel

VARG(teamdamage, 0, 1, 1); // damage team mates
VARG(teambalance, 0, 5, 6); // 0 = off, 2 = by effectiveness, 4 = ai number, humans eff (+1 = but force balance too), 6 = humans vs. bots
VARG(ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stffinish, 0, 0, 1); // finish when all bases captured

VARG(spawnweapon, 0, WEAPON_PLASMA, WEAPON_TOTAL-1);
VARG(instaspawnweapon, 0, WEAPON_RIFLE, WEAPON_TOTAL-1);

VARG(spawndelay, 0, 3, INT_MAX-1); // delay before spawning in most modes (except non-dm and stf, ctf, duel, lms)
VARG(stfspawndelay, 0, 5, INT_MAX-1); // .. in stf
VARG(ctfspawndelay, 0, 5, INT_MAX-1); // .. in ctf
FVARG(instaspawnscale, 0, 0.5f, 1000); // scale the above values by this in instagib
FVARG(paintspawnscale, 0, 1.f, 1000); // scale the above values by this in paintball

FVARG(botscale, 0, 1.f, 1000);
FVARG(botratio, 0, 0.5f, 1000);
VARG(botminskill, 1, 1, 101);
VARG(botmaxskill, 1, 100, 101);
VARG(botlimit, 0, MAXBOTS, MAXBOTS);

FVARG(damagescale, 0, 1.f, 1000);
FVARG(gravityscale, 0, 1.f, 1000);
FVARG(jumpscale, 0, 1.f, 1000);
FVARG(speedscale, 1e-3f, 1.f, 1000);
FVARG(hitpushscale, 0, 1.f, 1000);
FVARG(deadpushscale, 0, 1.f, 1000);
FVARG(wavepushscale, 0, 1.f, 1000);
