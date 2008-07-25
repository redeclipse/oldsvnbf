// console.cpp: the console buffer, its display, and command line control

#include "pch.h"
#include "engine.h"

VARP(centertime, 200, 5000, INT_MAX-1);
VARP(centerlines, 0, 3, 5);
FVARP(centerblend, 0.99f);

VARP(contime, 200, 20000, INT_MAX-1);
FVARP(conblend, 0.99f);

ICOMMAND(centerprint, "C", (char *s), console("\f6%s", CON_CENTER, s););

vector<cline> conlines[CN_MAX];

int conskip = 0;

bool saycommandon = false;
string commandbuf;
char *commandaction = NULL, *commandprompt = NULL;
int commandpos = -1;

void setconskip(int *n)
{
	conskip += *n;
	if(conskip<0) conskip = 0;
}

COMMANDN(conskip, setconskip, "i");

void conline(const char *sf, int n, int type = CON_NORMAL)
{
	int _types[] = { CON_NORMAL, CON_CENTER };

	loopi(CN_MAX)
	{
		if (type & _types[i])
		{
			cline cl;
			cl.cref = conlines[i].length()>100 ? conlines[i].pop().cref : newstringbuf("");
			cl.outtime = lastmillis;

			int pos = n && i == CN_CENTER ? n : 0;
			conlines[i].insert(pos,cl);

			int c = 0;
			#define addcref(d) { cl.cref[c] = d; c++; }

			if(type & CON_HILIGHT)
			{
				addcref('\f');
				addcref('0');
			}
			if(n && i != CN_CENTER)
			{
				addcref(' ');
				addcref(' ');
				s_strcat(cl.cref, sf);
			}

			addcref(0);
			s_strcat(cl.cref, sf);

			if (n)
			{
				string sd;
				loopj(2)
				{
					int off = pos+(i != CN_CENTER ? j : -j);
					if (conlines[i].inrange(off))
					{
						if (j) s_sprintf(sd)("%s\fs", conlines[i][off].cref);
						else s_sprintf(sd)("\fS%s", conlines[i][off].cref);
						s_strcpy(conlines[i][off].cref, sd);
					}
				}
			}
		}
	}
}

SVAR(contimefmt, "%c");

void console(const char *s, int type, ...)
{
	extern int scr_w, scr_h;
	int w = screen ? screen->w : scr_w, h = screen ? screen->h : scr_h;
	gettextres(w, h);

	s_sprintfdlv(sf, type, s);

	string osf, psf, fmt;
	s_sprintf(fmt)(contimefmt);
	filtertext(osf, sf);
	s_sprintf(psf)("%s [%02x] %s", gettime(fmt), type, osf);
	printf("%s\n", osf);
	fflush(stdout);

	conline(sf, 0, type);
}

void conoutf(const char *s, ...)
{
	s_sprintfdv(sf, s);
	console("%s", CON_NORMAL, sf);
}

bool fullconsole = false;
void toggleconsole() { fullconsole = !fullconsole; }
COMMAND(toggleconsole, "");

int rendercommand(int x, int y, int w)
{
    if(!saycommandon) return 0;

    s_sprintfd(s)("%s %s", commandprompt ? commandprompt : ">", commandbuf);
    int width, height;
    text_bounds(s, width, height, w);
    y-= height-FONTH;
    draw_text(s, x, y, 0xFF, 0xFF, 0xFF, 0xFF, true, (commandpos>=0) ? (commandpos+1+(commandprompt?strlen(commandprompt):1)) : strlen(s), w);
    return height;
}

void blendbox(int x1, int y1, int x2, int y2, bool border)
{
	notextureshader->set();

	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glBegin(GL_QUADS);
	if(border) glColor3d(0.5, 0.1, 0.1);
	else glColor3d(1.0, 1.0, 1.0);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
	glEnd();
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
	glColor3d(0.7, 0.1, 0.1);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	xtraverts += 8;
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);

	defaultshader->set();
}

