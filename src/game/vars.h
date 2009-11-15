VARG(serverdebug, 0, 0, 3);

SVARG(defaultmap, "");
VARG(defaultmode, G_LOBBY, G_CTF, G_MAX-1);
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);
VARG(storyplayers, 1, 5, MAXPLAYERS);

SVARG(storymaps, "wishbone");
SVARG(mainmaps, "bath bloodgrounds citadel deadsimple deli eight enigma firehouse forge gladiator longestyard nova panic refuge rivals smouldering tower warground venus");
SVARG(ctfmaps, "bath bloodgrounds citadel deadsimple deli enigma forge gladiator longestyard nova panic refuge rivals smouldering tower warground venus");
SVARG(mctfmaps, "bloodgrounds deadsimple enigma refuge");
SVARG(stfmaps, "bath bloodgrounds citadel deadsimple deli enigma forge gladiator longestyard nova panic refuge rivals smouldering tower warground venus");
SVARG(racemaps, "testchamber");
VARG(maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random

VARG(maxcarry, 0, 2, WEAP_MAX-1);
VARG(spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random
VARG(spawnweapon, 0, WEAP_PISTOL, WEAP_TOTAL-1);
VARG(instaspawnweapon, 0, WEAP_INSTA, WEAP_TOTAL-1);
VARG(spawngrenades, 0, 0, 2); // 0 = never, 1 = all but instagib, 2 = always
VARG(spawndelay, 0, 5000, INT_MAX-1); // delay before spawning in most modes
VARG(instaspawndelay, 0, 3000, INT_MAX-1); // .. in instagib matches
VARG(racespawndelay, 0, 1000, INT_MAX-1); // .. in race matches
VARG(spawnprotecttime, 0, 3000, INT_MAX-1); // delay before damage can be dealt to spawning player

VARG(maxhealth, 0, 100, INT_MAX-1);
VARG(extrahealth, 0, 100, INT_MAX-1);

VARG(fireburntime, 0, 5500, INT_MAX-1);
VARG(fireburndelay, 0, 1000, INT_MAX-1);
VARG(fireburndamage, 0, 5, INT_MAX-1);

VARG(vampire, 0, 0, 1);
VARG(regendelay, 0, 3000, INT_MAX-1);
VARG(regenguard, 0, 1000, INT_MAX-1);
VARG(regentime, 0, 1000, INT_MAX-1);
VARG(regenhealth, 0, 5, INT_MAX-1);
VARG(regenextra, 0, 10, INT_MAX-1);
VARG(regenflag, 0, 1, 2); // 0 = off, 1 = only guarding, 2 = also while carrying

VARG(impulsemeter, 0, 20000, INT_MAX-1); // impulse dash length
VARG(impulsecost, 0, 1000, INT_MAX-1); // cost of impulse jump
VARG(impulsecount, 0, 5, INT_MAX-1); // number of impulse actions per air transit
VARG(impulseskate, 0, 1000, INT_MAX-1); // length of time a run along a wall can last

VARG(itemsallowed, 0, 1, 2); // 0 = never, 1 = all but instagib, 2 = always
VARG(itemdropping, 0, 1, 1); // 0 = never, 1 = yes
VARG(itemspawntime, 1, 30000, INT_MAX-1); // when items respawn
VARG(itemspawndelay, 0, 1000, INT_MAX-1); // after map start items first spawn
VARG(itemspawnstyle, 0, 1, 2); // 0 = all at once, 1 = staggered, 2 = random
VARG(kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always

VARG(timelimit, 0, 15, INT_MAX-1);
VARG(racelimit, 0, 60000, INT_MAX-1);
VARG(intermlimit, 0, 15000, INT_MAX-1); // .. before vote menu comes up
VARG(votelimit, 0, 30000, INT_MAX-1); // .. before vote passes by default
VARG(duellimit, 0, 3000, INT_MAX-1); // .. before duel goes to next round
VARG(duelclear, 0, 0, 1); // clear items in duel

VARG(selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
VARG(teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
VARG(teambalance, 0, 1, 3); // 0 = off, 1 = by number then rank, 2 = by rank then number, 3 = humans vs. ai

VARG(fraglimit, 0, 0, INT_MAX-1); // finish when score is this or more

VARG(ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(ctfstyle, 0, 0, 3); // 0 = classic touch-and-return, 1 = grab and take home, 2 = defend and reset, 3 = dominate and protect
VARG(ctfresetdelay, 0, 30000, INT_MAX-1);

VARG(stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stffinish, 0, 0, 1); // finish when all bases captured
VARG(stfpoints, 0, 1, INT_MAX-1); // points added to score
VARG(stfoccupy, 0, 100, INT_MAX-1); // points needed to occupy

#ifdef STANDALONE
VARG(botbalance, 0, 1, 2); // 0 = turn off bots, 1 = fill only with enough bots to satisfy play, 2 = populate bots to numplayers
#else
VARG(botbalance, 0, 2, 2);
#endif
FVARG(botscale, 0, 1.f, 1000);
VARG(botminskill, 1, 50, 101);
VARG(botmaxskill, 1, 75, 101);
VARG(botlimit, 0, 16, MAXAI/2);

FVARG(forcegravity, 0, 0, 10000);
FVARG(forcemovespeed, 0, 0, 10000);
FVARG(forcemovecrawl, 0, 0, 10000);
FVARG(forcejumpspeed, 0, 0, 10000);
FVARG(forceimpulsespeed, 0, 0, 10000);

FVARG(damagescale, 0, 1.f, 1000);
FVARG(speedscale, 1e-3f, 1.f, 1000);
FVARG(hitpushscale, 0, 1.f, 1000);
FVARG(deadpushscale, 0, 2.f, 1000);

FVARG(wavepusharea, 0, 3.f, 1000);
FVARG(wavepushscale, 0, 1.f, 1000);

VARG(multikilldelay, 0, 3000, INT_MAX-1);
VARG(spreecount, 0, 5, INT_MAX-1);
VARG(dominatecount, 0, 5, INT_MAX-1);

VARG(resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
VARG(resetvarsonend, 0, 1, 2); // reset variables on end (1: just when empty, 2: when matches end)
