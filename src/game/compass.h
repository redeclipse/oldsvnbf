#define NUMCOMPASS 8

VARP(compassstyle, 0, 1, 1);
FVARP(compasssize, 0, 0.15f, 1000);
VARP(compassfade, 0, 250, INT_MAX-1);
FVARP(compassfadeamt, 0, 0.75f, 1);

struct caction
{
	char *name, *contents;
	caction() {}
	~caction()
	{
		DELETEP(name);
		DELETEP(contents);
	}
};
struct cmenu : caction
{
	vector<caction> actions;
	cmenu() {}
	~cmenu() { reset(); }
	void reset()
	{
		loopvrev(actions) actions.remove(i);
		actions.setsize(0);
	}
};

int compassmillis = 0, compasspos = 0;
cmenu *curcompass = NULL;
vector<cmenu> cmenus;

void resetcmenus()
{
	if(compassmillis > 0) compassmillis = -lastmillis;
	compasspos = 0;
	curcompass = NULL;
	loopvrev(cmenus) cmenus.remove(i);
	cmenus.setsize(0);
}
ICOMMAND(resetcompass, "", (), resetcmenus());

void addcmenu(const char *name, const char *contents)
{
	if(!name || !*name || !contents || !*contents) return;
	loopvrev(cmenus) if(!strcmp(name, cmenus[i].name)) cmenus.remove(i);
	cmenu &c = cmenus.add();
	c.name = newstring(name);
	c.contents = newstring(contents);
}
ICOMMAND(newcompass, "ss", (char *n, char *c), addcmenu(n, c));

void addaction(const char *name, const char *contents)
{
	if(!name || !*name || !curcompass) return;
	if(curcompass->actions.length() >= NUMCOMPASS) { conoutft(CON_DEBUG, "\frtoo many actions in compass: %s", curcompass->name); return; }
	caction &a = curcompass->actions.add();
	a.name = newstring(name);
	a.contents = newstring(contents && *contents ? contents : "");
}
ICOMMAND(compass, "ss", (char *n, char *c), addaction(n, c));

void showcmenu(const char *name)
{
	if(!name || !*name) return;
	loopv(cmenus) if(!strcmp(name, cmenus[i].name))
	{
		if(compassmillis <= 0) compassmillis = lastmillis;
		curcompass = &cmenus[i];
		compasspos = 0;
		curcompass->reset();
		execute(curcompass->contents);
		resetcursor();
		return;
	}
	conoutft(CON_DEBUG, "\frno such compass menu: %s", name);
}
ICOMMAND(showcompass, "s", (char *n), showcmenu(n));

void clearcmenu()
{
	if(!curcompass) return;
	if(compassmillis > 0) compassmillis = -lastmillis;
	curcompass->reset();
	curcompass = NULL;
	resetcursor();
}
ICOMMAND(clearcompass, "", (), clearcmenu());
ICOMMAND(compassactive, "", (), result(curcompass ? curcompass->name : "0"));

const struct compassdirs
{
	int x, y, align;
} compassdir[NUMCOMPASS+1] = {
	{ 0, 0, TEXT_CENTERED }, // special cancel case
	{ 0, -1, TEXT_CENTERED }, { 1, -1, TEXT_LEFT_JUSTIFY }, { 1, 0, TEXT_LEFT_JUSTIFY }, { 1, 1, TEXT_LEFT_JUSTIFY },
	{ 0, 1, TEXT_CENTERED }, { -1, 1, TEXT_RIGHT_JUSTIFY }, { -1, 0, TEXT_RIGHT_JUSTIFY }, { -1, -1, TEXT_RIGHT_JUSTIFY }
};

void cmenudims(int idx, int &x, int &y, int size, bool layout = false)
{
	x = hudwidth/2+(size*compassdir[idx].x);
	y = hudsize/2+(size*compassdir[idx].y);
	if(!compassdir[idx].y) { if(!layout) y -= FONTH; }
	else
	{
		if(!layout && compassdir[idx].y < 0) y -= FONTH*2;
		switch(compassstyle)
		{
			case 0:
			{
				if(compassdir[idx].x) x -= compassdir[idx].x*size/2;
				else y += compassdir[idx].y*FONTH*4;
				y -= compassdir[idx].y*size/2;
				break;
			}
			case 1: default:
			{
				if(compassdir[idx].x)
				{
					x -= compassdir[idx].x*size/2;
					y -= compassdir[idx].y*size/2;
				}
				break;
			}
		}
	}
}

int cmenuhit()
{
	if(compasspos > 0) return compasspos;
	int x = 0, y = 0, cx = int(hudwidth*cursorx), cy = int(hudsize*cursory), size = int(compasssize*hudsize);
	loopi(NUMCOMPASS)
	{
		cmenudims(i+1, x, y, size, true);
		if((!compassdir[i+1].x || (compassdir[i+1].x > 0 && cx >= x) || (compassdir[i+1].x < 0 && cx <= x)) && (!compassdir[i+1].y || (compassdir[i+1].y > 0 && cy >= y) || (compassdir[i+1].y < 0 && cy <= y)))
			return i+1;
	}
	return 0;
}

void renderaction(int idx, int size, const char *name, bool hit)
{
	int x = 0, y = 0; cmenudims(idx, x, y, size);
	pushfont(idx ? "default" : "sub");
	y += draw_textx("[%d]", x, y, 255, (hit ? 0 : 255), (hit ? 0 : 255), (hit ? 255 : 128), compassdir[idx].align, -1, -1, idx);
	popfont();
	pushfont(idx ? "sub" : "radar");
	draw_textx("%s", x, y, 200, (hit ? 0 : 200), (hit ? 0 : 200), (hit ? 255 : 128), compassdir[idx].align, -1, -1, name);
	popfont();
}

void rendercmenu()
{
	if(compassmillis <= 0 || !curcompass) return;
	int size = int(compasssize*hudsize);
	int hit = cmenuhit();
	renderaction(0, size, "cancel", !hit);
	loopi(NUMCOMPASS)
	{
		if(!curcompass->actions.inrange(i)) break;
		renderaction(i+1, size, curcompass->actions[i].name, hit == i+1);
	}
}

void runcmenu(int idx)
{
	if(idx > 0 && curcompass && curcompass->actions.inrange(idx-1)) execute(curcompass->actions[idx-1].contents);
	clearcmenu();
}

bool keypress(int code, bool isdown, int cooked)
{
	if(curcompass)
	{
		switch(code)
		{ // fall-through-o-rama
			case SDLK_RIGHT: case SDLK_UP: case SDLK_TAB: case -2: case -4: if(!isdown) { if(++compasspos > NUMCOMPASS) compasspos = 0; } return true; break;
			case SDLK_LEFT: case SDLK_DOWN: case -5: if(!isdown) { if(--compasspos < 0) compasspos = NUMCOMPASS; } return true; break;
			case SDLK_RETURN: case -1: if(!isdown) { runcmenu(cmenuhit()); } return true; break;
			case SDLK_ESCAPE: case -3: if(!isdown) { clearcmenu(); } return true; break;
			case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
				if(!isdown) { runcmenu(code-SDLK_0); } return true; break;
			default: break;
		}
	}
	return false;
}