VARP(consize, 0, 33, 100);
VARP(fullconsize, 0, 75, 100);

int renderconsole(int w, int h)					// render buffer taking into account time & scrolling
{
	vector<char *> refs;
	refs.setsizenodelete(0);

	pushfont("console");

	if (!guiactive(true, false) && centerlines)
	{
		loopv(conlines[CN_CENTER]) if(lastmillis-conlines[CN_CENTER][i].outtime<centertime)
		{
			refs.add(conlines[CN_CENTER][i].cref);
			if(refs.length() >= centerlines) break;
		}
		int cy = (((h*3)/4)*3)-FONTH*2;
		loopv(refs)
			cy = draw_textx("%s", (w*3)/2, cy, 255, 255, 255, int(255*centerblend), false, AL_CENTER, -1, -1, refs[i]);
	}

	int numl = min(h*3*(fullconsole ? fullconsize : consize)/100, h*3-FONTH/3*2)/FONTH;

	refs.setsizenodelete(0);

	if(numl)
	{
		loopv(conlines[CN_NORMAL])
		{
			if(conskip ? i>=conskip-1 || i>=conlines[CN_NORMAL].length()-numl : fullconsole || lastmillis-conlines[CN_NORMAL][i].outtime<contime)
			{
				refs.add(conlines[CN_NORMAL][i].cref);
				if (refs.length() >= numl) break;
			}
		}
	}

	int cy = FONTH/3;
	loopvrev(refs)
		cy = draw_textx("%s", FONTH/3, cy, 255, 255, 255, int(255*conblend), false, AL_LEFT, -1, w*3-w*3/4-FONTH, refs[i]);

	popfont();

	return cy;
}

// keymap is defined externally in keymap.cfg

struct keym
{
    enum
    {
        ACTION_DEFAULT = 0,
        ACTION_SPECTATOR,
        ACTION_EDITING,
        NUMACTIONS
    };

    int code;
    char *name;
    char *actions[NUMACTIONS];
    bool pressed;

    keym() : code(-1), name(NULL), pressed(false) { memset(actions, 0, sizeof(actions)); }
    ~keym() { DELETEA(name); loopi(NUMACTIONS) DELETEA(actions[i]); }
};

vector<keym> keyms;

void keymap(char *code, char *key)
{
	if(overrideidents) { conoutf("\frcannot override keymap %s", code); return; }
	keym &km = keyms.add();
	km.code = atoi(code);
	km.name = newstring(key);
    loopi(keym::NUMACTIONS) km.actions[i] = newstring("");
}

COMMAND(keymap, "ss");

keym *keypressed = NULL;
char *keyaction = NULL;

keym *findbind(char *key)
{
	loopv(keyms) if(!strcasecmp(keyms[i].name, key)) return &keyms[i];
	return NULL;
}

void getbind(char *key)
{
	keym *km = findbind(key);
    result(km ? km->actions[keym::ACTION_DEFAULT] : "");
}

void getspecbind(char *key)
{
    keym *km = findbind(key);
    result(km ? km->actions[keym::ACTION_SPECTATOR] : "");
}

void geteditbind(char *key)
{
	keym *km = findbind(key);
    result(km ? km->actions[keym::ACTION_EDITING] : "");
}

void bindkey(char *key, char *action, int state, const char *cmd)
{
    if(overrideidents) { conoutf("\frcannot override %s \"%s\"", cmd, key); return; }
	keym *km = findbind(key);
    if(!km) { conoutf("\frunknown key \"%s\"", key); return; }
    char *&binding = km->actions[state];
	if(!keypressed || keyaction!=binding) delete[] binding;
	binding = newstring(action);
}

void bindnorm(char *key, char *action) { bindkey(key, action, keym::ACTION_DEFAULT, "bind"); }
void bindspec(char *key, char *action) { bindkey(key, action, keym::ACTION_SPECTATOR, "specbind"); }
void bindedit(char *key, char *action) { bindkey(key, action, keym::ACTION_EDITING, "editbind"); }

