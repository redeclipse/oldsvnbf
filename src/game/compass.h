#define NUMCOMPASS 8

FVAR(IDF_PERSIST, compasssize, 0, 0.15f, 1000);
VAR(IDF_PERSIST, compassfade, 0, 250, INT_MAX-1);
FVAR(IDF_PERSIST, compassfadeamt, 0, 0.75f, 1);
TVAR(IDF_PERSIST, compasstex, "textures/compass", 3);
TVAR(IDF_PERSIST, compassringtex, "textures/progress", 3);

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

void clearcmenu()
{
	compasspos = 0;
	if(compassmillis > 0) compassmillis = -lastmillis;
	if(curcompass)
	{
		curcompass->reset();
		curcompass = NULL;
	}
}
ICOMMAND(0, clearcompass, "", (), clearcmenu());

void resetcmenus()
{
	clearcmenu();
	loopvrev(cmenus) cmenus.remove(i);
	cmenus.setsize(0);
}
ICOMMAND(0, resetcompass, "", (), resetcmenus());

void addcmenu(const char *name, const char *contents)
{
	if(!name || !*name || !contents || !*contents) return;
	if(curcompass) clearcmenu();
	loopvrev(cmenus) if(!strcmp(name, cmenus[i].name)) cmenus.remove(i);
	cmenu &c = cmenus.add();
	c.name = newstring(name);
	c.contents = newstring(contents);
}
ICOMMAND(0, newcompass, "ss", (char *n, char *c), addcmenu(n, c));

void addaction(const char *name, const char *contents)
{
	if(!name || !*name || !curcompass) return;
	if(curcompass->actions.length() >= NUMCOMPASS) { conoutft(CON_DEBUG, "\frtoo many actions in compass: %s", curcompass->name); return; }
	caction &a = curcompass->actions.add();
	a.name = newstring(name);
	a.contents = newstring(contents && *contents ? contents : "");
}
ICOMMAND(0, compass, "ss", (char *n, char *c), addaction(n, c));

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
		return;
	}
	conoutft(CON_DEBUG, "\frno such compass menu: %s", name);
}
ICOMMAND(0, showcompass, "s", (char *n), showcmenu(n));

ICOMMAND(0, compassactive, "", (), result(curcompass ? curcompass->name : "0"));

const struct compassdirs
{
	int x, y, align;
} compassdir[NUMCOMPASS+1] = {
	{ 0, 0, TEXT_CENTERED }, // special cancel case
	{ 0, -1, TEXT_CENTERED },		{ 1, -1, TEXT_LEFT_JUSTIFY },
	{ 1, 0, TEXT_LEFT_JUSTIFY },	{ 1, 1, TEXT_LEFT_JUSTIFY  },
	{ 0, 1, TEXT_CENTERED },		{ -1, 1, TEXT_RIGHT_JUSTIFY  },
	{ -1, 0, TEXT_RIGHT_JUSTIFY },	{ -1, -1, TEXT_RIGHT_JUSTIFY  }
};

int cmenuhit()
{
	if(curcompass)
	{
		if(compasspos > 0) return compasspos;
		float cx = (cursorx-0.5f)*2.f, cy = (cursory-0.5f)*2.f;
		if(sqrtf(cx*cx+cy*cy) <= compasssize*0.5f) return 0;
		else
		{
			float yaw = -(float)atan2(cx, cy)/RAD+202.5f; if(yaw >= 360) yaw -= 360;
			loopv(curcompass->actions) if(yaw > i*45 && yaw <= (i+1)*45)
				return i+1;
		}
	}
	return 0;
}

