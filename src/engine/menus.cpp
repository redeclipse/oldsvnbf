// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "pch.h"
#include "engine.h"

#define GUI_TITLE_COLOR  0xFFDD88
#define GUI_BUTTON_COLOR 0xFFFFFF
#define GUI_TEXT_COLOR  0xDDFFDD

static vec menupos;
static int menustart = 0;
static int menutab = 1;
static g3d_gui *cgui = NULL;
static bool cguifirstpass;

struct menu : g3d_callback
{
    char *name, *header, *contents;

    menu() : name(NULL), header(NULL), contents(NULL) {}

    void gui(g3d_gui &g, bool firstpass)
    {
        cgui = &g;
        cguifirstpass = firstpass;
        cgui->start(menustart, 0.03f, &menutab);
        cgui->tab(header ? header : name, GUI_TITLE_COLOR);
        execute(contents);
        cgui->end();
        cgui = NULL;
    }

    virtual void clear() {}
};

static hashtable<const char *, menu> guis;
static vector<menu *> guistack;
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

void popgui()
{
    menu *m = guistack.pop();
    m->clear();
}

void pushgui(menu *m, int pos = -1)
{
    if(guistack.empty())
    {
        menupos = menuinfrontofplayer();
        g3d_resetcursor();
    }
    if(pos < 0) guistack.add(m);
    else guistack.insert(pos, m);
    if(pos < 0 || pos==guistack.length()-1)
    {
        menutab = 1;
        menustart = totalmillis;
    }
}

void restoregui(int pos)
{
    int clear = guistack.length()-pos-1;
    loopi(clear) popgui();
    menutab = 1;
    menustart = totalmillis;
}

void showgui(const char *name)
{
    menu *m = guis.access(name);
    if(!m) return;
    int pos = guistack.find(m);
    if(pos<0) pushgui(m);
    else restoregui(pos);
}

int cleargui(int n)
{
    int clear = guistack.length();
    if(n>0) clear = min(clear, n);
    loopi(clear) popgui();
    if(!guistack.empty()) restoregui(guistack.length()-1);
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

void newgui(char *name, char *contents, char *header)
{
    menu *m = guis.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &guis[name];
        m->name = name;
    }
    else
    {
        DELETEA(m->header);
        DELETEA(m->contents);
    }
    m->header = header && header[0] ? newstring(header) : NULL;
    m->contents = newstring(contents);
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

COMMAND(newgui, "sss");
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

static vector<const char *> needsapply;
static int changetypes = 0;

static struct applymenu : menu
{
    void gui(g3d_gui &g, bool firstpass)
    {
        if(guistack.empty()) return;
        g.start(menustart, 0.03f);
        g.text("the following settings have changed:", GUI_TEXT_COLOR, "info");
        loopv(needsapply) g.text(needsapply[i], GUI_TEXT_COLOR, "info");
        g.separator();
        g.text("apply changes now?", GUI_TEXT_COLOR, "info");
        if(g.button("yes", GUI_BUTTON_COLOR, "action")&G3D_UP)
        {
            if(changetypes&CHANGE_GFX) executelater.add(newstring("resetgl"));
            if(changetypes&CHANGE_SOUND) executelater.add(newstring("resetsound"));
            clearlater = true;
        }
        if(g.button("no", GUI_BUTTON_COLOR, "action")&G3D_UP)
            clearlater = true;
        g.end();
    }

    void clear()
    {
        needsapply.setsize(0);
        changetypes = 0;
    }
} applymenu;

VARP(applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i], desc)) return;
    needsapply.add(desc);
    changetypes |= type;
    if(needsapply.length() && guistack.find(&applymenu) < 0)
        pushgui(&applymenu, 0);
}

void menuprocess()
{
	int level = guistack.length();
	loopv(executelater) execute(executelater[i]);
	executelater.deletecontentsa();
	if(clearlater)
	{
        if(level==guistack.length()) loopi(level) popgui();
		clearlater = false;
	}
}

void g3d_mainmenu()
{
	if(!guistack.empty()) g3d_addgui(guistack.last());
}

bool menuactive() { return !guistack.empty(); };
