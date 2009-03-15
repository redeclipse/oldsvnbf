// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "cube.h"
#include "engine.h"

int cmenustart = 0, cmenutab = 1;
g3d_gui *cgui = NULL;
menu *cmenu = NULL;
hashtable<const char *, menu> guis;
vector<menu *> guistack;
vector<char *> executelater;
bool shouldclearmenu = true, clearlater = false;

void popgui()
{
    menu *m = guistack.pop();
    m->passes = 0;
    m->clear();
}

void removegui(menu *m)
{
    loopv(guistack) if(guistack[i]==m)
    {
        guistack.remove(i);
		m->passes = 0;
        m->clear();
        return;
    }
}

void pushgui(menu *m, int pos = -1)
{
    if(guistack.empty()) resetcursor();
    if(pos < 0) guistack.add(m);
    else guistack.insert(pos, m);
    if(pos < 0 || pos==guistack.length()-1)
    {
        cmenutab = 1;
        cmenustart = totalmillis;
    }
	if(m) m->passes = 0;
}

void restoregui(int pos)
{
    int clear = guistack.length()-pos-1;
    loopi(clear) popgui();
    cmenutab = 1;
    cmenustart = totalmillis;
	menu *m = guistack.last();
	if(m) m->passes = 0;
}

void showgui(const char *name)
{
    menu *m = guis.access(name);
    if(!m) return;
    int pos = guistack.find(m);
    if(pos<0) pushgui(m);
    else restoregui(pos);
	playsound(S_GUIPRESS, camera1->o, camera1, SND_FORCED);
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

void guinoautotab(char *contents)
{
    if(!cgui) return;
    cgui->allowautotab(false);
    execute(contents);
    cgui->allowautotab(true);
}

//@DOC name and icon are optional
SVAR(guirollovername, "");
SVAR(guirolloveraction, "");

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
		setsvar("guirollovername", name, true);
		setsvar("guirolloveraction", action, true);
	}
}

SVAR(guirolloverimgpath, "");
SVAR(guirolloverimgaction, "");

void guiimage(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact)
{
	if(!cgui) return;
    Texture *t = textureload(path, 0, true, false);
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
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
		setsvar("guirolloverimgpath", path, true);
		setsvar("guirolloverimgaction", action, true);
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

void guistrut(int *strut, int *alt)
{
	if(cgui)
	{
		if(!*alt) cgui->pushlist();
		cgui->strut(*strut);
		if(!*alt) cgui->poplist();
	}
}

static void updateval(char *var, int val, char *onchange)
{
	ident *id = getident(var);
	string assign;
	if(!id) return;
    switch(id->type)
    {
        case ID_VAR:
        case ID_FVAR:
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
        case ID_FVAR: return int(*id->storage.f);
        case ID_SVAR: return atoi(*id->storage.s);
        case ID_ALIAS: return atoi(id->action);
        default: return 0;
    }
}

static int getvardef(char *var)
{
	ident *id = getident(var);
	if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return id->def.i;
        case ID_FVAR: return int(id->def.f);
        case ID_SVAR: return atoi(id->def.s);
        case ID_ALIAS: return atoi(id->action);
        default: return 0;
    }
}

void guislider(char *var, int *min, int *max, char *onchange)
{
	if(!cgui) return;
	int oldval = getval(var), val = oldval, vmin = *max ? *min : getvarmin(var), vmax = *max ? *max : getvarmax(var);
	if(vmax >= INT_MAX-1)
	{ // not a sane value for a slider..
		int vdef = getvardef(var);
		vmax = vdef > vmin ? vdef*3 : vmin*4;
	}
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
	if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "checkboxon" : "checkbox")&G3D_UP)
	{
		updateval(var, enabled ? *off : (*on || *off ? *on : 1), onchange);
	}
}

void guiradio(char *name, char *var, int *n, char *onchange)
{
	bool enabled = getval(var)==*n;
	if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "radioboxon" : "radiobox")&G3D_UP)
	{
		if(!enabled) updateval(var, *n, onchange);
	}
}