COMMANDN(bind,	 bindnorm, "ss");
COMMANDN(specbind, bindspec, "ss");
COMMANDN(editbind, bindedit, "ss");
COMMAND(getbind, "s");
COMMAND(getspecbind, "s");
COMMAND(geteditbind, "s");

void saycommand(char *init)						 // turns input to the command line on or off
{
	SDL_EnableUNICODE(saycommandon = (init!=NULL));
	keyrepeat(saycommandon);
    s_strcpy(commandbuf, init ? init : "");
    DELETEA(commandaction);
    DELETEA(commandprompt);
	commandpos = -1;
}

void inputcommand(char *init, char *action, char *prompt)
{
    saycommand(init);
    if(action[0]) commandaction = newstring(action);
    if(prompt[0]) commandprompt = newstring(prompt);
}

void mapmsg(char *s) { s_strncpy(hdr.maptitle, s, 128); }

COMMAND(saycommand, "C");
COMMAND(inputcommand, "sss");
COMMAND(mapmsg, "s");

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <SDL_syswm.h>
#endif

void pasteconsole()
{
	#ifdef WIN32
	if(!IsClipboardFormatAvailable(CF_TEXT)) return;
	if(!OpenClipboard(NULL)) return;
	char *cb = (char *)GlobalLock(GetClipboardData(CF_TEXT));
	s_strcat(commandbuf, cb);
	GlobalUnlock(cb);
	CloseClipboard();
	#elif defined(__APPLE__)
	extern void mac_pasteconsole(char *commandbuf);

	mac_pasteconsole(commandbuf);
	#else
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	wminfo.subsystem = SDL_SYSWM_X11;
	if(!SDL_GetWMInfo(&wminfo)) return;
	int cbsize;
	char *cb = XFetchBytes(wminfo.info.x11.display, &cbsize);
	if(!cb || !cbsize) return;
	size_t commandlen = strlen(commandbuf);
	for(char *cbline = cb, *cbend; commandlen + 1 < sizeof(commandbuf) && cbline < &cb[cbsize]; cbline = cbend + 1)
	{
		cbend = (char *)memchr(cbline, '\0', &cb[cbsize] - cbline);
		if(!cbend) cbend = &cb[cbsize];
		if(size_t(commandlen + cbend - cbline + 1) > sizeof(commandbuf)) cbend = cbline + sizeof(commandbuf) - commandlen - 1;
		memcpy(&commandbuf[commandlen], cbline, cbend - cbline);
		commandlen += cbend - cbline;
		commandbuf[commandlen] = '\n';
		if(commandlen + 1 < sizeof(commandbuf) && cbend < &cb[cbsize]) ++commandlen;
		commandbuf[commandlen] = '\0';
	}
	XFree(cb);
	#endif
}

SVAR(commandbuffer, "");

struct hline
{
    char *buf, *action, *prompt;

    hline() : buf(NULL), action(NULL), prompt(NULL) {}
    ~hline()
    {
        DELETEA(buf);
        DELETEA(action);
        DELETEA(prompt);
    }

    void restore()
    {
        s_strcpy(commandbuf, buf);
        if(commandpos >= (int)strlen(commandbuf)) commandpos = -1;
        DELETEA(commandaction);
        DELETEA(commandprompt);
        if(action) commandaction = newstring(action);
        if(prompt) commandprompt = newstring(prompt);
    }

    bool shouldsave()
    {
        return strcmp(commandbuf, buf) ||
               (commandaction ? !action || strcmp(commandaction, action) : action!=NULL) ||
               (commandprompt ? !prompt || strcmp(commandprompt, prompt) : prompt!=NULL);
    }

    void save()
    {
        buf = newstring(commandbuf);
        if(commandaction) action = newstring(commandaction);
        if(commandprompt) prompt = newstring(commandprompt);
    }

    void run()
    {
        if(action)
        {
            setsvar("commandbuffer", buf, true);
            execute(action);
        }
        else if(buf[0]=='/') execute(buf+1);
        else cc->toserver(0, buf);
    }
};
vector<hline *> history;
int histpos = 0;

