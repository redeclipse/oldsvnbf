// Joysticks based on SauerJoy by dbox

#include "pch.h"
#include "engine.h"

#define JOYBUTTONKEYMAP 320
#define JOYAXISKEYMAP	340
#define JOYHATKEYMAP	360

#define MOUSELBUTTON -1

struct joyaxis
{
	int value;
	bool min_pressed;
	bool max_pressed;
};

struct joymenuaction
{
	char* label;
	char* action;
	~joymenuaction()
	{
		if (label == NULL)
			delete label;
		if (action == NULL)
			delete action;
	}
};

VAR(joyshowevents, 0, 0, 1);

struct joystick
{
	int HAT_POS[9];
	char* HAT_POS_NAME[9];
	
	int num;
	bool enabled;
	int axis_count, hat_count;
	int button_count, *last_hat_move;
	joyaxis* jmove;
	
	int jxaxis, jyaxis, jmove_xaxis, jmove_yaxis;
	bool jmove_left_pressed, jmove_right_pressed, jmove_up_pressed, jmove_down_pressed;
	
	vector<joymenuaction*> joyactions;
	vector<char*> joysayactions;
		
	int _joyfovxaxis, _joyfovyaxis, _joysensitivity, _joyaxisminmove, _joyfovinvert, _joymousebutton;

