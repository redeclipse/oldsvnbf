namespace auth
{
    ENetSocket socket = ENET_SOCKET_NULL;
    char input[4096];
    vector<char> output;
    int inputpos = 0, outputpos = 0;
    int lastconnect = 0, lastactivity = 0;
    uint nextauthreq = 1;

	extern void connect();
	extern void disconnect();

    void setmaster(clientinfo *ci, bool val, const char *pass = "", const char *authname = NULL)
	{
        if(authname && !val) return;
		const char *name = "";
		if(val)
		{
            bool haspass = adminpass[0] && checkpassword(ci, adminpass, pass);
			if(ci->privilege)
			{
				if(!adminpass[0] || haspass==(ci->privilege==PRIV_ADMIN)) return;
			}
            else if(ci->state.state==CS_SPECTATOR && !haspass && !authname && !haspriv(ci, PRIV_MASTER))
				return;
            loopv(clients) if(ci!=clients[i] && clients[i]->privilege)
			{
				if(haspass) clients[i]->privilege = PRIV_NONE;
                else if((authname || ci->local) && clients[i]->privilege<=PRIV_MASTER) continue;
				else return;
			}
            if(haspass) ci->privilege = PRIV_ADMIN;
            else if(!authname && !(mastermask&MM_AUTOAPPROVE) && !ci->privilege && !ci->local)
            {
                sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "This server requires you to use the \"/auth\" command to gain master.");
                return;
            }
            else
            {
                if(authname)
                {
                    loopv(clients) if(ci!=clients[i] && clients[i]->privilege<=PRIV_MASTER) clients[i]->privilege = PRIV_NONE;
					ci->privilege = PRIV_ADMIN;
                }
                else ci->privilege = PRIV_MASTER;
            }
            name = privname(ci->privilege);
		}
		else
		{
			if(!ci->privilege) return;
			name = privname(ci->privilege);
			ci->privilege = 0;
		}
		mastermode = MM_OPEN;
        allowedips.setsize(0);
        if(val && authname) srvoutf("%s claimed %s as '\fs\fc%s\fS'", colorname(ci), name, authname);
        else srvoutf("%s %s %s", colorname(ci), val ? "claimed" : "relinquished", name);
		currentmaster = val ? ci->clientnum : -1;
		masterupdate = true;
	}

    bool isconnected() { return socket != ENET_SOCKET_NULL; }

    void addoutput(const char *str)
    {
        int len = strlen(str);
        if(len + output.length() > 64*1024) return;
        output.put(str, len);
    }

    clientinfo *findauth(uint id)
    {
        loopv(clients) if(clients[i]->authreq == id) return clients[i];
        return NULL;
    }

    void authfailed(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = ci->authname[0] = 0;
		sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "authority request failed, please check your credentials");
    }

    void authsucceeded(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
		setmaster(ci, true, "", ci->authname);
    }

    void authchallenged(uint id, const char *val)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        sendf(ci->clientnum, 1, "riis", SV_AUTHCHAL, id, val);
    }

    void tryauth(clientinfo *ci, const char *user)
    {
		if(!isconnected())
		{
			sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "not connected to authentication server");
			return;
		}
		else if(ci->authreq)
		{
			sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "waiting for previous attempt..");
			return;
		}
        if(!nextauthreq) nextauthreq = 1;
        ci->authreq = nextauthreq++;
        filtertext(ci->authname, user, false, 100);
        s_sprintfd(buf)("reqauth %u %s\n", ci->authreq, ci->authname);
        addoutput(buf);
		sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "please wait, requesting credential match");
    }

    void answerchallenge(clientinfo *ci, uint id, char *val)
    {
        if(ci->authreq != id) return;
        for(char *s = val; *s; s++)
        {
            if(!isxdigit(*s)) { *s = '\0'; break; }
        }
        s_sprintfd(buf)("confauth %u %s\n", id, val);
        addoutput(buf);
    }

    void processinput()
    {
		if(inputpos < 0) return;
		const int MAXWORDS = 25;
		char *w[MAXWORDS];
		int numargs = MAXWORDS;
		const char *p = input;
		for(char *end;; p = end)
		{
			end = (char *)memchr(p, '\n', &input[inputpos] - p);
			if(!end) end = (char *)memchr(p, '\0', &input[inputpos] - p);
			if(!end) break;
			*end++ = '\0';

			//conoutf("{authserv} %s", p);
			loopi(MAXWORDS)
			{
				w[i] = (char *)"";
				if(i > numargs) continue;
				char *s = parsetext(p);
				if(s) w[i] = s;
				else numargs = i;
			}

			p += strcspn(p, ";\n\0"); p++;
			if(!strcmp(w[0], "error")) conoutf("authserv error: %s", w[1]);
			else if(!strcmp(w[0], "echo")) conoutf("authserv reply: %s", w[1]);
			else if(!strcmp(w[0], "failauth")) authfailed((uint)(atoi(w[1])));
			else if(!strcmp(w[0], "succauth")) authsucceeded((uint)(atoi(w[1])));
			else if(!strcmp(w[0], "chalauth")) authchallenged((uint)(atoi(w[1])), w[2]);
			else if(w[0]) conoutf("authserv sent invalid command: %s", w[0]);
			loopj(numargs) if(w[j]) delete[] w[j];
		}
		inputpos = &input[inputpos] - p;
		memmove(input, p, inputpos);
        if(inputpos >= (int)sizeof(input)) disconnect();
    }

	void regserver()
	{
		conoutf("updating authentication server");
		s_sprintfd(msg)("server %d %d\n", serverport, serverqueryport);
		addoutput(msg);
		lastactivity = lastmillis;
	}

    void connect()
    {
        if(socket != ENET_SOCKET_NULL) return;

        lastconnect = lastmillis;
		conoutf("connecting to authentication server %s:[%d]...", servermaster, servermasterport);
		ENetAddress authserver = { ENET_HOST_ANY, servermasterport };
		if(enet_address_set_host(&authserver, servermaster) < 0) conoutf("could not set authentication host");
		else
		{
			socket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
			if(socket != ENET_SOCKET_NULL)
			{
				if(enet_socket_connect(socket, &authserver) < 0)
				{
					enet_socket_destroy(socket);
					socket = ENET_SOCKET_NULL;
				}
				else enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
			}
			if(socket == ENET_SOCKET_NULL) conoutf("couldn't connect to authentication server");
			else
			{
				regserver();
				loopv(clients) clients[i]->authreq = clients[i]->authname[0] = 0;
			}
		}
    }

    void disconnect()
    {
        if(socket == ENET_SOCKET_NULL) return;

        enet_socket_destroy(socket);
        socket = ENET_SOCKET_NULL;

        output.setsizenodelete(0);
        outputpos = inputpos = 0;
       	conoutf("disconnected from authentication server");
    }

    void flushoutput()
    {
        if(output.empty()) return;

        ENetBuffer buf;
        buf.data = &output[outputpos];
        buf.dataLength = output.length() - outputpos;
        int sent = enet_socket_send(socket, NULL, &buf, 1);
        if(sent > 0)
        {
        	lastactivity = lastmillis;
            outputpos += sent;
            if(outputpos >= output.length())
            {
                output.setsizenodelete(0);
                outputpos = 0;
            }
        }
        else if(sent < 0) disconnect();
    }

    void flushinput()
    {
        enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
        if(enet_socket_wait(socket, &events, 0) < 0 || !events) return;

        ENetBuffer buf;
        buf.data = &input[inputpos];
        buf.dataLength = sizeof(input) - inputpos;
        int recv = enet_socket_receive(socket, NULL, &buf, 1);

        if(recv > 0)
        {
            inputpos += recv;
            processinput();
        }
        else if(recv < 0) disconnect();
    }

    void update()
    {
        if(!isconnected())
        {
			if(servertype >= 2 && (!lastconnect || lastmillis - lastconnect > 60*1000)) connect();
			if(!isconnected()) return;
        }
        else if(servertype < 2)
        {
        	disconnect();
        	return;
        }

        if(lastmillis - lastactivity > 10*60*1000) regserver();
        flushoutput();
        flushinput();
    }
}
