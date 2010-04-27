GVAR(IDF_ADMIN, serverdebug, 0, 0, 3);
GVAR(IDF_ADMIN, serverclients, 1, 16, MAXCLIENTS);
GVAR(IDF_ADMIN, serveropen, 0, 3, 3);
GSVAR(IDF_ADMIN, serverdesc, "");
GSVAR(IDF_ADMIN, servermotd, "");
GVAR(IDF_ADMIN, automaster, 0, 0, 1);

GVAR(IDF_ADMIN, modelimit, 0, G_EDITMODE, G_MAX-1);
GVAR(IDF_ADMIN, mutslimit, 0, G_M_ALL, G_M_ALL);
GVAR(IDF_ADMIN, modelock, 0, 3, 5); // 0 = off, 1 = master only (+1 admin only), 3 = master can only set limited mode and higher (+1 admin), 5 = no mode selection
GVAR(IDF_ADMIN, mapslock, 0, 1, 5); // 0 = off, 1 = master can select non-allow maps (+1 admin), 3 = master can select non-rotation maps (+1 admin), 5 = no map selection
GVAR(IDF_ADMIN, varslock, 0, 0, 2); // 0 = master, 1 = admin only, 2 = nobody
GVAR(IDF_ADMIN, votelock, 0, 1, 5); // 0 = off, 1 = master can select same game (+1 admin), 3 = master only can vote (+1 admin), 5 = no voting
GVAR(IDF_ADMIN, votewait, 0, 3000, INT_MAX-1);

GVAR(IDF_ADMIN, resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetvarsonend, 0, 2, 2); // reset variables on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetmmonend, 0, 2, 2); // reset mastermode on end (1: just when empty, 2: when matches end)

GVARF(IDF_ADMIN, gamespeed, 1, 100, 1000, timescale = sv_gamespeed, timescale = gamespeed);
GVARF(IDF_ADMIN, gamepaused, 0, 0, 1, paused = sv_gamepaused, paused = gamepaused);

GSVAR(IDF_ADMIN, defaultmap, "");
GVAR(IDF_ADMIN, defaultmode, -1, G_DEATHMATCH, G_MAX-1);
GVAR(IDF_ADMIN, defaultmuts, -2, G_M_TEAM, G_M_ALL);
GVAR(IDF_ADMIN, campaignplayers, 1, 4, MAXPLAYERS);

GSVAR(IDF_ADMIN, allowmaps, "alphacampain bath bloodgrounds campaigntest chaos citadel darkness deadsimple deathtrap deli depot dutility eight enigma firehouse forge ghost gladiator hollow longestyard mist nova oasis overseer panic refuge rivals siege smouldering stone testchamber tower tranquility venus warground warp wishbone");
GSVAR(IDF_ADMIN, mainmaps, "bath bloodgrounds citadel darkness deadsimple deathtrap deli depot eight enigma ghost gladiator longestyard mist nova panic refuge rivals smouldering stone tower tranquility warground warp venus");
GSVAR(IDF_ADMIN, duelmaps, "bath citadel darkness deadsimple deathtrap dutility eight ghost gladiator longestyard mist nova panic refuge rivals tower tranquility warground venus");
GSVAR(IDF_ADMIN, ctfmaps, "bath citadel darkness deadsimple deli depot enigma forge ghost gladiator mist nova panic refuge rivals smouldering stone tranquility warground warp venus");
GSVAR(IDF_ADMIN, stfmaps, "bath bloodgrounds citadel darkness deadsimple deli depot enigma forge ghost gladiator mist nova panic refuge rivals smouldering stone tower tranquility warground warp venus");
GSVAR(IDF_ADMIN, trialmaps, "testchamber");
GSVAR(IDF_ADMIN, campaignmaps, "alphacampain wishbone campaigntest");

GICOMMAND(0, resetvars, "", (), server::resetgamevars(true), return);
GICOMMAND(IDF_ADMIN, resetconfig, "", (), rehash(true), );