void history_(int *n)
{
    static bool inhistory = true;
    if(!inhistory && history.inrange(*n))
    {
        inhistory = true;
        history[history.length()-*n-1]->run();
        inhistory = false;
    }
}

COMMANDN(history, history_, "i");

struct releaseaction
{
	keym *key;
	char *action;
};
vector<releaseaction> releaseactions;

const char *addreleaseaction(const char *s)
{
	if(!keypressed) return NULL;
	releaseaction &ra = releaseactions.add();
	ra.key = keypressed;
	ra.action = newstring(s);
	return keypressed->name;
}

void onrelease(char *s)
{
	addreleaseaction(s);
}

COMMAND(onrelease, "s");

void execbind(keym &k, bool isdown)
{
    loopv(releaseactions)
    {
        releaseaction &ra = releaseactions[i];
        if(ra.key==&k)
        {
            if(!isdown) execute(ra.action);
            delete[] ra.action;
            releaseactions.remove(i--);
        }
    }
    if(isdown)
    {
        int state = editmode ? keym::ACTION_EDITING : (cc->state() == CS_SPECTATOR ? keym::ACTION_SPECTATOR : keym::ACTION_DEFAULT);
        char *&action = k.actions[state][0] ? k.actions[state] : k.actions[keym::ACTION_DEFAULT];
        keyaction = action;
        keypressed = &k;
        execute(keyaction);
        keypressed = NULL;
        if(keyaction!=action) delete[] keyaction;
    }
    k.pressed = isdown;
}

void consolekey(int code, bool isdown, int cooked)
{
    #ifdef __APPLE__
        #define MOD_KEYS (KMOD_LMETA|KMOD_RMETA)
    #else
        #define MOD_KEYS (KMOD_LCTRL|KMOD_RCTRL)
    #endif

    if(isdown)
    {
        switch(code)
        {
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                break;

            case SDLK_HOME:
                if(strlen(commandbuf)) commandpos = 0;
                break;

            case SDLK_END:
                commandpos = -1;
                break;

            case SDLK_DELETE:
            {
                int len = (int)strlen(commandbuf);
                if(commandpos<0) break;
                memmove(&commandbuf[commandpos], &commandbuf[commandpos+1], len - commandpos);
                resetcomplete();
                if(commandpos>=len-1) commandpos = -1;
                break;
            }

            case SDLK_BACKSPACE:
            {
                int len = (int)strlen(commandbuf), i = commandpos>=0 ? commandpos : len;
                if(i<1) break;
                memmove(&commandbuf[i-1], &commandbuf[i], len - i + 1);
                resetcomplete();
                if(commandpos>0) commandpos--;
                else if(!commandpos && len<=1) commandpos = -1;
                break;
            }

            case SDLK_LEFT:
                if(commandpos>0) commandpos--;
                else if(commandpos<0) commandpos = (int)strlen(commandbuf)-1;
                break;

            case SDLK_RIGHT:
                if(commandpos>=0 && ++commandpos>=(int)strlen(commandbuf)) commandpos = -1;
                break;

            case SDLK_UP:
                if(histpos>0) history[--histpos]->restore();
                break;

            case SDLK_DOWN:
                if(histpos+1<history.length()) history[++histpos]->restore();
                break;

            case SDLK_TAB:
                if(!commandaction)
                {
                    complete(commandbuf);
                    if(commandpos>=0 && commandpos>=(int)strlen(commandbuf)) commandpos = -1;
                }
                break;

            case SDLK_f:
                if(SDL_GetModState()&MOD_KEYS) { cooked = '\f'; return; }
                // fall through

            case SDLK_v:
                if(SDL_GetModState()&MOD_KEYS) { pasteconsole(); return; }
                // fall through

            default:
                resetcomplete();
                if(cooked)
                {
                    size_t len = (int)strlen(commandbuf);
                    if(len+1<sizeof(commandbuf))
                    {
                        if(commandpos<0) commandbuf[len] = cooked;
                        else
                        {
                            memmove(&commandbuf[commandpos+1], &commandbuf[commandpos], len - commandpos);
                            commandbuf[commandpos++] = cooked;
                        }
                        commandbuf[len+1] = '\0';
                    }
                }
                break;
        }
    }
    else
    {
        if(code==SDLK_RETURN || code==SDLK_KP_ENTER)
        {
            hline *h = NULL;
            if(commandbuf[0])
            {
                if(history.empty() || history.last()->shouldsave())
                    history.add(h = new hline)->save(); // cap this?
                else h = history.last();
            }
            histpos = history.length();
            saycommand(NULL);
            if(h)
            {
            	interactive = true;
            	h->run();
            	interactive = false;
            }
        }
        else if(code==SDLK_ESCAPE)
        {
            histpos = history.length();
            saycommand(NULL);
        }
    }
}

