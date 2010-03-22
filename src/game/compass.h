FVAR(IDF_PERSIST, compasssize, 0, 0.15f, 1000);
VAR(IDF_PERSIST, compassfade, 0, 250, INT_MAX-1);
FVAR(IDF_PERSIST, compassfadeamt, 0, 0.75f, 1);
TVAR(IDF_PERSIST, compasstex, "textures/compass", 3);
TVAR(IDF_PERSIST, compassringtex, "textures/progress", 3);

struct cstate
{
    char *name, *contents;
    cstate() {}
    ~cstate()
    {
        DELETEP(name);
        DELETEP(contents);
    }
};
struct caction : cstate
{
    char code;
};
struct cmenu : cstate
{
    vector<caction> actions;
    cmenu() {}
    ~cmenu() { reset(); }
    void reset()
    {
        loopvrev(actions) actions.remove(i);
        actions.shrink(0);
    }

    int locate(const char code)
    {
        loopv(actions) if(actions[i].code == code || actions[i].code == toupper(code) || actions[i].code == tolower(code))
            return i;
        return -1;
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
        playsound(S_GUIACT, camera1->o, camera1, SND_FORCED);
        curcompass->reset();
        curcompass = NULL;
    }
}
ICOMMAND(0, clearcompass, "", (), clearcmenu());

void resetcmenus()
{
    clearcmenu();
    loopvrev(cmenus) cmenus.remove(i);
    cmenus.shrink(0);
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

void addaction(const char *name, char code, const char *contents)
{
    if(!name || !*name || !contents || !*contents || !curcompass || curcompass->locate(code) >= 0) return;
    caction &a = curcompass->actions.add();
    a.name = newstring(strlen(name) > 1 ? name : contents);
    a.contents = newstring(contents);
    a.code = code;
}
ICOMMAND(0, compass, "sss", (char *n, char *a, char *b), if(curcompass)
{
    if(*b)
    {
        int code = findkeycode(a);
        if(code) addaction(n, code, b);
    }
    else
    {
        char code = '1';
        while(true)
        {
            if(code > '9') code = 'a';
            if(curcompass->locate(code) < 0) break;
            else if(code >= 'z')
            {
                code = 0;
                break;
            }
            code++;
        }
        if(code) addaction(n, code, a);
    }
});

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
        playsound(S_GUIPRESS, camera1->o, camera1, SND_FORCED);
        return;
    }
    conoutft(CON_DEBUG, "\frno such compass menu: %s", name);
}
ICOMMAND(0, showcompass, "s", (char *n), showcmenu(n));

ICOMMAND(0, compassactive, "", (), result(curcompass ? curcompass->name : ""));

const struct compassdirs
{
    int x, y, align;
} compassdir[9] = {
    { 0, 0, TEXT_CENTERED }, // special cancel case
    { 0, -1, TEXT_CENTERED },       { 1, -1, TEXT_LEFT_JUSTIFY },
    { 1, 0, TEXT_LEFT_JUSTIFY },    { 1, 1, TEXT_LEFT_JUSTIFY  },
    { 0, 1, TEXT_CENTERED },        { -1, 1, TEXT_RIGHT_JUSTIFY  },
    { -1, 0, TEXT_RIGHT_JUSTIFY },  { -1, -1, TEXT_RIGHT_JUSTIFY  }
};

int cmenuhit()
{
    if(curcompass)
    {
        if(compasspos > 0) return compasspos;
        float cx = (cursorx-0.5f)*2.f, cy = (cursory-0.5f)*2.f;
        if(sqrtf(cx*cx+cy*cy) <= compasssize*0.5f) return -1;
        else
        {
            float yaw = -(float)atan2(cx, cy)/RAD+202.5f; if(yaw >= 360) yaw -= 360;
            loopi(min(curcompass->actions.length(), 8)) if(yaw > i*45 && yaw <= (i+1)*45)
                return i;
        }
    }
    return -1;
}

void renderaction(int idx, int size, Texture *t, char code, const char *name, bool hit)
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
        if(idx) drawslice(0.5f/8+(idx-2)/float(8), 1/float(8), hudwidth/2, hudsize/2, size);
        else drawsized(hudwidth/2-size*3/8, hudsize/2-size*3/8, size*3/4);
    }
    switch(compassdir[idx].y)
    {
        case -1:
        {
            pushfont("sub");
            y += draw_textx("%s", x, y, r, g, b, f, compassdir[idx].align, -1, -1, name);
            popfont();
            draw_textx("[%s]", x, y, r, g, b, 255, compassdir[idx].align, -1, -1, getkeyname(code));
            break;
        }
        case 0:
        {
            if(compassdir[idx].x)
            {
                defformatstring(s)("[%s]", getkeyname(code));
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
            y += draw_textx("[%s]", x, y, r, g, b, idx ? 255 : f, compassdir[idx].align, -1, -1, getkeyname(code));
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
    renderaction(0, size, *compassringtex ? textureload(compassringtex, 3) : NULL, '0', "cancel", hit < 0);
    Texture *t = *compasstex ? textureload(compasstex, 3) : NULL;
    loopi(min(curcompass->actions.length(), 8))
        renderaction(i+1, size, t, curcompass->actions[i].code, curcompass->actions[i].name, hit == i);
}

bool runcmenu(int idx)
{
    bool foundmenu = false;
    cmenu *oldcompass = curcompass;
    if(curcompass)
    {
        if(idx < 0)
        {
            foundmenu = true;
            clearcmenu();
        }
        else if(curcompass->actions.inrange(idx))
        {
            foundmenu = interactive = true;
            execute(curcompass->actions[idx].contents);
            interactive = false;
            if(oldcompass == curcompass) clearcmenu();
        }
    }
    return foundmenu;
}

bool keycmenu(int code, bool isdown, int cooked)
{
    switch(code)
    {
        case SDLK_RIGHT: case SDLK_UP: case SDLK_TAB: case -2: case -4:
        {
            if(!isdown && ++compasspos > curcompass->actions.length()) compasspos = 0;
            return true;
            break;
        }
        case SDLK_LEFT: case SDLK_DOWN: case -5:
        {
            if(!isdown && --compasspos < 0) compasspos = 8;
            return true;
            break;
        }
        case SDLK_RETURN: case -1:
        {
            if(!isdown) runcmenu(cmenuhit());
            return true;
            break;
        }
        case SDLK_ESCAPE: case -3:
        {
            if(!isdown) clearcmenu();
            return true;
            break;
        }
        default:
        {
            int idx = code != '0' ? curcompass->locate(code) : -1;
            if(idx >= 0 || code == '0') return isdown || runcmenu(idx);
            break;
        }
    }
    return false;
}
