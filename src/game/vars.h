VARG(serverdebug, 0, 0, 3);

SVARG(defaultmap, "");
VARG(defaultmode, G_LOBBY, G_CTF, G_MAX-1);
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);

SVARG(missionmaps, "mpspbf1 mpspbf2");
SVARG(mainmaps, "bath bloodgrounds chaos citadel deadsimple deli dualstar eight enigma forge gladiator hollow longestyard nova panic refuge rivals smouldering tower tranquility warehouse warground wet venus");
SVARG(ctfmaps, "bath bloodgrounds chaos citadel deadsimple deli dualstar enigma forge gladiator hollow longestyard nova panic refuge rivals smouldering tower tranquility warehouse warground venus");
SVARG(mctfmaps, "bloodgrounds deadsimple enigma refuge");
SVARG(stfmaps, "bath bloodgrounds chaos citadel deadsimple deli dualstar enigma forge gladiator hollow longestyard nova panic refuge rivals smouldering tower tranquility warehouse warground wet venus");
VARG(maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random
VARG(spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random

VARG(maxhealth, 0, 100, INT_MAX-1);
VARG(overctfhealth, 0, 200, INT_MAX-1);
VARG(overstfhealth, 0, 200, INT_MAX-1);
VARG(maxcarry, 0, 2, WEAP_MAX-1);

VARG(regendelay, 0, 3, INT_MAX-1);
VARG(regenctfguard, 0, 1, INT_MAX-1);
VARG(regenstfguard, 0, 1, INT_MAX-1);
VARG(regentime, 0, 1, INT_MAX-1);
VARG(regenhealth, 0, 10, INT_MAX-1);
VARG(regenctfflag, 0, 20, INT_MAX-1);
VARG(regenstfflag, 0, 20, INT_MAX-1);
VARG(spawnprotecttime, 0, 3, INT_MAX-1);

VARG(itemsallowed, 0, 1, 2); // 0 = never, 1 = all but instagib, 2 = always
VARG(itemdropping, 0, 1, 1); // 0 = never, 1 = yes
VARG(itemspawntime, 1, 30, INT_MAX-1); // secs when items respawn
VARG(itemspawndelay, 0, 3, INT_MAX-1); // secs after map start items first spawn
VARG(kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always

VARG(timelimit, 0, 15, INT_MAX-1);
VARG(intermlimit, 0, 10, INT_MAX-1); // secs before vote menu comes up
VARG(votelimit, 0, 20, INT_MAX-1); // secs before vote passes by default
VARG(duellimit, 0, 5, INT_MAX-1); // secs before duel goes to next round
VARG(duelclear, 0, 0, 1); // clear items in duel

VARG(selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
VARG(teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
VARG(teambalance, 0, 2, 3); // 0 = off, 1 = by number, 2 = by rank, 3 = humans vs. ai

VARG(fraglimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stffinish, 0, 0, 1); // finish when all bases captured

VARG(spawnweapon, 0, WEAP_PISTOL, WEAP_TOTAL-1);
VARG(instaspawnweapon, 0, WEAP_RIFLE, WEAP_TOTAL-1);

VARG(spawndelay, 0, 3, INT_MAX-1); // delay before spawning in most modes (except non-dm and stf, ctf, duel, lms)
VARG(stfspawndelay, 0, 5, INT_MAX-1); // .. in stf
VARG(ctfspawndelay, 0, 5, INT_MAX-1); // .. in ctf
FVARG(instaspawnscale, 0, 0.5f, 1000); // scale the above values by this in instagib
FVARG(paintspawnscale, 0, 1.f, 1000); // scale the above values by this in paintball

FVARG(botscale, 0, 1.f, 1000);
VARG(botminskill, 1, 50, 101);
VARG(botmaxskill, 1, 75, 101);
VARG(botlimit, 0, 32, MAXBOTS);

FVARG(damagescale, 0, 1.f, 1000);
FVARG(gravityscale, 0, 1.f, 1000);
FVARG(jumpscale, 0, 1.f, 1000);
FVARG(speedscale, 1e-3f, 1.f, 1000);
FVARG(hitpushscale, 0, 1.f, 1000);
FVARG(deadpushscale, 0, 2.f, 1000);
FVARG(wavepushscale, 0, 1.f, 1000);

VARG(resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
VARG(resetvarsonend, 0, 1, 2); // reset variables on end (1: just when empty, 2: when matches end)
