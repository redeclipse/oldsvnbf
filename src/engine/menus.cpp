// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "engine.h"

guient *cgui = NULL;
menu *cmenu = NULL;
hashtable<const char *, menu> menus;
vector<menu *> menustack;
vector<char *> executelater;
bool shouldclearmenu = true, clearlater = false;
FVAR(IDF_PERSIST, menuscale, 0, 0.02f, 1);
VAR(0, guipasses, 1, -1, -1);

void popgui()
{
    menu *m = menustack.pop();
    m->passes = 0;
    m->clear();
}

void removegui(menu *m)
{
    loopv(menustack) if(menustack[i]==m)
    {
        menustack.remove(i);
        m->passes = 0;
        m->clear();
        return;
    }
}

void pushgui(menu *m, int pos = -1, int tab = 0)
{
    if(menustack.empty()) resetcursor();
    if(pos < 0) menustack.add(m);
    else menustack.insert(pos, m);
    if(m)
    {
        m->passes = 0;
        m->menustart = lastmillis;
        if(tab > 0) m->menutab = tab;
        m->usetitle = tab >= 0 ? true : false;
    }
}

void restoregui(int pos, int tab = 0)
{
    int clear = menustack.length()-pos-1;
    loopi(clear) popgui();
    menu *m = menustack.last();
    if(m)
    {
        m->passes = 0;
        m->menustart = lastmillis;
        if(tab > 0) m->menutab = tab;
    }
}

void showgui(const char *name, int tab)
{
    menu *m = menus.access(name);
    if(!m) return;
    int pos = menustack.find(m);
    if(pos<0) pushgui(m, -1, tab);
    else restoregui(pos, tab);
    playsound(S_GUIPRESS, camera1->o, camera1, SND_FORCED);
}

extern bool closetexgui();
int cleargui(int n)
{
    if(closetexgui()) n--;
    int clear = menustack.length();
    if(n>0) clear = min(clear, n);
    loopi(clear) popgui();
    if(!menustack.empty()) restoregui(menustack.length()-1);
    return clear;
}

void cleargui_(int *n)
{
    intret(cleargui(*n));
}

void guishowtitle(int *n)
{
    if(!cmenu) return;
    cmenu->usetitle = *n ? true : false;
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
SVAR(0, guirollovername, "");
SVAR(0, guirolloveraction, "");

void guibutton(char *name, char *action, char *icon, char *altact)
{
    if(!cgui) return;
    int ret = cgui->button(name, 0xFFFFFF, *icon ? icon : NULL);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            executelater.add(newstring(act));
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", action, true);
    }
}

SVAR(0, guirolloverimgpath, "");
SVAR(0, guirolloverimgaction, "");

void guiimage(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact)
{
    if(!cgui) return;
    Texture *t = path[0] ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            executelater.add(newstring(act));
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirolloverimgpath", path, true);
        setsvar("guirolloverimgaction", action, true);
    }
}

void guislice(char *path, char *action, float *scale, float *start, float *end, char *text, char *altpath, char *altact)
{
    if(!cgui) return;
    Texture *t = path[0] ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->slice(t, *scale, *start, *end, text[0] ? text : NULL);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact[0] && ret&GUI_ALT) act = altact;
        else if(action[0]) act = action;
        if(act[0])
        {
            executelater.add(newstring(act));
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirolloverimgpath", path, true);
        setsvar("guirolloverimgaction", action, true);
    }
}

void guitext(char *name, char *icon)
{
    if(cgui) cgui->text(name, 0xFFFFFF, icon[0] ? icon : NULL);
}

void guititle(char *name)
{
    if(cgui) cgui->title(name);
}

void guitab(char *name)
{
    if(cgui) cgui->tab(name);
}

void guibar()
{
    if(cgui) cgui->separator();
}

void guistrut(float *strut, int *alt)
{
    if(cgui)
    {
        if(!*alt) cgui->pushlist();
        cgui->strut(*strut);
        if(!*alt) cgui->poplist();
    }
}

