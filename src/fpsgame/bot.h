struct botclient
{
	GAMECLIENT &cl;
	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "", (botclient *self), self->addbot());
		CCOMMAND(delbot, "", (botclient *self), self->delbot());
	}

	void addbot()
	{
		cl.cc.addmsg(SV_ADDBOT, "r");
	}

	void delbot()
	{
		loopvrev(cl.players)
		{
			fpsent *d = cl.players[i];
			if (d && d->ownernum >= 0 && d->ownernum == cl.player1->clientnum)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				break;
			}
		}
	}

	void update()
	{
		loopv(cl.players)
		{
			fpsent *d = cl.players[i];
			if (d && d->ownernum >= 0 && d->ownernum == cl.player1->clientnum) think(d);
		}
	}

	void think(fpsent *d)
	{
		d->move = d->strafe = 0;
		d->lastupdate = lastmillis;
		cl.ph.smoothplayer(d, 10);
	}
} bot;