GVAR(0, maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random

GVAR(0, maxcarry, 1, 2, INT_MAX-1);
GVAR(0, spawnrotate, 0, 4, INT_MAX-1); // 0 = let client decide, 1 = sequence, 2+ = random
GVAR(0, spawnweapon, 0, WEAP_PISTOL, WEAP_MAX-1);
GVAR(0, instaweapon, 0, WEAP_RIFLE, WEAP_MAX-1);
GVAR(0, limitedweapon, 0, WEAP_MELEE, WEAP_MAX-1);
GVAR(0, spawngrenades, 0, 0, 2); // 0 = never, 1 = all but instagib/time-trial, 2 = always
GVAR(0, spawndelay, 0, 3000, INT_MAX-1); // delay before spawning in most modes
GVAR(0, instadelay, 0, 1500, INT_MAX-1); // .. in instagib/arena matches
GVAR(0, trialdelay, 0, 500, INT_MAX-1); // .. in time trial matches
GVAR(0, spawnprotect, 0, 3000, INT_MAX-1); // delay before damage can be dealt to spawning player
GVAR(0, instaprotect, 0, 1500, INT_MAX-1); // .. in instagib/arena matches

GVAR(0, maxhealth, 0, 100, INT_MAX-1);
GVAR(0, extrahealth, 0, 100, INT_MAX-1);

GVAR(0, fireburntime, 0, 5500, INT_MAX-1);
GVAR(0, fireburndelay, 0, 1000, INT_MAX-1);
GVAR(0, fireburndamage, 0, 5, INT_MAX-1);

GVAR(0, vampire, 0, 0, 1);
GVAR(0, regendelay, 0, 3000, INT_MAX-1);
GVAR(0, regenguard, 0, 1000, INT_MAX-1);
GVAR(0, regentime, 0, 1000, INT_MAX-1);
GVAR(0, regenhealth, 0, 5, INT_MAX-1);
GVAR(0, regenextra, 0, 10, INT_MAX-1);
GVAR(0, regenflag, 0, 1, 2); // 0 = off, 1 = only guarding, 2 = also while carrying

GVAR(0, itemsallowed, 0, 2, 2); // 0 = never, 1 = all but instagib/time-trial, 2 = always
GVAR(0, itemdropping, 0, 1, 1); // 0 = never, 1 = yes
GVAR(0, itemspawntime, 1, 15000, INT_MAX-1); // when items respawn
GVAR(0, itemspawndelay, 0, 1000, INT_MAX-1); // after map start items first spawn
GVAR(0, itemspawnstyle, 0, 1, 2); // 0 = all at once, 1 = staggered, 2 = random
GFVAR(0, itemthreshold, 0, 1, 1000); // if numitems/numclients/maxcarry is less than this, spawn one of this type
GVAR(0, kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always

GVAR(0, timelimit, 0, 15, INT_MAX-1);
GVAR(0, triallimit, 0, 60000, INT_MAX-1);
GVAR(0, intermlimit, 0, 15000, INT_MAX-1); // .. before vote menu comes up
GVAR(0, votelimit, 0, 45000, INT_MAX-1); // .. before vote passes by default
GVAR(0, duelreset, 0, 1, 1); // reset winner in duel
GVAR(0, duelclear, 0, 1, 1); // clear items in duel
GVAR(0, duellimit, 0, 5000, INT_MAX-1); // .. before duel goes to next round

GVAR(0, selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
GVAR(0, trialdamage, 0, 0, 1); // 0 = off, 1 = allow damage in time-trial
GVAR(0, teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
GVAR(0, teambalance, 0, 1, 3); // 0 = off, 1 = by number then rank, 2 = by rank then number, 3 = humans vs. ai
GVAR(0, fraglimit, 0, 0, INT_MAX-1); // finish when score is this or more

GVAR(0, ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
GVAR(0, ctfstyle, 0, 0, 3); // 0 = classic touch-and-return, 1 = grab and take home, 2 = defend and reset, 3 = dominate and protect
GVAR(0, ctfresetdelay, 0, 30000, INT_MAX-1);

GVAR(0, stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
GVAR(0, stfstyle, 0, 1, 1); // 0 = overthrow and secure, 1 = instant secure
GVAR(0, stffinish, 0, 0, 1); // finish when all bases captured
GVAR(0, stfpoints, 0, 1, INT_MAX-1); // points added to score
GVAR(0, stfoccupy, 0, 100, INT_MAX-1); // points needed to occupy
GVAR(0, stfflags, 0, 3, 3); // 0 = init all (neutral), 1 = init neutral and team only, 2 = init team only, 3 = init all (team + neutral + converted)

GVAR(0, skillmin, 1, 65, 101);
GVAR(0, skillmax, 1, 85, 101);
GVAR(0, botbalance, -1, -1, INT_MAX-1); // -1 = populate bots to map defined numplayers, 0 = don't balance, 1 or more = fill only with this*numteams
GVAR(0, botlimit, 0, 16, INT_MAX-1);
GVAR(0, enemyallowed, 0, 1, 2); // 0 = off (campaign only), 1 = deathmatch, 2 = instagib too
GVAR(0, enemybalance, 0, 1, INT_MAX-1);
GVAR(0, enemydelay, 0, 15000, INT_MAX-1);

GFVAR(0, forcegravity, -1, -1, 1000);
GFVAR(0, forceliquidspeed, -1, -1, 1);
GFVAR(0, forceliquidcurb, -1, -1, 1000);
GFVAR(0, forcefloorcurb, -1, -1, 1000);
GFVAR(0, forceaircurb, -1, -1, 1000);

GFVAR(0, jumpspeed, 0, 100.f, 1000); // extra velocity to add when jumping
GFVAR(0, movespeed, 0, 100.f, 1000); // speed
GFVAR(0, movecrawl, 0, 0.5f, 1000); // crawl modifier
GFVAR(0, impulsespeed, 0, 100.f, 1000); // extra velocity to add when impulsing

GVAR(0, impulseallowed, 0, 3, 3); // impulse allowed; 0 = off, 1 = dash/boost only, 2 = dash/boost and sprint, 3 = all mechanics including parkour
GVAR(0, impulsestyle, 0, 1, 3); // impulse style; 0 = off, 1 = touch and count, 2 = count only, 3 = freestyle
GVAR(0, impulsecount, 0, 10, INT_MAX-1); // number of impulse actions per air transit
GVAR(0, impulsemeter, 0, 0, INT_MAX-1); // impulse dash length; 0 = unlimited, anything else = timer
GVAR(0, impulsecost, 0, 500, INT_MAX-1); // cost of impulse jump
GVAR(0, impulseskate, 0, 1000, INT_MAX-1); // length of time a run along a wall can last
GFVAR(0, impulseregen, 0, 10, 1000); // impulse regen multiplier

GFVAR(0, stillspread, 0, 2.5f, 1000);
GFVAR(0, movespread, 0, 5, 1000);
GFVAR(0, jumpspread, 0, 10, 1000);
GFVAR(0, impulsespread, 0, 25, 1000);

GFVAR(0, explodescale, 0, 1, 1000);
GFVAR(0, limitedscale, 0, 0.5f, 1000);
GFVAR(0, damagescale, 0, 1, 1000);
GVAR(0, damagecritchance, 0, 100, INT_MAX-1);

GFVAR(0, hitpushscale, 0, 1, 1000);
GFVAR(0, kickpushscale, 0, 1, 1000);
GFVAR(0, kickpushcrouch, 0, 0.5f, 1000);
GFVAR(0, kickpushsway, 0, 0.25f, 1000);
GFVAR(0, kickpushzoom, 0, 0.125f, 1000);
GFVAR(0, deadpushscale, 0, 2, 1000);
GFVAR(0, wavepushscale, 0, 1, 1000);

GVAR(0, multikilldelay, 0, 5000, INT_MAX-1);
GVAR(0, spreecount, 0, 5, INT_MAX-1);
GVAR(0, dominatecount, 0, 5, INT_MAX-1);

GVAR(0, alloweastereggs, 0, 1, 1); // 0 = off, 1 = on
GVAR(0, returningfire, 0, 0, 1); // 0 = off, 1 = on
