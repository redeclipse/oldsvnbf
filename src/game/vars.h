VARG(serverdebug, 0, 0, 3);

SVARG(defaultmap, "");
VARG(defaultmode, -1, -1, G_MAX-1);
VARG(defaultmuts, -1, -1, G_M_ALL);
VARG(storyplayers, 1, 5, MAXPLAYERS);

SVARG(allowmaps, "bath bloodgrounds chaos citadel darkness deadsimple deathtrap deli dualstar dutility eight enigma firehouse forge gladiator hollow longestyard nova overseer panic refuge rivals siege smouldering testchamber tower tranquility venus warehouse warground wet wishbone");
SVARG(mainmaps, "bath bloodgrounds citadel darkness deadsimple deathtrap deli eight enigma gladiator longestyard nova panic refuge rivals smouldering tower warground venus");
SVARG(duelmaps, "bath citadel darkness deadsimple deathtrap dutility eight gladiator longestyard nova panic refuge rivals tower warground venus");
SVARG(ctfmaps, "bath citadel darkness deadsimple deli enigma forge gladiator nova panic refuge rivals smouldering warground venus");
SVARG(mctfmaps, "bloodgrounds deadsimple enigma refuge");
SVARG(stfmaps, "bath bloodgrounds citadel darkness deadsimple deli enigma forge gladiator nova panic refuge rivals smouldering tower warground venus");
SVARG(trialmaps, "testchamber");
SVARG(storymaps, "wishbone storytest");
VARG(maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random

VARG(maxcarry, 0, 1, WEAP_MAX-1);
VARG(spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random
VARG(spawnweapon, 0, WEAP_MELEE, WEAP_TOTAL-1);
VARG(instaweapon, 0, WEAP_INSTA, WEAP_TOTAL-1);
VARG(trialweapon, 0, WEAP_MELEE, WEAP_TOTAL-1);
VARG(spawngrenades, 0, 0, 2); // 0 = never, 1 = all but instagib/time-trial, 2 = always
VARG(spawndelay, 0, 5000, INT_MAX-1); // delay before spawning in most modes
VARG(instadelay, 0, 2500, INT_MAX-1); // .. in instagib/arena matches
VARG(trialdelay, 0, 1000, INT_MAX-1); // .. in time trial matches
VARG(spawnprotect, 0, 3000, INT_MAX-1); // delay before damage can be dealt to spawning player
VARG(instaprotect, 0, 1500, INT_MAX-1); // .. in instagib/arena matches

VARG(maxhealth, 0, 100, INT_MAX-1);
VARG(extrahealth, 0, 100, INT_MAX-1);

VARG(fireburntime, 0, 5500, INT_MAX-1);
VARG(fireburndelay, 0, 1000, INT_MAX-1);
VARG(fireburndamage, 0, 7, INT_MAX-1);

VARG(vampire, 0, 0, 1);
VARG(regendelay, 0, 3000, INT_MAX-1);
VARG(regenguard, 0, 1000, INT_MAX-1);
VARG(regentime, 0, 1000, INT_MAX-1);
VARG(regenhealth, 0, 5, INT_MAX-1);
VARG(regenextra, 0, 10, INT_MAX-1);
VARG(regenflag, 0, 1, 2); // 0 = off, 1 = only guarding, 2 = also while carrying

VARG(itemsallowed, 0, 2, 3); // 0 = never, 1 = all but instagib/time-trial, 2 = override insta if arena, 3 = always
VARG(itemdropping, 0, 1, 1); // 0 = never, 1 = yes
VARG(itemspawntime, 1, 30000, INT_MAX-1); // when items respawn
VARG(itemspawndelay, 0, 1000, INT_MAX-1); // after map start items first spawn
VARG(itemspawnstyle, 0, 1, 2); // 0 = all at once, 1 = staggered, 2 = random
VARG(kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always

VARG(timelimit, 0, 15, INT_MAX-1);
VARG(triallimit, 0, 60000, INT_MAX-1);
VARG(intermlimit, 0, 15000, INT_MAX-1); // .. before vote menu comes up
VARG(votelimit, 0, 30000, INT_MAX-1); // .. before vote passes by default
VARG(duelreset, 0, 1, 1); // reset winner in duel
VARG(duelclear, 0, 1, 1); // clear items in duel
VARG(duellimit, 0, 5000, INT_MAX-1); // .. before duel goes to next round

VARG(selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
VARG(trialdamage, 0, 0, 1); // 0 = off, 1 = allow damage in time-trial
VARG(teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
VARG(teambalance, 0, 1, 3); // 0 = off, 1 = by number then rank, 2 = by rank then number, 3 = humans vs. ai

VARG(fraglimit, 0, 0, INT_MAX-1); // finish when score is this or more

VARG(ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(ctfstyle, 0, 0, 3); // 0 = classic touch-and-return, 1 = grab and take home, 2 = defend and reset, 3 = dominate and protect
VARG(ctfresetdelay, 0, 30000, INT_MAX-1);

VARG(stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
VARG(stfstyle, 0, 1, 1); // 0 = overthrow and secure, 1 = instant secure
VARG(stffinish, 0, 0, 1); // finish when all bases captured
VARG(stfpoints, 0, 1, INT_MAX-1); // points added to score
VARG(stfoccupy, 0, 100, INT_MAX-1); // points needed to occupy

VARG(botbalance, -1, -1, MAXAI/2); // -1 = populate bots to map defined numplayers, 0 = don't balance, 1 or more = fill only with this*numteams
VARG(botminskill, 1, 70, 101);
VARG(botmaxskill, 1, 80, 101);
VARG(botlimit, 0, 16, MAXAI/2);

VARFG(gamespeed, 1, 100, 1000, timescale = sv_gamespeed, timescale = gamespeed);
VARFG(gamepaused, 0, 0, 1, paused = sv_gamepaused, paused = gamepaused);

FVARG(forcegravity, -1, -1, 10000);
FVARG(forcejumpspeed, -1, -1, 10000);
FVARG(forcemovespeed, -1, -1, 10000);
FVARG(forcemovecrawl, -1, -1, 10000);
FVARG(forceimpulsespeed, -1, -1, 10000);

VARG(forceimpulsestyle, -1, -1, 3);
VARG(forceimpulsemeter, -1, -1, INT_MAX-1);
VARG(forceimpulsecost, -1, -1, INT_MAX-1);
VARG(forceimpulsecount, -1, -1, INT_MAX-1);
VARG(forceimpulseskate, -1, -1, INT_MAX-1);
FVARG(forceimpulseregen, -1, -1, 10000);

FVARG(forceliquidspeed, -1, -1, 1);
FVARG(forceliquidcurb, -1, -1, 10000);
FVARG(forcefloorcurb, -1, -1, 10000);
FVARG(forceaircurb, -1, -1, 10000);

FVARG(damagescale, 0, 1, 1000);
FVARG(hitpushscale, 0, 1, 1000);
FVARG(deadpushscale, 0, 2, 1000);

FVARG(wavepusharea, 0, 2, 1000);
FVARG(wavepushscale, 0, 1, 1000);

VARG(multikilldelay, 0, 3000, INT_MAX-1);
VARG(spreecount, 0, 5, INT_MAX-1);
VARG(dominatecount, 0, 5, INT_MAX-1);

VARG(resetmmonend, 0, 2, 2); // reset mastermode on end (1: just when empty, 2: when matches end)
VARG(resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
VARG(resetvarsonend, 0, 2, 2); // reset variables on end (1: just when empty, 2: when matches end)