void guibitfield(char *name, char *var, int *mask, char *onchange)
{
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "checkboxon" : "checkbox")&G3D_UP)
    {
        updateval(var, enabled ? val & ~*mask : val | *mask, onchange);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange)
{
	if(!cgui) return;
    const char *initval = "";
	ident *id = getident(var);
    if(id && id->type==ID_ALIAS) initval = id->action;
	char *result = cgui->field(var, GUI_BUTTON_COLOR, *maxlength ? *maxlength : 12, 0, initval);
	if(result)
	{
		alias(var, result);
		if(onchange[0]) execute(onchange);
	}
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode)
{
    if(!cgui) return;
    cgui->field(name, GUI_BUTTON_COLOR, *maxlength ? *maxlength : 12, *height, NULL, *mode<=0 ? EDITORFOREVER : *mode);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange)
{
    if(!cgui) return;
    const char *initval = "";
    ident *id = getident(var);
    if(id && id->type==ID_ALIAS) initval = id->action;
    char *result = cgui->keyfield(var, GUI_BUTTON_COLOR, *maxlength ? *maxlength : -8, 0, initval);
    if(result)
    {
        alias(var, result);
        if(onchange[0]) execute(onchange);
    }
}

//use text<action> to do more...


void guilist(char *contents)
{
	if(!cgui) return;
	cgui->pushlist();
	execute(contents);
	cgui->poplist();
}

void newgui(char *name, char *contents, char *initscript, char *header)
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
        DELETEA(m->initscript);
    }
    m->contents = contents && contents[0] ? newstring(contents) : NULL;
    m->initscript = initscript && initscript[0] ? newstring(initscript) : NULL;
    m->header = header && header[0] ? newstring(header) : NULL;
}

void guimodify(char *name, char *contents)
{
    menu *m = guis.access(name);
    if(!m) return;
    if(m->contents) delete[] m->contents;
	m->contents = contents && contents[0] ? newstring(contents) : NULL;
}

void guiservers()
{
	if(cgui)
	{
		extern int showservers(g3d_gui *cgui);
		int n = showservers(cgui);
		if(n >= 0 && servers.inrange(n))
		{
			s_sprintfd(c)("connect %s %d %d", servers[n]->name, servers[n]->port, servers[n]->qport);
			executelater.add(newstring(c));
            if(shouldclearmenu) clearlater = true;
		}
	}
}

COMMAND(newgui, "ssss");
COMMAND(guimodify, "ss");
COMMAND(guibutton, "ssss");
COMMAND(guitext, "ss");
COMMAND(guiservers, "");
COMMANDN(cleargui, cleargui_, "i");
COMMAND(showgui, "s");
COMMAND(guistayopen, "s");
COMMAND(guinoautotab, "s");

ICOMMAND(guicount, "", (), intret(guistack.length()));

COMMAND(guilist, "s");
COMMAND(guititle, "s");
COMMAND(guibar,"");
COMMAND(guistrut,"ii");
COMMAND(guiimage,"ssfiss");
COMMAND(guislider,"siis");
COMMAND(guilistslider, "sss");
COMMAND(guiradio,"ssis");
COMMAND(guibitfield, "ssis");
COMMAND(guicheckbox, "ssiis");
COMMAND(guitab, "s");
COMMAND(guifield, "sis");
COMMAND(guikeyfield, "sis");
COMMAND(guieditor, "siii");

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

static struct applymenu : menu
{
    void gui(g3d_gui &g, bool firstpass)
    {
        if(guistack.empty()) return;
        g.start(cmenustart, 0.03f, NULL, true);
        g.text("the following settings have changed:", GUI_TEXT_COLOR, "info");
        loopv(needsapply) g.text(needsapply[i].desc, GUI_TEXT_COLOR, "info");
        g.separator();
        g.text("apply changes now?", GUI_TEXT_COLOR, "info");
        if(g.button("yes", GUI_BUTTON_COLOR, "action")&G3D_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
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
    }
} applymenu;

VARP(applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    if(needsapply.length() && guistack.find(&applymenu) < 0)
        pushgui(&applymenu, max(guistack.length()-1, 0));
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) removegui(&applymenu);
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

bool guiactive()
{
	return !guistack.empty();
}

ICOMMAND(guistacklen, "", (void), intret(guistack.length()));
