// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "pch.h"
#include "engine.h"

static vec menupos;
static int menustart = 0;
static int menutab = 1;
static g3d_gui *cgui = NULL;
static bool cguifirstpass;

static hashtable<char *, char *> guis;
static vector<char *> guistack;
static vector<char *> executelater;
static bool shouldclearmenu = true, clearlater = false;

VARP(menudistance,  16, 40,  256);
VARP(menuautoclose, 32, 120, 4096);

vec menuinfrontofplayer()
{
	vec dir;
	vecfromyawpitch(camera1->yaw, 0, 1, 0, dir);
	dir.mul(menudistance).add(camera1->o);
	dir.z -= camera1->eyeheight-1;
	return dir;
}

int cleargui(int n)
{
	int m = guistack.length() - (cc->ready() ? 0 : 1), clear = n > 0 ? min(m, n) : m;
	loopi(clear) delete[] guistack.pop();
	if (!guistack.empty()) showgui(guistack.last());
	if (clear) cl->menuevent(MN_BACK);
	return clear;
}

void cleargui_(int *n)
{
	intret(cleargui(*n));
}

void guistayopen(char *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

#define GUI_TITLE_COLOR  0xFFDD88
#define GUI_BUTTON_COLOR 0xFFFFFF
#define GUI_TEXT_COLOR	0xDDFFDD

//@DOC name and icon are optional
void guibutton(char *name, char *action, char *icon)
{
	if(!cgui) return;
	int ret = cgui->button(name, GUI_BUTTON_COLOR, *icon ? icon : (strstr(action, "showgui") ? "menu" : "action"));
	if(ret&G3D_UP)
	{
		executelater.add(newstring(*action ? action : name));
        if(shouldclearmenu) clearlater = true;
	}
	else if(ret&G3D_ROLLOVER)
	{
		alias("guirollovername", name);
		alias("guirolloveraction", action);
	}
}

void guiimage(char *path, char *action, float *scale, int *overlaid)
{
	if(!cgui) return;
	int ret = cgui->image(path, *scale, *overlaid!=0);
	if(ret&G3D_UP)
	{
		if(*action)
		{
			executelater.add(newstring(action));
            if(shouldclearmenu) clearlater = true;
		}
	}
	else if(ret&G3D_ROLLOVER)
	{
		alias("guirolloverimgpath", path);
		alias("guirolloverimgaction", action);
	}
}

void guitext(char *name, char *icon)
{
	if(cgui) cgui->text(name, *icon ? GUI_BUTTON_COLOR : GUI_TEXT_COLOR, *icon ? icon : NULL);
}

void guititle(char *name)
{
	if(cgui) cgui->title(name, GUI_TITLE_COLOR);
}

void guitab(char *name)
{
	if(cgui) cgui->tab(name, GUI_TITLE_COLOR);
}

void guibar()
{
	if(cgui) cgui->separator();
}

static void updateval(char *var, int val, char *onchange)
{
	ident *id = getident(var);
	string assign;
	if(!id) return;
	else if(id->_type==ID_VAR) s_sprintf(assign)("%s %d", var, val);
	else if(id->_type==ID_ALIAS) s_sprintf(assign)("%s = %d", var, val);
	else return;
	executelater.add(newstring(assign));
	if(onchange[0]) executelater.add(newstring(onchange));
}

static int getval(char *var)
{
	ident *id = getident(var);
	if(!id) return 0;
	else if(id->_type==ID_VAR) return *id->_storage;
	else if(id->_type==ID_ALIAS) return atoi(id->_action);
	else return 0;
}

void guislider(char *var, int *min, int *max, char *onchange)
{
	if(!cgui) return;
	int oldval = getval(var), val = oldval, vmin = *max ? *min : getvarmin(var), vmax = *max ? *max : getvarmax(var);
	cgui->slider(val, vmin, vmax, GUI_TITLE_COLOR);
	if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange)
{
	if(!cgui) return;
	vector<int> vals;
	list += strspn(list, "\n\t ");
	while(*list)
	{
		vals.add(atoi(list));
		list += strcspn(list, "\n\t \0");
		list += strspn(list, "\n\t ");
	}
	if(vals.empty()) return;
	int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
	loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
	s_sprintfd(label)("%d", val);
	cgui->slider(offset, 0, vals.length()-1, GUI_TITLE_COLOR, label);
	if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guicheckbox(char *name, char *var, int *on, int *off, char *onchange)
{
	bool enabled = getval(var)!=*off;
	if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "checkbox_on" : "checkbox_off")&G3D_UP)
	{
		updateval(var, enabled ? *off : (*on || *off ? *on : 1), onchange);
	}
}