void guifont(char *font, char *body)
{
    if(cgui)
    {
        if(font[0])
        {
            cgui->pushfont(font);
            if(body[0])
            {
                execute(body);
                cgui->popfont();
            }
        }
        else cgui->popfont();
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
            formatstring(assign)("%s %d", var, val);
            break;
        case ID_ALIAS:
            formatstring(assign)("%s = %d", var, val);
            break;
        default:
            return;
    }
    executelater.add(newstring(assign));
    if(onchange[0]) executelater.add(newstring(onchange));
}

static void updatefval(char *var, float val, char *onchange)
{
    ident *id = getident(var);
    string assign;
    if(!id) return;
    switch(id->type)
    {
        case ID_FVAR:
            formatstring(assign)("%s %f", var, val);
            break;
        case ID_VAR:
        case ID_SVAR:
            formatstring(assign)("%s %d", var, int(val));
            break;
        case ID_ALIAS:
            formatstring(assign)("%s = %d", var, int(val));
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

static float getfval(char *var)
{
    ident *id = getident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return *id->storage.f;
        case ID_SVAR: return atof(*id->storage.s);
        case ID_ALIAS: return atof(id->action);
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

void guiprogress(float *percent, float *scale)
{
    if(!cgui) return;
    cgui->progress(*percent, *scale);
}

void guislider(char *var, int *min, int *max, char *onchange, int *reverse, int *scroll)
{
    if(!cgui) return;
    int oldval = getval(var), val = oldval, vmin = *max ? *min : getvarmin(var), vmax = *max ? *max : getvarmax(var);
    if(vmax >= INT_MAX-1)
    { // not a sane value for a slider..
        int vdef = getvardef(var);
        vmax = vdef > vmin ? vdef*3 : vmin*4;
    }
    cgui->slider(val, vmin, vmax, 0xFFFFFF, NULL, *reverse ? true : false, *scroll ? true : false);
    if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange, int *reverse, int *scroll)
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
    defformatstring(label)("%d", val);
    cgui->slider(offset, 0, vals.length()-1, 0xFFFFFF, label, *reverse ? true : false, *scroll ? true : false);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guinameslider(char *var, char *names, char *list, char *onchange, int *reverse, int *scroll)
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
    char *label = indexlist(names, offset);
    cgui->slider(offset, 0, vals.length()-1, 0xFFFFFF, label, *reverse ? true : false, *scroll ? true : false);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
    delete[] label;
}

void guicheckbox(char *name, char *var, float *on, int *off, char *onchange)
{
    bool enabled = getfval(var)!=*off;
    if(cgui && cgui->button(name, 0xFFFFFF, enabled ? "checkboxon" : "checkbox", enabled ? false : true)&GUI_UP)
    {
        updatefval(var, enabled ? *off : (*on || *off ? *on : 1), onchange);
    }
}

void guiradio(char *name, char *var, float *n, char *onchange)
{
    bool enabled = getfval(var)==*n;
    if(cgui && cgui->button(name, 0xFFFFFF, enabled ? "radioboxon" : "radiobox", enabled ? false : true)&GUI_UP)
    {
        if(!enabled) updatefval(var, *n, onchange);
    }
}

void guibitfield(char *name, char *var, int *mask, char *onchange)
{
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    if(cgui && cgui->button(name, 0xFFFFFF, enabled ? "checkboxon" : "checkbox", enabled ? false : true)&GUI_UP)
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
    char *result = cgui->field(var, 0x666666, *maxlength ? *maxlength : 12, 0, initval);
    if(result)
    {
        alias(var, result);
        if(onchange[0])
        {
            interactive = true;
            execute(onchange);
            interactive = false;
        }
    }
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode)
{
    if(!cgui) return;
    cgui->field(name, 0x666666, *maxlength ? *maxlength : 12, *height, NULL, *mode<=0 ? EDITORFOREVER : *mode);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange)
{
    if(!cgui) return;
    const char *initval = "";
    ident *id = getident(var);
    if(id && id->type==ID_ALIAS) initval = id->action;
    char *result = cgui->keyfield(var, 0x666666, *maxlength ? *maxlength : -8, 0, initval);
    if(result)
    {
        alias(var, result);
        if(onchange[0])
        {
            interactive = true;
            execute(onchange);
            interactive = false;
        }
    }
}

//use text<action> to do more...

void guibody(char *contents, char *action, char *altact)
{
    if(!cgui) return;
    cgui->pushlist(action[0] ? true : false);
    execute(contents);
    int ret = cgui->poplist();
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(ret&GUI_ALT && altact[0]) act = altact;
        else if(action[0]) act = action;
        if(act)
        {
            executelater.add(newstring(act));
            if(shouldclearmenu) clearlater = true;
        }
    }
}

void guilist(char *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    execute(contents);
    cgui->poplist();
}

void newgui(char *name, char *contents, char *initscript)
{
    menu *m = menus.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &menus[name];
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
}

void guiheader(char *name)
{
    if(!cmenu) return;
    DELETEA(cmenu->header);
    cmenu->header = name && name[0] ? newstring(name) : NULL;
}

void guimodify(char *name, char *contents)
{
    menu *m = menus.access(name);
    if(!m) return;
    if(m->contents) delete[] m->contents;
    m->contents = contents && contents[0] ? newstring(contents) : NULL;
}

COMMAND(0, newgui, "sss");
COMMAND(0, guiheader, "s");
COMMAND(0, guimodify, "ss");
COMMAND(0, guibutton, "ssss");
COMMAND(0, guitext, "ss");
COMMANDN(0, cleargui, cleargui_, "i");
ICOMMAND(0, showgui, "ss", (const char *s, const char *n), showgui(s, n[0] ? atoi(n) : 0));
COMMAND(0, guishowtitle, "i");
COMMAND(0, guistayopen, "s");
COMMAND(0, guinoautotab, "s");

ICOMMAND(0, guicount, "", (), intret(menustack.length()));

COMMAND(0, guilist, "s");
COMMAND(0, guibody, "sss");
COMMAND(0, guititle, "s");
COMMAND(0, guibar,"");
COMMAND(0, guistrut,"fi");
COMMAND(0, guifont,"ss");
COMMAND(0, guiimage,"ssfiss");
COMMAND(0, guislice,"ssfffsss");
COMMAND(0, guiprogress,"ff");
COMMAND(0, guislider,"siisii");
COMMAND(0, guilistslider, "sssii");
COMMAND(0, guinameslider, "ssssii");
COMMAND(0, guiradio,"ssfs");
COMMAND(0, guibitfield, "ssis");
COMMAND(0, guicheckbox, "ssffs");
COMMAND(0, guitab, "s");
COMMAND(0, guifield, "sis");
COMMAND(0, guikeyfield, "sis");
COMMAND(0, guieditor, "siii");

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
    void gui(guient &g, bool firstpass)
    {
        if(menustack.empty()) return;
        g.start(menustart, menuscale, &menutab, true);
        g.text("the following settings have changed:", 0xFFFFFF, "info");
        loopv(needsapply) g.text(needsapply[i].desc, 0xFFFFFF, "info");
        g.separator();
        g.text("apply changes now?", 0xFFFFFF, "info");
        if(g.button("yes", 0xFFFFFF, "action")&GUI_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
            if(changetypes&CHANGE_GFX) executelater.add(newstring("resetgl"));
            if(changetypes&CHANGE_SOUND) executelater.add(newstring("resetsound"));
            clearlater = true;
        }
        if(g.button("no", 0xFFFFFF, "action")&GUI_UP)
            clearlater = true;
        g.end();
    }

    void clear()
    {
        needsapply.setsize(0);
    }
} applymenu;

VAR(IDF_PERSIST, applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    if(needsapply.length() && menustack.find(&applymenu) < 0)
        pushgui(&applymenu, max(menustack.length()-1, 0));
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
    int level = menustack.length();
    interactive = true;
    loopv(executelater) execute(executelater[i]);
    executelater.deletecontentsa();
    interactive = false;
    if(clearlater)
    {
        if(level==menustack.length()) loopi(level) popgui();
        clearlater = false;
    }
}

void progressmenu()
{
    menu *m = menus.access("loading");
    if(m)
    {
        m->usetitle = m->useinput = false;
        UI::addcb(m);
    }
    else conoutf("cannot find menu 'loading'");
}

void mainmenu()
{
    if(!menustack.empty()) UI::addcb(menustack.last());
}

bool menuactive()
{
    return !menustack.empty();
}

ICOMMAND(0, menustacklen, "", (void), intret(menustack.length()));
