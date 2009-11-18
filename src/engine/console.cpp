// console.cpp: the console buffer, its display, and command line control

#include "engine.h"

VARP(showconsole, 0, 1, 1);
VARP(consoletime, 200, 20000, INT_MAX-1);

vector<cline> conlines;

int commandmillis = -1;
string commandbuf;
char *commandaction = NULL, *commandicon = NULL;
int commandpos = -1;

void conline(int type, const char *sf, int n)
{
	cline cl;
	cl.type = type;
	cl.cref = conlines.length()>100 ? conlines.pop().cref : newstringbuf("");
	cl.reftime = cl.outtime = lastmillis;
	conlines.insert(n, cl);

	int c = 0;
	#define addcref(d) { cl.cref[c] = d; c++; }
	if(n)
	{
		addcref(' ');
		addcref(' ');
		concatstring(cl.cref, sf);
	}
	addcref(0);
	concatstring(cl.cref, sf);

	if(n)
	{
		string sd;
		loopj(2)
		{
			int off = n+j;
			if(conlines.inrange(off))
			{
				if(j) formatstring(sd)("%s\fs", conlines[off].cref);
				else formatstring(sd)("\fS%s", conlines[off].cref);
				copystring(conlines[off].cref, sd);
			}
		}
	}
}

// keymap is defined externally in keymap.cfg
struct keym
{
    enum
    {
        ACTION_DEFAULT = 0,
        ACTION_SPECTATOR,
        ACTION_EDITING,
        ACTION_WAITING,
        NUMACTIONS
    };

    int code;
    char *name;
    char *actions[NUMACTIONS];
    bool pressed;

    keym() : code(-1), name(NULL), pressed(false) { loopi(NUMACTIONS) actions[i] = newstring(""); }
    ~keym() { DELETEA(name); loopi(NUMACTIONS) DELETEA(actions[i]); }
};

/*
extern keym *keypressed;
extern char *keyaction;
extern keym *findbind(const char *key);
extern int findactionkey(const char *action, int which, int num);

extern const char *retbind(const char *key, int which);
extern const char *retbindaction(const char *action, int which, int num);
*/

hashtable<int, keym> keyms(128);

void keymap(int *code, char *key)
{
	if(overrideidents) { conoutf("\frcannot override keymap %s", code); return; }
    keym &km = keyms[*code];
    km.code = *code;
    DELETEA(km.name);
    km.name = newstring(key);
}

COMMAND(keymap, "is");

keym *keypressed = NULL;
char *keyaction = NULL;

const char *getkeyname(int code)
{
    keym *km = keyms.access(code);
    return km ? km->name : NULL;
}

void searchbindlist(const char *action, int type, int limit, const char *sep, const char *pretty, vector<char> &names)
{
    const char *name1 = NULL, *name2 = NULL, *lastname = NULL;
    int found = 0;
    enumerate(keyms, keym, km,
    {
    	char *act = ((!km.actions[type] || !*km.actions[type]) && type ? km.actions[keym::ACTION_DEFAULT] : km.actions[type]);
        if(!strcmp(act, action))
        {
            if(!name1) name1 = km.name;
            else if(!name2) name2 = km.name;
            else
            {
                #define ADDSEP(comma, conj) do { \
                    if(pretty) \
                    { \
                        names.put("\fs", 2); \
                        names.put(pretty, strlen(pretty)); \
                        if(comma) names.add(comma); \
                        names.add(' '); \
                        if((conj)[0]) \
                        { \
                            names.put(conj, strlen(conj)); \
                            names.add(' '); \
                        } \
                        names.put("\fS", 2); \
                    } \
                    else if(sep && *sep) names.put(sep, strlen(sep)); \
                    else names.add(' '); \
                } while(0)
                if(lastname)
                {
                    ADDSEP(',', "");
                    names.put(lastname, strlen(lastname));
                }
                else
                {
                    names.put(name1, strlen(name1));
                    ADDSEP(',', "");
                    names.put(name2, strlen(name2));
                }
                lastname = km.name;
            }
            ++found;
            if(limit > 0 && found >= limit) break;
        }
    });
    if(lastname)
    {
        ADDSEP(',', sep);
        names.put(lastname, strlen(lastname));
    }
    else
    {
        if(name1) names.put(name1, strlen(name1));
        if(name2)
        {
            ADDSEP('\0', sep);
            names.put(name2, strlen(name2));
        }
    }
    names.add('\0');
}

const char *searchbind(const char *action, int type)
{
    enumerate(keyms, keym, km,
    {
    	char *act = ((!km.actions[type] || !*km.actions[type]) && type ? km.actions[keym::ACTION_DEFAULT] : km.actions[type]);
        if(!strcmp(act, action)) return km.name;
    });
    return NULL;
}

keym *findbind(char *key)
{
    enumerate(keyms, keym, km,
    {
        if(!strcasecmp(km.name, key)) return &km;
    });
    return NULL;
}