void guiradio(char *name, char *var, int *n, char *onchange)
{
	bool enabled = getval(var)==*n;
	if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "radio_on" : "radio_off")&G3D_UP)
	{
		if(!enabled) updateval(var, *n, onchange);
	}
}

void guifield(char *var, int *maxlength, char *onchange, char *updateval)
{
	if(!cgui) return;
	char *initval = "";
	if(!cguifirstpass && strcmp(g3d_fieldname(), var))
	{
		if(updateval[0]) execute(updateval);
		ident *id = getident(var);
		if(id && id->_type==ID_ALIAS) initval = id->_action;
	}
	char *result = cgui->field(var, GUI_BUTTON_COLOR, *maxlength>0 ? *maxlength : 12, initval);
	if(result)
	{
		alias(var, result);
		if(onchange[0]) execute(onchange);
	}
}

void guilist(char *contents)
{
	if(!cgui) return;
	cgui->pushlist();
	execute(contents);
	cgui->poplist();
}

void newgui(char *name, char *contents)
{
	if(guis.access(name))
	{
		delete[] guis[name];
		guis[name] = newstring(contents);
	}
	else guis[newstring(name)] = newstring(contents);
}

void showgui(char *name)
{
	int pos = guistack.find(name);
	if(pos<0)
	{
		if(!guis.access(name)) return;
		if(guistack.empty())
		{
			menupos = menuinfrontofplayer();
			g3d_resetcursor();
		}
		guistack.add(newstring(name));
	}
	else
	{
		pos = guistack.length()-pos-1;
		loopi(pos) delete[] guistack.pop();
	}
	menutab = 1;
	menustart = totalmillis;

	cl->menuevent(MN_INPUT);
}

void guiservers()
{
	extern const char *showservers(g3d_gui *cgui);
	if(cgui)
	{
		const char *name = showservers(cgui);
		if(name)
		{
			s_sprintfd(connect)("connect %s", name);
			executelater.add(newstring(connect));
            if(shouldclearmenu) clearlater = true;
		}
	}
}

COMMAND(newgui, "ss");
COMMAND(guibutton, "sss");
COMMAND(guitext, "ss");
COMMAND(guiservers, "s");
COMMANDN(cleargui, cleargui_, "i");
COMMAND(showgui, "s");
COMMAND(guistayopen, "s");

COMMAND(guilist, "s");
COMMAND(guititle, "s");
COMMAND(guibar,"");
COMMAND(guiimage,"ssfi");
COMMAND(guislider,"siis");
COMMAND(guilistslider, "sss");
COMMAND(guiradio,"ssis");
COMMAND(guicheckbox, "ssiis");
COMMAND(guitab, "s");
COMMAND(guifield, "siss");

static struct mainmenucallback : g3d_callback
{
	void gui(g3d_gui &g, bool firstpass)
	{
		if(guistack.empty()) return;
		char *name = guistack.last();
		char **contents = guis.access(name);
		if(!contents) return;
		cgui = &g;
		cguifirstpass = firstpass;
		cgui->start(menustart, 0.03f, &menutab);
		guitab(name);
		execute(*contents);
		cgui->end();
		cgui = NULL;
	}
} mmcb;

void menuprocess()
{
	int level = guistack.length();
	loopv(executelater) execute(executelater[i]);
	executelater.deletecontentsa();
	if(clearlater)
	{
		if (level == guistack.length()) guistack.deletecontentsa();
		clearlater = false;
	}
}

void g3d_mainmenu()
{
	if(!guistack.empty())
	{
		g3d_addgui(&mmcb, menupos, true);
	}
}

bool menuactive() { return !guistack.empty(); };
