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
	
		loopv(cl.players) // state of other 'clients'
		{ 
			if (cl.players[i] && isbot(cl.players[i])) think(time, cl.players[i]);
		}

	}
	else if (botdrop) // make sure our new waypoints are saved!
	{
		setvar("botdrop", 0); // hacked
	}
}