extern bool menukey(int code, bool isdown, int cooked);

void keypress(int code, bool isdown, int cooked)
{
	char alpha = cooked < 0x80 ? cooked : '?';
    keym *haskey = NULL;
    loopv(keyms) if(keyms[i].code==code) { haskey = &keyms[i]; break; }
    if(haskey && haskey->pressed) execbind(*haskey, isdown); // allow pressed keys to release
    else if(!menukey(code, isdown, alpha)) // 3D GUI mouse button intercept
    {
        if(saycommandon) consolekey(code, isdown, alpha);
        else if(haskey) execbind(*haskey, isdown);
    }
}

char *getcurcommand()
{
	return saycommandon ? commandbuf : (char *)NULL;
}

void clear_console()
{
	keyms.setsize(0);
}

void writebinds(FILE *f)
{
    loopv(keyms) loopj(3)
	{
        static const char *cmds[3] = { "bind", "specbind", "editbind" };
        if(*keyms[i].actions[j]) fprintf(f, "%s \"%s\" [%s]\n", cmds[j], keyms[i].name, keyms[i].actions[j]);
	}
}

// tab-completion of all idents and base maps

enum { FILES_DIR = 0, FILES_LIST };

struct fileskey
{
    int type;
	const char *dir, *ext;

	fileskey() {}
    fileskey(int type, const char *dir, const char *ext) : type(type), dir(dir), ext(ext) {}
};

struct filesval
{
    int type;
	char *dir, *ext;
	vector<char *> files;

    filesval(int type, const char *dir, const char *ext) : type(type), dir(newstring(dir)), ext(ext && ext[0] ? newstring(ext) : NULL) {}
	~filesval() { DELETEA(dir); DELETEA(ext); loopv(files) DELETEA(files[i]); files.setsize(0); }
};

static inline bool htcmp(const fileskey &x, const fileskey &y)
{
    return x.type==y.type && !strcmp(x.dir, y.dir) && (x.ext == y.ext || (x.ext && y.ext && !strcmp(x.ext, y.ext)));
}

static inline uint hthash(const fileskey &k)
{
	return hthash(k.dir);
}

static hashtable<fileskey, filesval *> completefiles;
static hashtable<char *, filesval *> completions;

int completesize = 0;
string lastcomplete;

void resetcomplete() { completesize = 0; }

void addcomplete(char *command, int type, char *dir, char *ext)
{
	if(overrideidents)
	{
		conoutf("\frcannot override complete %s", command);
		return;
	}
	if(!dir[0])
	{
		filesval **hasfiles = completions.access(command);
		if(hasfiles) *hasfiles = NULL;
		return;
	}
    if(type==FILES_DIR)
    {
	int dirlen = (int)strlen(dir);
	while(dirlen > 0 && (dir[dirlen-1] == '/' || dir[dirlen-1] == '\\'))
		dir[--dirlen] = '\0';
        if(ext)
        {
	if(strchr(ext, '*')) ext[0] = '\0';
            if(!ext[0]) ext = NULL;
        }
    }
    fileskey key(type, dir, ext);
	filesval **val = completefiles.access(key);
	if(!val)
	{
        filesval *f = new filesval(type, dir, ext);
        if(type==FILES_LIST) explodelist(dir, f->files);
        val = &completefiles[fileskey(type, f->dir, f->ext)];
		*val = f;
	}
	filesval **hasfiles = completions.access(command);
	if(hasfiles) *hasfiles = *val;
	else completions[newstring(command)] = *val;
}