void getbind(char *key, int type)
{
    keym *km = findbind(key);
    result(km ? ((!km->actions[type] || !*km->actions[type]) && type ? km->actions[keym::ACTION_DEFAULT] : km->actions[type]) : "");
}

int changedkeys = 0;

void bindkey(char *key, char *action, int state, const char *cmd)
{
    if(overrideidents) { conoutf("\frcannot override %s \"%s\"", cmd, key); return; }
	keym *km = findbind(key);
    if(!km) { conoutf("\frunknown key \"%s\"", key); return; }
    char *&binding = km->actions[state];
	if(!keypressed || keyaction!=binding) delete[] binding;
    // trim white-space to make searchbinds more reliable
    while(isspace(*action)) action++;
    int len = strlen(action);
    while(len>0 && isspace(action[len-1])) len--;
    binding = newstring(action, len);
    changedkeys = totalmillis;
}

ICOMMAND(bind,     "ss", (char *key, char *action), bindkey(key, action, keym::ACTION_DEFAULT, "bind"));
ICOMMAND(specbind, "ss", (char *key, char *action), bindkey(key, action, keym::ACTION_SPECTATOR, "specbind"));
ICOMMAND(editbind, "ss", (char *key, char *action), bindkey(key, action, keym::ACTION_EDITING, "editbind"));
ICOMMAND(waitbind, "ss", (char *key, char *action), bindkey(key, action, keym::ACTION_WAITING, "waitbind"));
ICOMMAND(getbind,     "s", (char *key), getbind(key, keym::ACTION_DEFAULT));
ICOMMAND(getspecbind, "s", (char *key), getbind(key, keym::ACTION_SPECTATOR));
ICOMMAND(geteditbind, "s", (char *key), getbind(key, keym::ACTION_EDITING));
ICOMMAND(getwaitbind, "s", (char *key), getbind(key, keym::ACTION_WAITING));
ICOMMAND(searchbinds,     "sis", (char *action, int *limit, char *sep), { vector<char> list; searchbindlist(action, keym::ACTION_DEFAULT, max(*limit, 0), sep, NULL, list); result(list.getbuf()); });
ICOMMAND(searchspecbinds, "sis", (char *action, int *limit, char *sep), { vector<char> list; searchbindlist(action, keym::ACTION_SPECTATOR, max(*limit, 0), sep, NULL, list); result(list.getbuf()); });
ICOMMAND(searcheditbinds, "sis", (char *action, int *limit, char *sep), { vector<char> list; searchbindlist(action, keym::ACTION_EDITING, max(*limit, 0), sep, NULL, list); result(list.getbuf()); });
ICOMMAND(searchwaitbinds, "sis", (char *action, int *limit, char *sep), { vector<char> list; searchbindlist(action, keym::ACTION_WAITING, max(*limit, 0), sep, NULL, list); result(list.getbuf()); });

void inputcommand(char *init, char *action = NULL, char *icon = NULL) // turns input to the command line on or off
{
    commandmillis = init ? lastmillis : -lastmillis;
    SDL_EnableUNICODE(commandmillis > 0 ? 1 : 0);
    keyrepeat(commandmillis > 0);
    copystring(commandbuf, init ? init : "");
    DELETEA(commandaction);
    DELETEA(commandicon);
    commandpos = -1;
    if(action && action[0]) commandaction = newstring(action);
    if(icon && icon[0]) commandicon = newstring(icon);
}

ICOMMAND(saycommand, "C", (char *init), inputcommand(init));
COMMAND(inputcommand, "sss");

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
	concatstring(commandbuf, cb);
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
    char *buf, *action, *icon;

    hline() : buf(NULL), action(NULL), icon(NULL) {}
    ~hline()
    {
        DELETEA(buf);
        DELETEA(action);
        DELETEA(icon);
    }

    void restore()
    {
        copystring(commandbuf, buf);
        if(commandpos >= (int)strlen(commandbuf)) commandpos = -1;
        DELETEA(commandaction);
        DELETEA(commandicon);
        if(action) commandaction = newstring(action);
        if(icon) commandicon = newstring(icon);
    }

    bool shouldsave()
    {
        return strcmp(commandbuf, buf) ||
               (commandaction ? !action || strcmp(commandaction, action) : action!=NULL) ||
               (commandicon ? !icon || strcmp(commandicon, icon) : icon!=NULL);
    }

    void save()
    {
        buf = newstring(commandbuf);
        if(commandaction) action = newstring(commandaction);
        if(commandicon) icon = newstring(commandicon);
    }

    void run()
    {
        if(buf[0]=='/') execute(buf+1); // above all else
		else if(action)
        {
            setsvar("commandbuffer", buf, true);
            execute(action);
        }
        else client::toserver(0, buf);
    }
};
vector<hline *> history;
int histpos = 0;

