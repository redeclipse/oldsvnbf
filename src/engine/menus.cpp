// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "pch.h"
#include "engine.h"

static vec menupos;
static int menustart = 0;
static int menutab = 1;
static g3d_gui *cgui = NULL;
static bool cguifirstpass;

static hashtable<const char *, char *> guis;
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
	dir.z -= camera1->height-1;
	return dir;
}

int cleargui(int n)
{
    int clear = guistack.length();
    if(n>0) clear = min(clear, n);
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
void guibutton(char *name, char *action, char *icon, char *altact)
{
	if(!cgui) return;
	int ret = cgui->button(name, GUI_BUTTON_COLOR, *icon ? icon : (strstr(action, "showgui") ? "menu" : "action"));
	if(ret&G3D_UP)
	{
		char *act = name;
		if (*altact && ret&G3D_ALTERNATE) act = altact;
		else if (*action) act = action;
		executelater.add(newstring(act));
        if(shouldclearmenu) clearlater = true;
	}
	else if(ret&G3D_ROLLOVER)
	{
		alias("guirollovername", name);
		alias("guirolloveraction", action);
	}
}

void guiimage(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact)
{
	if(!cgui) return;
    Texture *t = textureload(path, 0, true, false);
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        //if(t == notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0);
	if(ret&G3D_UP)
	{
		char *act = NULL;
		if (*altact && ret&G3D_ALTERNATE) act = altact;
		else if (*action) act = action;
		if (*act)
		{
			executelater.add(newstring(act));
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
	if(cgui) cgui->text(name, icon[0] ? GUI_BUTTON_COLOR : GUI_TEXT_COLOR, icon[0] ? icon : NULL);
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
    switch(id->type)
    {
        case ID_VAR:
        case ID_SVAR:
            s_sprintf(assign)("%s %d", var, val);
            break;
        case ID_ALIAS:
            s_sprintf(assign)("%s = %d", var, val);
            break;
        default:
            return;
    }
	executelater.add(newstring(assign));
	if(onchange[0]) executelater.add(newstring(onchange));
}

static int getval(char *var)
{
	ident *id = getident(var);
	if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_SVAR: return atoi(*id->storage.s);
        case ID_ALIAS: return atoi(id->action);
        default: return 0;
    }
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

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange, char *updateval)
{
	if(!cgui) return;
    const char *initval = "";
    if(!cguifirstpass && strcmp(g3d_fieldname(), var) && updateval[0]) execute(updateval);
	ident *id = getident(var);
    if(id && id->type==ID_ALIAS) initval = id->action;
	char *result = cgui->field(var, GUI_BUTTON_COLOR, (*maxlength!=0) ? *maxlength : 12, initval);
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

void showgui(const char *name)
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
COMMAND(guibutton, "ssss");
COMMAND(guitext, "ss");
COMMAND(guiservers, "s");
COMMANDN(cleargui, cleargui_, "i");
COMMAND(showgui, "s");
COMMAND(guistayopen, "s");

COMMAND(guilist, "s");
COMMAND(guititle, "s");
COMMAND(guibar,"");
COMMAND(guiimage,"ssfiss");
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

VARP(applydialog, 0, 1, 1);

static vector<const char *> needsapply;

void addchange(const char *desc)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i], desc)) return;
    needsapply.add(desc);
}

static vec applypos;
static int applystart = 0;

static struct applychangescallback : g3d_callback
{
    void gui(g3d_gui &g, bool firstpass)
    {
        g.start(applystart, 0.03f);
        g.text("the following settings have changed:", GUI_TEXT_COLOR, "info");
        loopv(needsapply) g.text(needsapply[i], GUI_TEXT_COLOR, "info");
        g.separator();
        g.text("apply changes now?", GUI_TEXT_COLOR, "info");
        if(g.button("yes", GUI_BUTTON_COLOR, "action")&G3D_UP)
        {
            applystart = 0;
            needsapply.setsize(0);
            executelater.add(newstring("resetgl"));
        }
        if(g.button("no", GUI_BUTTON_COLOR, "action")&G3D_UP)
        {
            applystart = 0;
            needsapply.setsize(0);
        }
        g.end();
    }
} accb;

void g3d_mainmenu()
{
	if(!guistack.empty())
	{
		g3d_addgui(&mmcb);
	}
    else if(needsapply.length())
    {
        if(!applystart)
        {
            applystart = totalmillis;
            applypos = menuinfrontofplayer();
            g3d_resetcursor();
        }
        g3d_addgui(&accb);
    }
}

bool menuactive() { return !guistack.empty(); };