void addfilecomplete(char *command, char *dir, char *ext)
{
    addcomplete(command, FILES_DIR, dir, ext);
}

void addlistcomplete(char *command, char *list)
{
    addcomplete(command, FILES_LIST, list, NULL);
}

COMMANDN(complete, addfilecomplete, "sss");
COMMANDN(listcomplete, addlistcomplete, "ss");

void complete(char *s)
{
	if(*s!='/')
	{
		string t;
		s_strcpy(t, s);
		s_strcpy(s, "/");
		s_strcat(s, t);
	}
	if(!s[1]) return;
	if(!completesize) { completesize = (int)strlen(s)-1; lastcomplete[0] = '\0'; }

	filesval *f = NULL;
	if(completesize)
	{
		char *end = strchr(s, ' ');
		if(end)
		{
			string command;
			s_strncpy(command, s+1, min(size_t(end-s), sizeof(command)));
			filesval **hasfiles = completions.access(command);
			if(hasfiles) f = *hasfiles;
		}
	}

    const char *nextcomplete = NULL;
	string prefix;
	s_strcpy(prefix, "/");
	if(f) // complete using filenames
	{
		int commandsize = strchr(s, ' ')+1-s;
		s_strncpy(prefix, s, min(size_t(commandsize+1), sizeof(prefix)));
        if(f->type==FILES_DIR && f->files.empty()) listfiles(f->dir, f->ext, f->files);
		loopi(f->files.length())
		{
			if(strncmp(f->files[i], s+commandsize, completesize+1-commandsize)==0 &&
				strcmp(f->files[i], lastcomplete) > 0 && (!nextcomplete || strcmp(f->files[i], nextcomplete) < 0))
				nextcomplete = f->files[i];
		}
	}
	else // complete using command names
	{
		enumerate(*idents, ident, id,
            if(id.flags&IDF_COMPLETE && strncmp(id.name, s+1, completesize)==0 &&
               strcmp(id.name, lastcomplete) > 0 && (!nextcomplete || strcmp(id.name, nextcomplete) < 0))
                nextcomplete = id.name;
		);
	}
	if(nextcomplete)
	{
		s_strcpy(s, prefix);
		s_strcat(s, nextcomplete);
		s_strcpy(lastcomplete, nextcomplete);
	}
	else lastcomplete[0] = '\0';
}

void setcompletion(char *s, bool on)
{
	enumerate(*idents, ident, id,
		if(!strcmp(id.name, s))
		{
			if(on && !(id.flags&IDF_COMPLETE)) id.flags |= IDF_COMPLETE;
			else if(!on && id.flags&IDF_COMPLETE) id.flags &= ~IDF_COMPLETE;
			return;
		}
	);
	conoutf("\frcompletion of %s failed as it does not exist", s);
}

ICOMMAND(setcomplete, "ss", (char *s, char *t), {
	bool on = false;
	if (isnumeric(*t)) on = atoi(t) ? true : false;
	else if (!strcasecmp("false", t)) on = false;
	else if (!strcasecmp("true", t)) on = true;
	else if (!strcasecmp("on", t)) on = true;
	else if (!strcasecmp("off", t)) on = false;
	setcompletion(s, on);
});

void writecompletions(FILE *f)
{
	enumeratekt(completions, char *, k, filesval *, v,
        if(!v) continue;
        if(v->type==FILES_LIST) fprintf(f, "listcomplete \"%s\" [%s]\n", k, v->dir);
        else fprintf(f, "complete \"%s\" \"%s\" \"%s\"\n", k, v->dir, v->ext ? v->ext : "*");
	);
}