void renderaction(int idx, int size, Texture *t, const char *name, bool hit)
{
	int x = hudwidth/2+(size*compassdir[idx].x), y = hudsize/2+(size*compassdir[idx].y),
		r = 255, g = hit ? 0 : 255, b = hit ? 0 : 255, f = hit ? 255 : 128;
	pushfont("default");
	if(!compassdir[idx].y) y -= compassdir[idx].x ? FONTH/2 : FONTH;
	else
	{
		if(compassdir[idx].y < 0) y -= FONTH*2;
		else if(compassdir[idx].y > 0) y += FONTH/3;
		if(compassdir[idx].x) { x -= compassdir[idx].x*size/2; y -= compassdir[idx].y*size/2; }
		else y -= compassdir[idx].y*FONTH/2;
	}
	if(t)
	{
		glBindTexture(GL_TEXTURE_2D, t->id);
		glColor4f(r/255.f, g/255.f, b/255.f, f/255.f);
		if(idx) drawslice(0.5f/NUMCOMPASS+(idx-2)/float(NUMCOMPASS), 1/float(NUMCOMPASS), hudwidth/2, hudsize/2, size);
		else drawsized(hudwidth/2-size*3/8, hudsize/2-size*3/8, size*3/4);
	}
	switch(compassdir[idx].y)
	{
		case -1:
		{
			pushfont("sub");
			y += draw_textx("%s", x, y, r, g, b, f, compassdir[idx].align, -1, -1, name);
			popfont();
			draw_textx("[%d]", x, y, r, g, b, 255, compassdir[idx].align, -1, -1, idx);
			break;
		}
		case 0:
		{
			if(compassdir[idx].x)
			{
				defformatstring(s)("[%d]", idx);
				draw_textx("%s", x, y, r, g, b, 255, compassdir[idx].align, -1, -1, s);
				x += (text_width(s)+FONTW/4)*compassdir[idx].x;
				y += FONTH/2;
				pushfont("sub");
				y -= FONTH/2;
				draw_textx("%s", x, y, r, g, b, f, compassdir[idx].align, -1, -1, name);
				popfont();
				break;
			} // center uses default case
		}
		case 1: default:
		{
			y += draw_textx("[%d]", x, y, r, g, b, idx ? 255 : f, compassdir[idx].align, -1, -1, idx);
			pushfont("sub");
			draw_textx("%s", x, y, r, g, b, f, compassdir[idx].align, -1, -1, name);
			popfont();
			break;
		}
	}
	popfont();
}

void rendercmenu()
{
	if(compassmillis <= 0 || !curcompass) return;
	int size = int(compasssize*hudsize), hit = cmenuhit();
	renderaction(0, size, *compassringtex ? textureload(compassringtex, 3) : NULL, "cancel", !hit);
	Texture *t = *compasstex ? textureload(compasstex, 3) : NULL;
	loopi(NUMCOMPASS)
	{
		if(!curcompass->actions.inrange(i)) break;
		renderaction(i+1, size, t, curcompass->actions[i].name, hit == i+1);
	}
}

void runcmenu(int idx)
{
	cmenu *oldcompass = curcompass;
	if(idx > 0 && curcompass && curcompass->actions.inrange(idx-1))
	{
		interactive = true;
		execute(curcompass->actions[idx-1].contents);
		interactive = false;
	}
	if(oldcompass == curcompass) clearcmenu();
}

bool keycmenu(int code, bool isdown, int cooked)
{
	switch(code)
	{
		case SDLK_RIGHT: case SDLK_UP: case SDLK_TAB: case -2: case -4:
			if(!isdown) { if(++compasspos > curcompass->actions.length()) compasspos = 0; } return true; break;
		case SDLK_LEFT: case SDLK_DOWN: case -5:
			if(!isdown) { if(--compasspos < 0) compasspos = NUMCOMPASS; } return true; break;
		case SDLK_RETURN: case -1:
			if(!isdown) { runcmenu(cmenuhit()); } return true; break;
		case SDLK_ESCAPE: case -3:
			if(!isdown) { clearcmenu(); } return true; break;
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
			if(!isdown) { runcmenu(code-SDLK_0); } return true; break;
		default: break;
	}
	return false;
}