	#define JCOMMAND(a, b, c, d, f) \
		if (a) CCOMMAND(b##a, c, d, f); \
		else CCOMMAND(b, c, d, f);

	#define JVAR(a, b, c, d, e) \
		_##b = d; \
		if (a) { \
			CCOMMAND(b##a, "i", (joystick *self, char *s), { \
				if (*s) { \
					int _n = atoi(s); \
					if (_n <= e && _n >= c) self->_##b = _n; \
					else conoutf("valid range for %s is %d..%d", #b#a, c, e); \
				} \
				else conoutf("%s = %d", #b#a, self->_##b); \
			}); \
		} \
		else { \
			CCOMMAND(b, "i", (joystick *self, char *s), { \
				if (*s) { \
					int _n = atoi(s); \
					if (_n <= e && _n >= c) self->_##b = _n; \
					else conoutf("valid range for %s is %d..%d", #b, c, e); \
				} \
				else conoutf("%s = %d", #b, self->_##b); \
			}); \
		}

	joystick(int _num)
	{
		num = _num;
		enabled = false;
		axis_count = 0;
		jmove = NULL;
		
		hat_count = 0;
		last_hat_move = NULL;
		HAT_POS[0] = SDL_HAT_CENTERED;
		HAT_POS[1] = SDL_HAT_UP;
		HAT_POS[2] = SDL_HAT_RIGHTUP;
		HAT_POS[3] = SDL_HAT_RIGHT;
		HAT_POS[4] = SDL_HAT_RIGHTDOWN;
		HAT_POS[5] = SDL_HAT_DOWN;
		HAT_POS[6] = SDL_HAT_LEFTDOWN;
		HAT_POS[7] = SDL_HAT_LEFT;
		HAT_POS[8] = SDL_HAT_LEFTUP;
		
		HAT_POS_NAME[0] = "CENTER";
		HAT_POS_NAME[1] = "NORTH";
		HAT_POS_NAME[2] = "NE";
		HAT_POS_NAME[3] = "EAST";
		HAT_POS_NAME[4] = "SE";
		HAT_POS_NAME[5] = "SOUTH";
		HAT_POS_NAME[6] = "SW";
		HAT_POS_NAME[7] = "WEST";
		HAT_POS_NAME[8] = "NW";
		
		jxaxis = 0;
		jyaxis = 0;

		SDL_Joystick* stick = SDL_JoystickOpen(num);
		if (stick == NULL)
		{
			// there was some sort of problem, bail
			enabled = false;
			return;
		}
		
		// how many axes are there?
		axis_count = SDL_JoystickNumAxes(stick);
		if (axis_count < 2)
		{
			// sorry, we need at least one set of axes to control fov
			enabled = false;
			return;
		}
		
		// initialize the array of joyaxis structures used to maintain
		// axis position information
		if (jmove != NULL)
			free(jmove);
		jmove = (joyaxis*)malloc(sizeof(joyaxis)*axis_count);
		
		// initialize any hat controls
		hat_count = SDL_JoystickNumHats(stick);
		if (hat_count > 0)
		{
			if (last_hat_move != NULL) free(last_hat_move);
			last_hat_move = (int*)malloc(sizeof(int)*hat_count);
			
			for (int i=0; i < hat_count; i++) last_hat_move[i] = 0;
		}
		
		// how many buttons do we have to work with
		button_count = SDL_JoystickNumButtons(stick);
		
		// if we made it this far, everything must be ok
		JCOMMAND(num, joyaction, "ss", (joystick *self, char *a, char *b), self->add_joyaction(a, b));
		JCOMMAND(num, joyactionsay, "s", (joystick *self, char *a), self->add_joysayaction(a));
		JCOMMAND(num, joymenu, "", (joystick *self), self->show_menu());

		JVAR(num, joyfovxaxis, 1, 1, 10);
		JVAR(num, joyfovyaxis, 1, 2, 10);
		JVAR(num, joysensitivity, 1, 8, 10);
		JVAR(num, joyaxisminmove, 1, 6, 30);
		JVAR(num, joyfovinvert, 0, 1, 1);
		JVAR(num, joymousebutton, 0, 0, 100);

		enabled = true;
	}
	
	~joystick()
	{
		// clean up
		if (jmove != NULL) free(jmove);
		if (last_hat_move != NULL) free(last_hat_move);
			
		joyactions.deletecontentsp();
		joysayactions.deletecontentsp();
	}
	
	// translate joystick events into keypress events
	void process_event(SDL_Event* event)
	{
		if (!enabled) return;
			
		int hat_pos;
		
		if ((_joymousebutton-1) == event->button.button &&
				(event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP))
		{
			keypress(MOUSELBUTTON, event->button.state!=0, 0);
		}
		
		switch (event->type)
		{
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			if (joyshowevents)
				conoutf("JOYBUTTON%d: %s", event->button.button+1,
						(event->button.state == SDL_PRESSED) ? "DOWN" : "UP");
			keypress(event->button.button + JOYBUTTONKEYMAP, event->button.state==SDL_PRESSED, 0);
			break;
			
			case SDL_JOYAXISMOTION:
			if (joyshowevents)
				conoutf("JOYAXIS%d: %d", event->jaxis.axis+1, event->jaxis.value);
				
			if (event->jaxis.axis == _joyfovxaxis-1)
				jxaxis = event->jaxis.value;
			else if (event->jaxis.axis == _joyfovyaxis-1)
				jyaxis = event->jaxis.value;
				
			jmove[event->jaxis.axis].value = event->jaxis.value;
			break;
			
			case SDL_JOYHATMOTION:
			hat_pos = get_hat_pos(event->jhat.value);
			if (joyshowevents && hat_pos)
				conoutf("JOYPAD%d%s", event->jhat.hat+1, HAT_POS_NAME[hat_pos]);
			process_hat(event->jhat.hat, hat_pos);
			break;
		}
	}
	
	int get_hat_pos(int hat)
	{
		for (int i=0; i < 9; i++)
			if (hat == HAT_POS[i])
				return i;
		return 0;
	}
	
	// translate any hat movements into keypress events
	void process_hat(int index, int current)
	{
		if (last_hat_move[index] > 0)
		{
			int key = JOYHATKEYMAP + index*8 + last_hat_move[index] - 1;
			keypress(key, false, 0);
		}
		last_hat_move[index] = current;
		if (current > 0)
		{
			int key = JOYHATKEYMAP + index*8 + last_hat_move[index] - 1;
			keypress(key, true, 0);
		}
	}
	
	void move()
	{
		if (!enabled) return;
			
		// move the player's field of view
		float dx = jxaxis/3200.0;
		float dy = jyaxis/3200.0;
		
		float jsensitivity = .1f * _joysensitivity;
		
		if (dx < 0.0f) dx *= dx * jsensitivity * -1;
		else dx *= dx * jsensitivity;
			
		if (dy < 0.0f) dy *= dy * jsensitivity * -1;
		else dy *= dy * jsensitivity;
			
		if (_joyfovinvert) dy *= -1;
			
		if (fabs(dx) >= 1.0f || fabs(dy) >= 1.0f)
			mousemove((int)dx, (int)dy);
			
		// translate any axis movements into keypress events
		for (int i=0; i < axis_count; i++)
		{
			int key = JOYAXISKEYMAP + i * 2;
			if (jmove[i].value > _joyaxisminmove * 1000)
			{
				if (jmove[i].min_pressed)
					keypress(key, false, 0);
				jmove[i].min_pressed = false;
				if (!jmove[i].max_pressed)
					keypress(key+1, true, 0);
				jmove[i].max_pressed = true;
			}
			else if (jmove[i].value < _joyaxisminmove * -1000)
			{
				if (jmove[i].max_pressed)
					keypress(key+1, false, 0);
				jmove[i].max_pressed = false;
				if (!jmove[i].min_pressed)
					keypress(key, true, 0);
				jmove[i].min_pressed = true;
			}
			else
			{
				if (jmove[i].max_pressed)
					keypress(key+1, false, 0);
				jmove[i].max_pressed = false;
				if (jmove[i].min_pressed)
					keypress(key, false, 0);
				jmove[i].min_pressed = false;
			}
		}
	}
	
	void show_menu()
	{
		cleargui();
		
		char label[200];
		char command[500];
		char item[500];
		char menuscript[5000];
		
		strcpy(menuscript, "newgui joyfovxaxis [\n");
		for (int i=0; i < axis_count; i++)
		{
			if (i+1 == _joyfovxaxis)
				sprintf(label, "[axis %d]", i+1);
			else
				sprintf(label, " axis %d", i+1);
			sprintf(command, "joyfovxaxis %d; joymenu", i+1);
			sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
			strcat(menuscript, item);
		}
		strcat(menuscript, "]\n");
		execute(menuscript);
		
		strcpy(menuscript, "newgui joyfovyaxis [\n");
		for (int i=0; i < axis_count; i++)
		{
			if (i+1 == _joyfovyaxis)
				sprintf(label, "[axis %d]", i+1);
			else
				sprintf(label, " axis %d", i+1);
			sprintf(command, "joyfovyaxis %d; joymenu", i+1);
			sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
			strcat(menuscript, item);
		}
		strcat(menuscript, "]\n");
		execute(menuscript);
		
		char buffer[200];
		strcpy(menuscript, "newgui joymousebutton [\n");
		strcat(menuscript, "guititle \"emulate mouse button\"\n");
		if (_joymousebutton == 0)
		{
			sprintf(buffer, "guibutton \"[not set]\" \"joymousebutton -1; joymenu\"\n");
			strcat(menuscript, buffer);
		}
		else
			strcat(menuscript, "guibutton \" not set\" \"joymousebutton -1; joymenu\"\n");
		for (int i=0; i < button_count; i++)
		{
			if (i+1 == _joymousebutton)
				sprintf(label, "[button %d]", i+1);
			else
				sprintf(label, " button %d ", i+1);
				
			sprintf(command, "joymousebutton %d; joymenu", i+1);
			sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
			strcat(menuscript, item);
		}
		strcat(menuscript, "]\n");
		execute(menuscript);
		
		// create the main joystick menu
		strcpy(menuscript, "newgui joystick [\n");
		strcat(menuscript, "guitext \"Joystick Configuration\"\n");
		strcat(menuscript, "guibar \n");
		
		sprintf(buffer, "guibutton \"look left/right\t[axis %d]\" \"showgui joyfovxaxis\"\n", _joyfovxaxis);
		strcat(menuscript, buffer);
		sprintf(buffer, "guibutton \"look up/down\t[axis %d]\" \"showgui joyfovyaxis\"\n", _joyfovyaxis);
		strcat(menuscript, buffer);
		strcat(menuscript, "guibar \n");
		strcat(menuscript, "guitext \"look sensitivity\"\n");
		strcat(menuscript, "guislider \"joysensitivity\"\n");
		strcat(menuscript, "guibar \n");
		strcat(menuscript, "guicheckbox \"invert look\" \"joyfovinvert\"\n");
		strcat(menuscript, "guicheckbox \"show events\" \"joyshowevents\"\n");
		strcat(menuscript, "guibar \n");
		if (_joymousebutton > 0)
		{
			sprintf(buffer, "guibutton \"emulate mouse \t[button %d]\" \"showgui joymousebutton\"\n", _joymousebutton);
			strcat(menuscript, buffer);
		}
		else
			strcat(menuscript, "guibutton \"emulate mouse\" \"showgui joymousebutton\"\n");
			
		strcat(menuscript, "guitab \"buttons\"\n");
		char keyname[50];
		for (int i=0; i < button_count; i++)
		{
			sprintf(keyname, "joybutton%d", i+1);
			char* action = get_keyaction_label(keyname);
			if (action != NULL && strlen(action) > 30)
				strcpy(action+25, "... ");
			sprintf(label, "button %d\t\t[%s]", i+1, (action == NULL) ? "" : action);
			sprintf(command, "showgui %s", keyname);
			sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
			strcat(menuscript, item);
			build_action_menu(keyname);
			if (action != NULL)
				delete action;
		}
		
		strcat(menuscript, "guitab \"pad\"\n");
		for (int i=0; i < hat_count; i++)
		{
			for (int j=1; j < 9; j++)
			{
				char hatpos[20];
				strcpy(hatpos, HAT_POS_NAME[j]);
				for(char *x = hatpos; *x; x++)
					*x = tolower(*x);
					
				sprintf(keyname, "joyhat%d%s", i+1, hatpos);
				char* action = get_keyaction_label(keyname);
				sprintf(label, "pad %d %s  \t[%s]", i+1, hatpos, (action == NULL) ? "" : action);
				sprintf(command, "showgui %s", keyname);
				sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
				strcat(menuscript, item);
				build_action_menu(keyname);
				if (action != NULL)
					delete action;
			}
		}
		
		strcat(menuscript, "guitab \"axis\"\n");
		strcat(menuscript, "guitext \"axis event sensitivity\"\n");
		strcat(menuscript, "guislider \"joyaxisminmove\"\n");
		strcat(menuscript, "guibar \n");
		char* minmax;
		for (int i=0; i < axis_count; i++)
		{
			for (int j=0; j < 2; j++)
			{
				if (j)
					minmax = "max";
				else
					minmax = "min";
				sprintf(keyname, "joyaxis%d%s", i+1, minmax);
				char* action = get_keyaction_label(keyname);
				sprintf(label, "axis %d %s\t[%s]", i+1, minmax, (action == NULL) ? "" : action);
				sprintf(command, "showgui %s", keyname);
				sprintf(item, "guibutton \"%s\" \"%s\"\n", label, command);
				strcat(menuscript, item);
				build_action_menu(keyname);
				if (action != NULL)
					delete action;
			}
		}
		
		strcat(menuscript, "]\n");
		execute(menuscript);
		
		showgui("joystick");
	}
	
	// if the returned pointer is not NULL, it should be deleted
	// by the calling method
	char* get_keyaction_label(char* keyname)
	{
		extern char* getkeyaction(char* key);
		char* action = getkeyaction(keyname);
		if (action != NULL)
		{
			action = newstring(action);
			if (strlen(action) > 30)
				strcpy(action+25, "... ");
		}
		return action;
	}
	
	void build_action_menu(char* key)
	{
		char item[200];
		char menuscript[5000];
		sprintf(menuscript, "newgui %s [\n", key);
		
		extern char* getkeyaction(char* key);
		char* curr_action = getkeyaction(key);
		if (!curr_action)
			curr_action = "";
			
		char command[50];
		sprintf(command, "bind %s []; joymenu", key);
		if (strcmp(curr_action, "") == 0 || strcmp(curr_action, " ") == 0)
			sprintf(item, "guibutton \"[none]\" \"%s\"\n", command);
		else
			sprintf(item, "guibutton \" none\" \"%s\"\n", command);
		strcat(menuscript, item);
		
		loopv(joyactions)
		add_action_menuitem(menuscript, key, joyactions[i]->label, joyactions[i]->action, curr_action);
		
		/*
		char buffer[1000];
		loopv(joysayactions)
		{
			sprintf(buffer, "[say \"%s\"]", joysayactions[i]);
			add_action_menuitem(menuscript, key, buffer, buffer, curr_action);
		}
		*/
		strcat(menuscript, "]\n");
		execute(menuscript);
	}
	
	void add_action_menuitem(char* menuscript, char* key, char* name, char* action, char* curr_action)
	{
		char label[500];
		
		if (strcmp(action, curr_action) == 0)
			sprintf(label, "[%s]", name);
		else
			sprintf(label, " %s", name);
			
		char item[1000];
		sprintf(item, "guibutton \"%s\" [ bind %s [%s]; joymenu ]\n", label, key, action);
		strcat(menuscript, item);
	}
	
	void add_joyaction(char* label, char* action)
	{
		joymenuaction*& maction = joyactions.add();
		maction = new joymenuaction();
		maction->label = newstring(label);
		maction->action = newstring(action);
	}
	
	void add_joysayaction(char* msg)
	{
		char*& newmsg = joysayactions.add();
		newmsg = newstring(msg);
	}
};

vector<joystick *> joysticks;

void initjoy()
{
	// initialize joystick support
	if (joysticks.length())
	{
		loopvrev(joysticks)
		{
			DELETEP(joysticks[i]);
			joysticks.remove(i);
		}
	}
	
	int num = SDL_NumJoysticks();
	loopi(num)
	{
		joysticks.add(new joystick(i));
	}
	conoutf("%d of %d joystick(s) enabled", joysticks.length(), SDL_NumJoysticks());
}

void processjoy(SDL_Event *event)
{
	loopv(joysticks)
	{
		joystick *js = joysticks[i];

		if (i < cl->localplayers() && js->enabled) js->process_event(event);
	}
}

void movejoy()
{
	loopv(joysticks)
	{
		joystick *js = joysticks[i];

		if (i < cl->localplayers() && js->enabled) js->move();
	}
}

void swapjoy(int *from, int *to)
{
	if ((*from >= 0 && *from < joysticks.length()) && (*to >= 0 && *to < joysticks.length()))
	{
		joystick *js[2] = { joysticks[*from], joysticks[*to] };
		joysticks[*from] = js[1];
		joysticks[*to] = js[0];
		conoutf("joystick #%d swapped with #%d", *from, *to);
	}
	else conoutf("valid joystick range is 0..%d", joysticks.length()-1);
}
COMMAND(swapjoy, "ii");
