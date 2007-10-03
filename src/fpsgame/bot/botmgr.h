// Blood Frontier - BOT - Offline test and practice AI by Quinton Reeves
// Included by: fpsgame/bot/bot.h
// These are hooks for the console/script commands to manage the bots and their waypoints.

void start(const char *name)
{
	setvar("botnum", 0);
	setvar("botdrop", 0);
	avoid.setsize(0);
	waypoints = 0;

	loopv(cl.et.ents) if (cl.et.ents[i]->type == WAYPOINT) waypoints++;

	if (!waypoints && *name)
	{
		conoutf("WARNING: no waypoints found, generating them");
		setvar("botdrop", 1);
		wayspawn();
	}
	//add(-1, -1);
}

void players(int time)
{ 
	if (!cl.intermission)
	{
		avoid.setsize(0);
	
		wayposition(cl.player1);
	
		loopv(cl.players)
		{ 
			if (cl.players[i]) wayposition(cl.players[i]);
		}
	
		loopv(cl.ms.monsters)
		{ 
			if (cl.ms.monsters[i]) wayposition(cl.ms.monsters[i]);
		}

		loopv(cl.players) // state of other 'clients'
		{ 
			if (cl.players[i] && isbot(cl.players[i])) think(time, cl.players[i]);
		}
	
		loopv(cl.ms.monsters) // state of other 'clients'
		{ 
			if (cl.ms.monsters[i] && isbot(cl.ms.monsters[i])) think(time, cl.ms.monsters[i]);
		}
	}
	else if (botdrop) // make sure our new waypoints are saved!
	{
		setvar("botdrop", 0); // hacked
	}
}

void aimonster(fpsent *d, int rate, int state, int health)
{
	reset(d);

	d->botrate = rate;

	while (d->botrate > 100) d->botrate = d->botrate / 10;
	if (d->botrate < 1) d->botrate = 1;

	d->health = d->maxhealth = health;
	d->botflags = BOT_MONSTER;
	d->botstate = state;
	s_strcpy(d->team, "monster");
}