void history_(int *n)
{
    static bool inhistory = false;
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

        int state = keym::ACTION_DEFAULT;
        switch(client::state())
        {
        	case CS_ALIVE: case CS_DEAD: default: break;
        	case CS_SPECTATOR: state = keym::ACTION_SPECTATOR; break;
        	case CS_EDITING: state = keym::ACTION_EDITING; break;
        	case CS_WAITING: state = keym::ACTION_WAITING; break;
        }
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
                //if(!commandaction)
                //{
                    complete(commandbuf);
                    if(commandpos>=0 && commandpos>=(int)strlen(commandbuf)) commandpos = -1;
                //}
                break;

            case SDLK_f:
                if(SDL_GetModState()&MOD_KEYS) cooked = '\f';
                // fall through

            case SDLK_v:
                if(SDL_GetModState()&MOD_KEYS)
                {
                	pasteconsole();
                	break;
				}
                // fall through

            case SDLK_F4:
                if(SDL_GetModState()&MOD_KEYS)
                {
                	quit();
                	break;
				}
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
            inputcommand(NULL);
            if(h)
            {
            	interactive = true;
            	h->run();
            	interactive = false;
            }
        }
        else if(code==SDLK_ESCAPE || code < 0)
        {
            histpos = history.length();
            inputcommand(NULL);
        }
    }
}

void keypress(int code, bool isdown, int cooked)
{
	char alpha = cooked < 0x80 ? cooked : '?';
    keym *haskey = keyms.access(code);
    if(haskey && haskey->pressed) execbind(*haskey, isdown); // allow pressed keys to release
	else if(commandmillis > 0) consolekey(code, isdown, alpha);
	else if(!UI::keypress(code, isdown, alpha) && haskey) execbind(*haskey, isdown);
}

char *getcurcommand()
{
	return commandmillis > 0 ? commandbuf : (char *)NULL;
}

void clear_console()
{
	keyms.clear();
}

static int sortbinds(keym **x, keym **y)
{
    return strcmp((*x)->name, (*y)->name);
}

void writebinds(stream *f)
{
    static const char *cmds[4] = { "bind", "specbind", "editbind", "waitbind" };
    vector<keym *> binds;
    enumerate(keyms, keym, km, binds.add(&km));
    binds.sort(sortbinds);
    loopj(4)
    {
        loopv(binds)
        {
            keym &km = *binds[i];
            if(*km.actions[j]) f->printf("\t%s \"%s\" [%s]\n", cmds[j], km.name, km.actions[j]);
        }
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
    int millis;

    filesval(int type, const char *dir, const char *ext) : type(type), dir(newstring(dir)), ext(ext && ext[0] ? newstring(ext) : NULL), millis(-1) {}
	~filesval() { DELETEA(dir); DELETEA(ext); loopv(files) DELETEA(files[i]); files.setsize(0); }

    static int comparefiles(char **x, char **y) { return strcmp(*x, *y); }

    void update()
    {
        if(type!=FILES_DIR || millis >= commandmillis) return;
        files.deletecontentsa();
        listfiles(dir, ext, files);
        files.sort(comparefiles);
        loopv(files) if(i && !strcmp(files[i], files[i-1])) delete[] files.remove(i--);
        millis = lastmillis;
    }
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
		copystring(t, s);
		copystring(s, "/");
		concatstring(s, t);
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
			copystring(command, s+1, min(size_t(end-s), sizeof(command)));
			filesval **hasfiles = completions.access(command);
			if(hasfiles) f = *hasfiles;
		}
	}

    const char *nextcomplete = NULL;
	string prefix;
	copystring(prefix, "/");
	if(f) // complete using filenames
	{
		int commandsize = strchr(s, ' ')+1-s;
		copystring(prefix, s, min(size_t(commandsize+1), sizeof(prefix)));
		f->update();
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
		copystring(s, prefix);
		concatstring(s, nextcomplete);
		copystring(lastcomplete, nextcomplete);
	}
	else lastcomplete[0] = '\0';
}

void setcompletion(const char *s, bool on)
{
    ident *id = idents->access(s);
    if(!s) conoutf("\frcompletion of %s failed as it does not exist", s);
    else
    {
	    if(on && !(id->flags&IDF_COMPLETE)) id->flags |= IDF_COMPLETE;
		else if(!on && id->flags&IDF_COMPLETE) id->flags &= ~IDF_COMPLETE;
	}
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

static int sortcompletions(char **x, char **y)
{
    return strcmp(*x, *y);
}

void writecompletions(stream *f)
{
    vector<char *> cmds;
    enumeratekt(completions, char *, k, filesval *, v, { if(v) cmds.add(k); });
    cmds.sort(sortcompletions);
    loopv(cmds)
    {
        char *k = cmds[i];
        filesval *v = completions[k];
        if(v->type==FILES_LIST) f->printf("\tlistcomplete \"%s\" [%s]\n", k, v->dir);
        else f->printf("\tcomplete \"%s\" \"%s\" \"%s\"\n", k, v->dir, v->ext ? v->ext : "*");
    }
}

