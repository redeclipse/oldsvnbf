#include "game.h"
namespace physics
{
    FVAR(IDF_WORLD, gravity,            0, 50.f, 1000);         // gravity
    FVAR(IDF_WORLD, liquidspeed,        0, 0.85f, 1);
    FVAR(IDF_WORLD, liquidcurb,         0, 10.f, 1000);
    FVAR(IDF_WORLD, floorcurb,          0, 5.f, 1000);
    FVAR(IDF_WORLD, aircurb,            0, 25.f, 1000);

    FVAR(IDF_WORLD, stairheight,        0, 4.1f, 1000);
    FVAR(IDF_WORLD, floorz,             0, 0.867f, 1);
    FVAR(IDF_WORLD, slopez,             0, 0.5f, 1);
    FVAR(IDF_WORLD, wallz,              0, 0.2f, 1);
    FVAR(IDF_WORLD, stepspeed,          1e-3f, 1.f, 1000);
    FVAR(IDF_WORLD, ladderspeed,        1e-3f, 1.f, 1000);

    FVAR(IDF_PERSIST, floatspeed,       1e-3f, 75, 1000);
    FVAR(IDF_PERSIST, floatcurb,        0, 1.f, 1000);

    FVAR(IDF_PERSIST, impulseroll,      0, 10, 90);
    FVAR(IDF_PERSIST, impulsereflect,   0, 155, 360);

    VAR(IDF_PERSIST, physframetime,     5, 5, 20);
    VAR(IDF_PERSIST, physinterp,        0, 1, 1);

    VAR(IDF_PERSIST, impulseaction,     0, 1, 2);               // determines if impulse remains active when pushed, 0 = off, 1 = only if no gravity or impulsestyle requires no ground contact, 2 = always
    VAR(IDF_PERSIST, impulsedash,       0, 1, 3);               // determines how impulsedash works, 0 = off, 1 = double jump, 2 = double tap, 3 = double jump only

    VAR(IDF_PERSIST, crouchstyle,       0, 1, 2);               // 0 = press and hold, 1 = double-tap toggle, 2 = toggle
    VAR(IDF_PERSIST, sprintstyle,       0, 4, 5);               // 0 = press and hold, 1 = double-tap toggle, 2 = toggle, 3-5 = inverted

    int physsteps = 0, lastphysframe = 0, lastmove = 0, lastdirmove = 0, laststrafe = 0, lastdirstrafe = 0, lastcrouch = 0, lastsprint = 0;

    #define imov(name,v,u,d,s,os) \
        void do##name(bool down) \
        { \
            game::player1->s = down; \
            int dir = game::player1->s ? d : (game::player1->os ? -(d) : 0); \
            game::player1->v = dir; \
            if(down) \
            { \
                if(impulsestyle && impulsedash == 2 && last##v && lastdir##v && dir == lastdir##v && lastmillis-last##v < PHYSMILLIS) \
                { \
                    game::player1->action[AC_DASH] = true; \
                    game::player1->actiontime[AC_DASH] = lastmillis; \
                } \
                last##v = lastmillis; lastdir##v = dir; \
                last##u = lastdir##u = 0; \
            } \
        } \
        ICOMMAND(0, name, "D", (int *down), { do##name(*down!=0); });

    imov(backward, move,   strafe,  -1, k_down,  k_up);
    imov(forward,  move,   strafe,   1, k_up,    k_down);
    imov(left,     strafe, move,     1, k_left,  k_right);
    imov(right,    strafe, move,    -1, k_right, k_left);

    // inputs
    void doaction(int type, bool down)
    {
        if(type < AC_TOTAL && type > -1)
        {
            if(game::allowmove(game::player1))
            {
                int style = 0, *last = NULL;
                switch(type)
                {
                    case AC_CROUCH: style = crouchstyle; last = &lastcrouch; break;
                    case AC_SPRINT: style = sprintstyle%3; last = &lastsprint; break;
                    default: break;
                }
                switch(style)
                {
                    case 1:
                    {
                        if(!down && game::player1->action[type])
                        {
                            if(*last && lastmillis-*last < PHYSMILLIS) return;
                            *last = lastmillis;
                        }
                        break;
                    }
                    case 2:
                    {
                        if(!down)
                        {
                            if(*last)
                            {
                                *last = 0;
                                return;
                            }
                            *last = lastmillis;
                        }
                        break;
                    }
                    default: break;
                }
                if(type == AC_CROUCH)
                {
                    if(game::player1->action[type] != down)
                    {
                        if(game::player1->actiontime[type] >= 0) game::player1->actiontime[type] = lastmillis-max(PHYSMILLIS-(lastmillis-game::player1->actiontime[type]), 0);
                        else if(down) game::player1->actiontime[type] = -game::player1->actiontime[type];
                    }
                }
                else if(down) game::player1->actiontime[type] = lastmillis;
                game::player1->action[type] = down;
            }
            else
            {
                game::player1->action[type] = false;
                if(type == AC_ATTACK && down) game::respawn(game::player1);
            }
        }
    }
    ICOMMAND(0, action, "Di", (int *n, int *i), { doaction(*i, *n!=0); });

    bool issolid(physent *d, physent *e, bool esc)
    {
        bool projectile = false, actor = false;
        if(e) switch(e->type)
        {
            case ENT_PLAYER: case ENT_AI: if(((gameent *)e)->aitype >= 0) actor = true; break;
            case ENT_PROJ:
            {
                projent *p = (projent *)e;
                if(p->hit == d || !(p->projcollide&HIT_PLAYER)) return false;
                if(p->owner == d && ((!returningfire && !(p->projcollide&COLLIDE_OWNER)) || (esc && !p->escaped))) return false;
                projectile = true;
                break;
            }
            default: break;
        }
        if(d->state == CS_ALIVE)
        {
            if(d->type == ENT_PLAYER || d->type == ENT_AI)
            {
                gameent *f = (gameent *)d;
                if(!projectile && !actor && f->aitype >= AI_START && !f->move && !f->strafe && !f->action[AC_CROUCH]) return false;
                if(f->protect(lastmillis, m_protect(game::gamemode, game::mutators))) return false;
            }
            return true;
        }
        return d->state == CS_DEAD || d->state == CS_WAITING;
    }

    bool iscrouching(physent *d)
    {
        if(d->type == ENT_PLAYER || d->type == ENT_AI)
        {
            gameent *e = (gameent *)d;
            return e->action[AC_CROUCH] || e->actiontime[AC_CROUCH] < 0 || lastmillis-e->actiontime[AC_CROUCH] <= PHYSMILLIS;
        }
        return false;
    }

    bool sprinting(physent *d, bool last, bool turn, bool move)
    {
        if(impulsestyle && (d->type == ENT_PLAYER || d->type == ENT_AI))
        {
            gameent *e = (gameent *)d;
            if(last && e->lastsprint && lastmillis-e->lastsprint <= PHYSMILLIS) return true;
            if(!iscrouching(d))
            {
                if(turn && e->turnside) return true;
                if((d != game::player1 && !e->ai) || !impulsemeter || e->impulse[IM_METER] < impulsemeter)
                {
                    bool value = e->action[AC_SPRINT];
                    if(d == game::player1 && sprintstyle >= 3) value = !value;
                    if(value && (!move || d->move || d->strafe)) return true;
                }
            }
        }
        return false;
    }

    bool liquidcheck(physent *d) { return d->inliquid && d->submerged > 0.8f; }

    float liquidmerge(physent *d, float from, float to)
    {
        if(d->inliquid)
        {
            if(d->physstate >= PHYS_SLIDE && d->submerged < 1.f)
                return from-((from-to)*d->submerged);
            else return to;
        }
        return from;
    }

    float jumpforce(physent *d, bool liquid) { return jumpspeed*(d->weight/100.f)*(liquid ? liquidmerge(d, 1.f, PHYS(liquidspeed)) : 1.f); }
    float impulseforce(physent *d) { return impulsespeed*(d->weight/100.f); }
    float gravityforce(physent *d) { return PHYS(gravity)*(d->weight/100.f); }

    float stepforce(physent *d, bool up)
    {
        if(up && d->onladder) return ladderspeed;
        if(d->physstate > PHYS_FALL) return stepspeed;
        return 1.f;
    }

    bool sticktofloor(physent *d)
    {
        if(!d->onladder && !liquidcheck(d) && (d->type == ENT_PLAYER || d->type == ENT_AI) && PHYS(gravity) > 0)
        {
            gameent *e = (gameent *)d;
            if(e->turnside || (e->lastpush && lastmillis-e->lastpush <= PHYSMILLIS/2) || (e->actiontime[AC_JUMP] && lastmillis-e->actiontime[AC_JUMP] <= PHYSMILLIS/2))
                return false;
            return true;
        }
        return false;
    }

    bool sticktospecial(physent *d)
    {
        if(d->onladder) return true;
        if(d->type == ENT_PLAYER || d->type == ENT_AI) { if(((gameent *)d)->turnside) return true; }
        return false;
    }

    bool canimpulse(physent *d, int cost)
    {
        if((d->type == ENT_PLAYER || d->type == ENT_AI) && impulsestyle)
        {
            gameent *e = (gameent *)d;
            if(impulsemeter && e->impulse[IM_METER]+(cost > 0 ? cost : impulsecost) > impulsemeter) return false;
            if(cost <= 0)
            {
                if(e->impulse[IM_TIME] && lastmillis-e->impulse[IM_TIME] <= PHYSMILLIS) return false;
                if(PHYS(gravity) > 0)
                {
                    if(impulsestyle <= 2 && e->impulse[IM_COUNT] >= impulsecount) return false;
                    if(cost == 0 && impulsestyle == 1 && e->impulse[IM_TYPE] > IM_T_NONE && e->impulse[IM_TYPE] < IM_T_WALL)
                        return false;
                }
            }
            return true;
        }
        return false;
    }

    float movevelocity(physent *d)
    {
        if(d->type == ENT_CAMERA) return game::player1->maxspeed*(game::player1->weight/100.f)*(floatspeed/100.0f);
        else if(d->type == ENT_PLAYER || d->type == ENT_AI)
        {
            if(d->state == CS_EDITING || d->state == CS_SPECTATOR) return d->maxspeed*(d->weight/100.f)*(floatspeed/100.0f);
            else
            {
                float speed = movespeed;
                if(iscrouching(d) || (d == game::player1 && game::inzoom())) speed *= movecrawl;
                if(physics::sprinting(d, false, false)) speed += impulsespeed*(d->move < 0 ? 0.5f : 1);
                return max(d->maxspeed,1.f)*(d->weight/100.f)*(speed/100.f);
            }
        }
        return max(d->maxspeed,1.f);
    }

    bool movepitch(physent *d)
    {
        if(d->type == ENT_CAMERA || d->state == CS_EDITING || d->state == CS_SPECTATOR) return true;
        if(d->state == CS_ALIVE && PHYS(gravity) <= 0 && d->physstate == PHYS_FALL) return true;
        return false;
    }

    void recalcdir(physent *d, const vec &oldvel, vec &dir)
    {
        float speed = oldvel.magnitude();
        if(speed > 1e-6f)
        {
            float step = dir.magnitude();
            dir = d->vel;
            dir.add(d->falling);
            dir.mul(step/speed);
        }
    }

    void slideagainst(physent *d, vec &dir, const vec &obstacle, bool foundfloor, bool slidecollide)
    {
        vec wall(obstacle);
        if(foundfloor ? wall.z > 0 : slidecollide)
        {
            wall.z = 0;
            if(!wall.iszero()) wall.normalize();
        }
        vec oldvel(d->vel);
        oldvel.add(d->falling);
        d->vel.project(wall);
        d->falling.project(wall);
        recalcdir(d, oldvel, dir);
    }

    void switchfloor(physent *d, vec &dir, const vec &floor)
    {
        if(floor.z >= floorz) d->falling = vec(0, 0, 0);

        vec oldvel(d->vel);
        oldvel.add(d->falling);
        if(dir.dot(floor) >= 0)
        {
            if(d->physstate < PHYS_SLIDE || fabs(dir.dot(d->floor)) > 0.01f*dir.magnitude()) return;
            d->vel.projectxy(floor, 0.0f);
        }
        else d->vel.projectxy(floor);
        d->falling.project(floor);
        recalcdir(d, oldvel, dir);
    }

    bool trystepup(physent *d, vec &dir, const vec &obstacle, float maxstep, const vec &floor)
    {
        vec old(d->o), stairdir = (obstacle.z >= 0 && obstacle.z < slopez ? vec(-obstacle.x, -obstacle.y, 0) : vec(dir.x, dir.y, 0)).rescale(1);
        float force = stepforce(d, true);
        if(d->onladder)
        {
            vec laddir = vec(stairdir).add(vec(0, 0, maxstep)).mul(0.1f*force);
            loopi(2)
            {
                d->o.add(laddir);
                if(collide(d))
                {
                    if(d->physstate == PHYS_FALL || d->floor != floor)
                    {
                        d->timeinair = 0;
                        d->floor = vec(0, 0, 1);
                        switchfloor(d, dir, d->floor);
                    }
                    d->physstate = PHYS_STEP_UP;
                    return true;
                }
                d->o = old; // try again, but only up
                laddir.x = laddir.y = 0;
            }
            return false;
        }
        bool cansmooth = true;
        d->o = old;
        /* check if there is space atop the stair to move to */
        if(d->physstate != PHYS_STEP_UP)
        {
            vec checkdir = stairdir;
            checkdir.mul(0.1f);
            checkdir.z += maxstep + 0.1f;
            checkdir.mul(force);
            d->o.add(checkdir);
            if(!collide(d))
            {
                d->o = old;
                if(collide(d, vec(0, 0, -1), slopez)) return false;
                cansmooth = false;
            }
        }

        if(cansmooth)
        {
            d->o = old;
            vec checkdir = stairdir;
            checkdir.z += 1;
            checkdir.mul(maxstep*force);
            d->o.add(checkdir);
            if(!collide(d, checkdir))
            {
                if(collide(d, vec(0, 0, -1), slopez))
                {
                    d->o = old;
                    return false;
                }
            }
            /* try stepping up half as much as forward */
            d->o = old;
            vec smoothdir = vec(dir.x, dir.y, 0).mul(force);
            float magxy = smoothdir.magnitude();
            if(magxy > 1e-9f)
            {
                if(magxy > 2*dir.z)
                {
                    smoothdir.mul(1/magxy);
                    smoothdir.z = 0.5f;
                    smoothdir.mul(dir.magnitude()*force/smoothdir.magnitude());
                }
                else smoothdir.z = dir.z;
                d->o.add(smoothdir);
                d->o.z += maxstep*force + 0.1f*force;
                if(collide(d, smoothdir))
                {
                    d->o.z -= maxstep*force + 0.1f*force;
                    if(d->physstate == PHYS_FALL || d->floor != floor)
                    {
                        d->timeinair = 0;
                        d->floor = floor;
                        switchfloor(d, dir, d->floor);
                    }
                    d->physstate = PHYS_STEP_UP;
                    return true;
                }
            }
        }

        /* try stepping up */
        d->o = old;
        d->o.z += dir.magnitude()*force;
        if(collide(d, vec(0, 0, 1)))
        {
            if(d->physstate == PHYS_FALL || d->floor != floor)
            {
                d->timeinair = 0;
                d->floor = floor;
                switchfloor(d, dir, d->floor);
            }
            if(cansmooth) d->physstate = PHYS_STEP_UP;
            return true;
        }
        d->o = old;
        return false;
    }

    bool trystepdown(physent *d, vec &dir, float step, float xy, float z, bool init = false)
    {
        vec stepdir(dir.x, dir.y, 0);
        stepdir.z = -stepdir.magnitude2()*z/xy;
        if(!stepdir.z) return false;
        stepdir.normalize();

        vec old(d->o);
        d->o.add(vec(stepdir).mul(stairheight/fabs(stepdir.z))).z -= stairheight;
        d->zmargin = -stairheight;
        if(!collide(d, vec(0, 0, -1), slopez))
        {
            d->o = old;
            d->o.add(vec(stepdir).mul(step));
            d->zmargin = 0;
            if(collide(d, vec(0, 0, -1)))
            {
                vec stepfloor(stepdir);
                stepfloor.mul(-stepfloor.z).z += 1;
                stepfloor.normalize();
                if(d->physstate >= PHYS_SLOPE && d->floor != stepfloor)
                {
                    // prevent alternating step-down/step-up states if player would keep bumping into the same floor
                    vec stepped(d->o);
                    d->o.z -= 0.5f;
                    d->zmargin = -0.5f;
                    if(!collide(d, stepdir) && wall == d->floor)
                    {
                        d->o = old;
                        if(!init) { d->o.x += dir.x; d->o.y += dir.y; if(dir.z <= 0 || !collide(d, dir)) d->o.z += dir.z; }
                        d->zmargin = 0;
                        d->physstate = PHYS_STEP_DOWN;
                        return true;
                    }
                    d->o = init ? old : stepped;
                    d->zmargin = 0;
                }
                else if(init) d->o = old;
                switchfloor(d, dir, stepfloor);
                d->floor = stepfloor;
                if(init)
                {
                    d->timeinair = 0;
                    d->physstate = PHYS_STEP_DOWN;
                }
                return true;
            }
        }
        d->o = old;
        d->zmargin = 0;
        return false;
    }

    bool trystepdown(physent *d, vec &dir, bool init = false)
    {
        if(!sticktofloor(d)) return false;
        float step = dir.magnitude();
        if(trystepdown(d, dir, step, 2, 1, init)) return true;
        if(trystepdown(d, dir, step, 1, 1, init)) return true;
        if(trystepdown(d, dir, step, 1, 2, init)) return true;
        return false;
    }

    void falling(physent *d, vec &dir, const vec &floor)
    {
        if(d->physstate >= PHYS_SLOPE && dir.dot(d->floor) <= 0.01f*dir.magnitude() && trystepdown(d, dir, true))
            return;

        if(floor.z > 0.0f && floor.z < slopez)
        {
            if(floor.z >= wallz) switchfloor(d, dir, floor);
            d->timeinair = 0;
            d->physstate = PHYS_SLIDE;
            d->floor = floor;
        }
        else if(sticktospecial(d))
        {
            d->timeinair = 0;
            d->physstate = PHYS_FLOOR;
            d->floor = vec(0, 0, 1);
        }
        else d->physstate = PHYS_FALL;
    }

    void landing(physent *d, vec &dir, const vec &floor, bool collided)
    {
        switchfloor(d, dir, floor);
        d->timeinair = 0;
        if((d->physstate!=PHYS_STEP_UP && d->physstate!=PHYS_STEP_DOWN) || !collided)
            d->physstate = floor.z >= floorz ? PHYS_FLOOR : PHYS_SLOPE;
        d->floor = floor;
    }

    bool findfloor(physent *d, bool collided, const vec &obstacle, bool &slide, vec &floor)
    {
        bool found = false;
        vec moved(d->o);
        d->o.z -= 0.1f;
        if(sticktospecial(d))
        {
            floor = vec(0, 0, 1);
            found = true;
        }
        else if(d->physstate != PHYS_FALL && !collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE || d->physstate == PHYS_STEP_DOWN ? slopez : floorz))
        {
            floor = wall;
            found = true;
        }
        else if(collided && obstacle.z >= slopez)
        {
            floor = obstacle;
            found = true;
            slide = false;
        }
        else if(d->physstate == PHYS_STEP_UP || d->physstate == PHYS_SLIDE)
        {
            if(!collide(d, vec(0, 0, -1)) && wall.z > 0.0f)
            {
                floor = wall;
                if(floor.z >= slopez) found = true;
            }
        }
        else if(d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f)
        {
            if(!collide(d, vec(d->floor).neg(), 0.95f) || !collide(d, vec(0, 0, -1)))
            {
                floor = wall;
                if(floor.z >= slopez && floor.z < 1.0f) found = true;
            }
        }
        if(collided && (!found || obstacle.z > floor.z))
        {
            floor = obstacle;
            slide = !found && (floor.z < wallz || floor.z >= slopez);
        }
        d->o = moved;
        return found;
    }

    bool move(physent *d, vec &dir)
    {
        vec old(d->o), obstacle; d->o.add(dir);
        bool collided = false, slidecollide = false;
        if(!collide(d, dir))
        {
            obstacle = wall;
            /* check to see if there is an obstacle that would prevent this one from being used as a floor */
            if((d->type==ENT_PLAYER || d->type==ENT_AI) && ((wall.z>=slopez && dir.z<0) || (wall.z<=-slopez && dir.z>0)) && (dir.x || dir.y) && !collide(d, vec(dir.x, dir.y, 0)))
            {
                if(wall.dot(dir) >= 0) slidecollide = true;
                obstacle = wall;
            }

            d->o = old;
            d->o.z -= stairheight;
            d->zmargin = -stairheight;
            if(d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR  || (!collide(d, vec(0, 0, -1), slopez) && (d->physstate == PHYS_STEP_UP || d->physstate == PHYS_STEP_DOWN || wall.z >= floorz || d->onladder)))
            {
                d->o = old;
                d->zmargin = 0;
                if(trystepup(d, dir, obstacle, stairheight, d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR || d->onladder ? d->floor : vec(wall)))
                    return true;
            }
            else
            {
                d->o = old;
                d->zmargin = 0;
            }
            collided = true; // can't step over the obstacle, so just slide against it
        }
        else if(d->physstate == PHYS_STEP_UP || d->onladder)
        {
            if(!collide(d, vec(0, 0, -1), slopez))
            {
                d->o = old;
                if(trystepup(d, dir, vec(0, 0, 1), stairheight, d->onladder ? d->floor : vec(wall))) return true;
                d->o.add(dir);
            }
        }
        else if((d->type == ENT_PLAYER || d->type == ENT_AI) && d->physstate == PHYS_STEP_DOWN && dir.dot(d->floor) <= 1e-6f)
        {
            vec moved(d->o);
            d->o = old;
            if(trystepdown(d, dir)) return true;
            d->o = moved;
        }
        vec floor(0, 0, 0);
        bool slide = collided, found = findfloor(d, collided, obstacle, slide, floor);
        if(slide || (!collided && floor.z > 0 && floor.z < wallz))
        {
            slideagainst(d, dir, slide ? obstacle : floor, found, slidecollide);
            d->blocked = true;
        }
        if(found) landing(d, dir, floor, collided);
        else falling(d, dir, floor);
        return !collided;
    }

    void modifyinput(gameent *d, vec &m, bool wantsmove, bool floating, int millis)
    {
        if(floating)
        {
            if(game::allowmove(d) && d->action[AC_JUMP]) d->vel.z += jumpforce(d, false);
        }
        else if(game::allowmove(d))
        {
            bool onfloor = d->physstate >= PHYS_SLOPE || d->onladder || liquidcheck(d);
            if(millis && impulsestyle)
            {
                if(sprinting(d) && canimpulse(d, millis))
                {
                    d->impulse[IM_METER] += millis;
                    d->lastsprint = lastmillis;
                }
                else if(d->impulse[IM_METER] > 0 && impulseregen > 0)
                {
                    int timeslice = max(int(millis*impulseregen), 1);
                    if(iscrouching(d)) timeslice += timeslice;
                    if(d->move || d->strafe) timeslice -= timeslice/2;
                    if(d->physstate == PHYS_FALL && !d->onladder) timeslice -= timeslice/2;
                    if((d->impulse[IM_METER] -= timeslice) < 0) d->impulse[IM_METER] = 0;
                }
            }

            if(d->turnside && (!impulsestyle || d->impulse[IM_TYPE] != IM_T_SKATE || lastmillis-d->impulse[IM_TIME] > impulseskate))
            {
                d->turnside = 0;
                d->resetphys();
            }

            if(impulsestyle && (d->ai || (impulsedash > 0 && impulsedash < 3)) && canimpulse(d) && (d->move || d->strafe) && (!d->ai && impulsedash == 2 ? d->action[AC_DASH] : d->action[AC_JUMP] && !onfloor))
            {
                float mag = impulseforce(d)+max(d->vel.magnitude(), 1.f);
                vecfromyawpitch(d->aimyaw, !d->ai && impulsedash == 2 ? max(d->aimpitch, 10.f) : d->aimpitch, d->move, d->strafe, d->vel);
                d->vel.normalize().mul(mag); d->vel.z += mag/4;
                d->doimpulse(impulsecost, IM_T_DASH, lastmillis);
                playsound(S_IMPULSE, d->o, d); game::impulseeffect(d, true);
                client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_IMPULSE);
            }
            if(!d->turnside && onfloor && d->action[AC_JUMP])
            {
                d->vel.z += jumpforce(d, true);
                if(d->inliquid)
                {
                    float scale = liquidmerge(d, 1.f, PHYS(liquidspeed));
                    d->vel.x *= scale;
                    d->vel.y *= scale;
                }
                d->action[AC_JUMP] = false;
                d->resetphys();
                d->lastjump = lastmillis;
                playsound(S_JUMP, d->o, d);
                regularshape(PART_SMOKE, int(d->radius), 0x111111, 21, 20, 150, d->feetpos(), 1, 1, -10, 0, 10.f);
                client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_JUMP);
            }
            if(!d->turnside && !onfloor && d->action[AC_JUMP] && impulsestyle && !d->turnside && !onfloor && canimpulse(d))
            {
                d->vel.z += impulseforce(d)*1.5f;
                d->doimpulse(impulsecost, IM_T_BOOST, lastmillis);
                playsound(S_IMPULSE, d->o, d);
                game::impulseeffect(d, true);
                client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_IMPULSE);
            }
            if(impulsestyle && (d->turnside || d->action[AC_SPECIAL]) && !d->inliquid && !d->onladder)
            {
                loopi(d->turnside ? 4 : 2)
                {
                    vec oldpos = d->o, dir;
                    int move = i%2 ? -1 : 1, strafe = i >= 2 ? d->turnside : d->strafe;
                    vecfromyawpitch(d->aimyaw, 0, move, strafe, dir);
                    d->o.add(vec(dir).mul(d->radius));
                    if(collide(d, dir) || (!hitplayer && wall.iszero()))
                    {
                        d->o = oldpos;
                        if(i >= (d->turnside ? 3 : 0)) { if(d->turnside) { d->turnside = 0; d->resetphys(); } break; }
                        continue;
                    }
                    d->o = oldpos;
                    if(hitplayer) wall = vec(hitplayer->o).sub(d->o);
                    wall.normalize();
                    float yaw = 0, pitch = 0;
                    vectoyawpitch(wall, yaw, pitch);
                    float off = yaw-d->aimyaw;
                    if(off > 180) off -= 360;
                    else if(off < -180) off += 360;
                    int key = (d->action[AC_JUMP] && d->turnside) ? AC_JUMP : ((hitplayer && (d->action[AC_SPECIAL] || d->turnside)) || ((d->action[AC_SPECIAL] && !d->turnside && !onfloor && fabs(off) >= impulsereflect && canimpulse(d, -1))) ? AC_SPECIAL : -1);
                    if(key >= 0)
                    {
                        float mag = (impulseforce(d)+max(d->vel.magnitude(), 1.f))/2;
                        d->vel = vec(d->turnside ? wall : vec(dir).reflect(wall)).add(vec(d->vel).reflect(wall).rescale(1)).mul(mag/2);
                        d->vel.z += d->turnside || hitplayer ? mag : mag/2;
                        d->doimpulse(impulsecost, IM_T_KICK, lastmillis);
                        d->action[key] = false;
                        if(hitplayer) weapons::doshot(d, hitplayer->o, WEAP_MELEE, true, true);
                        else
                        {
                            vectoyawpitch(d->vel, yaw, pitch);
                            off = yaw-d->aimyaw;
                            if(off > 180) off -= 360;
                            else if(off < -180) off += 360;
                            d->turnmillis = PHYSMILLIS;
                            d->turnside = (off < 0 ? -1 : 1)*(move ? move : 1);
                            d->turnyaw = off;
                            d->turnroll = 0;
                            playsound(S_IMPULSE, d->o, d);
                            game::impulseeffect(d, true);
                            client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_IMPULSE);
                        }
                    }
                    else if(d->turnside || (!onfloor && d->action[AC_SPECIAL] && canimpulse(d, -1)))
                    {
                        if(off < 0) yaw += 90;
                        else yaw -= 90;
                        while(yaw >= 360) yaw -= 360;
                        while(yaw < 0) yaw += 360;
                        vec rft; vecfromyawpitch(yaw, 0, 1, 0, rft);
                        if(!d->turnside)
                        {
                            float mag = max(d->vel.magnitude(), 3.f);
                            d->vel = vec(rft).mul(mag);
                            off = yaw-d->aimyaw;
                            if(off > 180) off -= 360;
                            else if(off < -180) off += 360;
                            d->doimpulse(impulsecost, IM_T_SKATE, lastmillis);
                            d->action[AC_SPECIAL] = false;
                            d->turnmillis = PHYSMILLIS;
                            d->turnside = (off < 0 ? -1 : 1)*(move ? move : 1);
                            d->turnyaw = off;
                            d->turnroll = (impulseroll*d->turnside)-d->roll;
                        }
                        else if(d->vel.magnitude() >= 3) m = rft; // re-project and override
                        else { d->turnside = 0; d->resetphys(); break; }
                    }
                    break;
                }
            }
            else if(d->turnside) { d->turnside = 0; d->resetphys(); }
        }

        if(d->action[AC_JUMP] && impulseaction < (PHYS(gravity) > 0 && impulsestyle < 2 ? 2 : 1))
            d->action[AC_JUMP] = false;
        d->action[AC_DASH] = false;

        if((d->physstate == PHYS_FALL && !d->onladder) || d->turnside)
            d->timeinair += millis;
        else d->dojumpreset(true);
    }

    void modifyvelocity(physent *pl, bool local, bool floating, int millis)
    {
        vec m(0, 0, 0);
        bool wantsmove = game::allowmove(pl) && (pl->move || pl->strafe);
        if(wantsmove)
        {
            vecfromyawpitch(pl->aimyaw, floating || (pl->inliquid && (liquidcheck(pl) || pl->aimpitch < 0.f)) || movepitch(pl) ? pl->aimpitch : 0, pl->move, pl->strafe, m);
            if((pl->type == ENT_PLAYER || pl->type == ENT_AI) && !floating && pl->physstate >= PHYS_SLOPE)
            { // move up or down slopes in air but only move up slopes in liquid
                float dz = -(m.x*pl->floor.x + m.y*pl->floor.y)/pl->floor.z;
                m.z = liquidcheck(pl) ? max(m.z, dz) : dz;
            }
            m.normalize();
        }
        if(local && (pl->type == ENT_PLAYER || pl->type == ENT_AI)) modifyinput((gameent *)pl, m, wantsmove, floating, millis);
        else if(pl->physstate == PHYS_FALL && !pl->onladder) pl->timeinair += millis;
        else pl->timeinair = 0;
        vec d = vec(m).mul(movevelocity(pl));
        if((pl->type == ENT_PLAYER || pl->type == ENT_AI) && !floating && !pl->inliquid)
            d.mul(pl->move || pl->strafe || pl->physstate == PHYS_FALL || pl->physstate == PHYS_STEP_DOWN ? (pl->strafe || pl->move <= 0 ? 1.25f : 1.5f) : 1.0f);
        if(floating || pl->type==ENT_CAMERA) pl->vel.lerp(d, pl->vel, pow(max(1.0f - 1.0f/floatcurb, 0.0f), millis/20.0f));
        else
        {
            bool floor = pl->physstate >= PHYS_SLOPE;
            if(floor && sprinting(pl, true)) floor = false;
            float curb = floor ? PHYS(floorcurb) : PHYS(aircurb), fric = pl->inliquid ? liquidmerge(pl, curb, PHYS(liquidcurb)) : curb;
            pl->vel.lerp(d, pl->vel, pow(max(1.0f - 1.0f/fric, 0.0f), millis/20.0f));
        }
    }

    void modifygravity(physent *pl, int curtime)
    {
        float secs = curtime/1000.0f;
        vec g(0, 0, 0);
        if(PHYS(gravity) > 0)
        {
            if(pl->physstate == PHYS_FALL) g.z -= gravityforce(pl)*secs;
            else if(pl->floor.z > 0 && pl->floor.z < floorz)
            {
                g.z = -1;
                g.project(pl->floor);
                g.normalize();
                g.mul(gravityforce(pl)*secs);
            }
            if(!liquidcheck(pl) || (!pl->move && !pl->strafe)) pl->falling.add(g);
        }
        else pl->falling = g;
        if(liquidcheck(pl) || pl->physstate >= PHYS_SLOPE)
        {
            float fric = liquidcheck(pl) ? liquidmerge(pl, PHYS(aircurb), PHYS(liquidcurb)) : PHYS(floorcurb),
                  c = liquidcheck(pl) ? 1.0f : clamp((pl->floor.z - slopez)/(floorz-slopez), 0.0f, 1.0f);
            pl->falling.mul(pow(max(1.0f - c/fric, 0.0f), curtime/20.0f));
        }
    }

    void updatematerial(physent *pl, const vec &center, float radius, const vec &bottom, bool local, bool floating)
    {
        int matid = lookupmaterial(bottom), curmat = matid&MATF_VOLUME, flagmat = matid&MATF_FLAGS,
            oldmat = pl->inmaterial&MATF_VOLUME;

        if(!floating && curmat != oldmat)
        {
            #define mattrig(mo,mcol,ms,mt,mz,mq,mp,mw) \
            { \
                int col = (int(mcol[2]*mq) + (int(mcol[1]*mq) << 8) + (int(mcol[0]*mq) << 16)); \
                regularshape(mp, mt, col, 21, 20, mz, mo, ms, 1, 10, 0, 20); \
                if(mw >= 0) playsound(mw, mo, pl); \
            }
            if(curmat == MAT_WATER || oldmat == MAT_WATER)
                mattrig(bottom, watercol, 0.5f, int(radius), PHYSMILLIS, 0.25f, PART_SPARK, curmat != MAT_WATER ? S_SPLASH2 : S_SPLASH1);
            if(curmat == MAT_LAVA) mattrig(vec(bottom).add(vec(0, 0, radius)), lavacol, 2.f, int(radius), PHYSMILLIS*2, 1.f, PART_FIREBALL, S_BURNING);
        }
        if(local && (pl->type == ENT_PLAYER || pl->type == ENT_AI) && pl->state == CS_ALIVE && flagmat == MAT_DEATH)
            game::suicide((gameent *)pl, (curmat == MAT_LAVA ? HIT_MELT : (curmat == MAT_WATER ? HIT_WATER : HIT_DEATH))|HIT_FULL);
        pl->inmaterial = matid;
        if((pl->inliquid = !floating && isliquid(curmat)) != false)
        {
            float frac = float(center.z-bottom.z)/10.f, sub = pl->submerged;
            vec tmp = bottom;
            int found = 0;
            loopi(10)
            {
                tmp.z += frac;
                if(!isliquid(lookupmaterial(tmp)&MATF_VOLUME))
                {
                    found = i+1;
                    break;
                }
            }
            pl->submerged = found ? found/10.f : 1.f;
            if(local)
            {
                if(curmat == MAT_WATER && (pl->type == ENT_PLAYER || pl->type == ENT_AI) && pl->submerged >= 0.25f && ((gameent *)pl)->onfire(lastmillis, fireburntime))
                {
                    gameent *d = (gameent *)pl;
                    d->resetfire();
                    playsound(S_EXTINGUISH, d->o, d, 0, d != game::focus ? 128 : 224, -1, -1);
                    client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_EXTINGUISH);
                }
                if(pl->physstate < PHYS_SLIDE && sub >= 0.5f && pl->submerged < 0.5f && pl->vel.z > 1e-16f)
                    pl->vel.z = max(pl->vel.z, max(jumpforce(pl, false), max(gravityforce(pl), 50.f)));
            }
        }
        else pl->submerged = 0;
        pl->onladder = !floating && flagmat == MAT_LADDER;
        if(pl->onladder && pl->physstate < PHYS_SLIDE) pl->floor = vec(0, 0, 1);
    }

    void updatematerial(physent *pl, bool local, bool floating)
    {
        updatematerial(pl, pl->o, pl->height/2.f, (pl->type == ENT_PLAYER || pl->type == ENT_AI) ? pl->feetpos(1) : pl->o, local, floating);
    }

    void updateragdoll(dynent *d, const vec &center, float radius)
    {
        vec bottom(center);
        bottom.z -= radius/2.f;
        updatematerial(d, center, radius, bottom, false, false);
    }

    // main physics routine, moves a player/monster for a time step
    // moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
    // local is false for multiplayer prediction

    bool moveplayer(physent *pl, int moveres, bool local, int millis)
    {
        bool floating = pl->type == ENT_CAMERA || (pl->type == ENT_PLAYER && pl->state == CS_EDITING);
        float secs = millis/1000.f;

        pl->blocked = false;
        if(pl->type==ENT_PLAYER || pl->type==ENT_AI)
        {
            updatematerial(pl, local, floating);
            if(!floating && !sticktospecial(pl)) modifygravity(pl, millis); // apply gravity
        }
        modifyvelocity(pl, local, floating, millis); // apply any player generated changes in velocity

        vec d(pl->vel);
        if((pl->type==ENT_PLAYER || pl->type==ENT_AI) && !floating && pl->inliquid) d.mul(liquidmerge(pl, 1.f, PHYS(liquidspeed)));
        d.add(pl->falling);
        d.mul(secs);

        if(floating)                // just apply velocity
        {
            if(pl->physstate != PHYS_FLOAT)
            {
                pl->physstate = PHYS_FLOAT;
                pl->timeinair = 0;
                pl->falling = vec(0, 0, 0);
            }
            pl->o.add(d);
        }
        else                        // apply velocity with collision
        {
            const float f = 1.0f/moveres;
            int collisions = 0, timeinair = pl->timeinair;
            vec vel(pl->vel);

            d.mul(f);
            loopi(moveres) if(!move(pl, d)) { if(++collisions<5) i--; } // discrete steps collision detection & sliding
            if(pl->type == ENT_PLAYER && !pl->timeinair && timeinair > PHYSMILLIS*4) // if we land after long time must have been a high jump, make thud sound
                playsound(S_LAND, pl->o, pl);
        }

        if(pl->type == ENT_PLAYER || pl->type == ENT_AI)
        {
            if(pl->state == CS_ALIVE) updatedynentcache(pl);
            if(local)
            {
                gameent *d = (gameent *)pl;
                if(d->state == CS_ALIVE)
                {
                    if(d->o.z < 0)
                    {
                        game::suicide(d, HIT_DEATH|HIT_FULL);
                        return false;
                    }
                    if(d->turnmillis > 0)
                    {
                        float amt = float(millis)/float(PHYSMILLIS), yaw = d->turnyaw*amt, roll = d->turnroll*amt;
                        if(yaw != 0) { d->aimyaw += yaw; d->yaw += yaw; }
                        if(roll != 0) d->roll += roll;
                        d->turnmillis -= millis;
                    }
                    else
                    {
                        d->turnmillis = 0;
                        if(d->roll != 0 && !d->turnside) adjustscaled(float, d->roll, PHYSMILLIS);
                    }
                }
                else
                {
                    d->turnmillis = d->turnside = 0;
                    d->roll = 0;
                }
            }
        }

        return true;
    }

    bool movecamera(physent *pl, const vec &dir, float dist, float stepdist)
    {
        int steps = (int)ceil(dist/stepdist);
        if(steps <= 0) return true;

        vec d(dir);
        d.mul(dist/steps);
        loopi(steps)
        {
            vec oldpos(pl->o);
            pl->o.add(d);
            if(!collide(pl, vec(0, 0, 0), 0, false))
            {
                pl->o = oldpos;
                return false;
            }
        }
        return true;
    }

    void interppos(physent *d)
    {
        d->o = d->newpos;
        d->o.z += d->height;

        int diff = lastphysframe - lastmillis;
        if(diff <= 0 || !physinterp) return;

        vec deltapos(d->deltapos);
        deltapos.mul(min(diff, physframetime)/float(physframetime));
        d->o.add(deltapos);
    }

    void move(physent *d, int moveres, bool local)
    {
        if(physsteps <= 0)
        {
            if(local) interppos(d);
            return;
        }

        if(local)
        {
            d->o = d->newpos;
            d->o.z += d->height;
        }
        loopi(physsteps-1) moveplayer(d, moveres, local, physframetime);
        if(local) d->deltapos = d->o;
        moveplayer(d, moveres, local, physframetime);
        if(local)
        {
            d->newpos = d->o;
            d->deltapos.sub(d->newpos);
            d->newpos.z -= d->height;
            interppos(d);
        }
    }

    void avoidcollision(physent *d, const vec &dir, physent *obstacle, float space)
    {
        float rad = obstacle->radius+d->radius;
        vec bbmin(obstacle->o);
        bbmin.x -= rad;
        bbmin.y -= rad;
        bbmin.z -= obstacle->height+d->aboveeye;
        bbmin.sub(space);
        vec bbmax(obstacle->o);
        bbmax.x += rad;
        bbmax.y += rad;
        bbmax.z += obstacle->aboveeye+d->height;
        bbmax.add(space);

        loopi(3) if(d->o[i] <= bbmin[i] || d->o[i] >= bbmax[i]) return;

        float mindist = 1e16f;
        loopi(3) if(dir[i] != 0)
        {
            float dist = ((dir[i] > 0 ? bbmax[i] : bbmin[i]) - d->o[i]) / dir[i];
            mindist = min(mindist, dist);
        }
        if(mindist >= 0.0f && mindist < 1e15f) d->o.add(vec(dir).mul(mindist));
    }

    void updatephysstate(physent *d)
    {
        if(d->physstate == PHYS_FALL && !d->onladder) return;
        d->timeinair = 0;
        vec old(d->o);
        /* Attempt to reconstruct the floor state.
         * May be inaccurate since movement collisions are not considered.
         * If good floor is not found, just keep the old floor and hope it's correct enough.
         */
        bool foundfloor = false;
        switch(d->physstate)
        {
            case PHYS_SLOPE:
            case PHYS_FLOOR:
            case PHYS_STEP_DOWN:
                d->o.z -= 0.15f;
                if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE || d->physstate == PHYS_STEP_DOWN ? slopez : floorz))
                {
                    d->floor = wall;
                    foundfloor = true;
                }
                break;

            case PHYS_STEP_UP:
                d->o.z -= stairheight+0.15f;
                if(!collide(d, vec(0, 0, -1), slopez))
                {
                    d->floor = wall;
                    foundfloor = true;
                }
                break;

            case PHYS_SLIDE:
                d->o.z -= 0.15f;
                if(!collide(d, vec(0, 0, -1)) && wall.z < slopez)
                {
                    d->floor = wall;
                    foundfloor = true;
                }
                break;
            default: break;
        }
        if((d->physstate > PHYS_FALL && d->floor.z <= 0) || (d->onladder && !foundfloor)) d->floor = vec(0, 0, 1);
        d->o = old;
    }

    bool xcollide(physent *d, const vec &dir, physent *o)
    {
        hitflags = HITFLAG_NONE;
        if(d->type == ENT_PROJ && (o->type == ENT_PLAYER || (o->type == ENT_AI && (!isaitype(((gameent *)o)->aitype) || aistyle[((gameent *)o)->aitype].maxspeed))))
        {
            gameent *e = (gameent *)o;
            if(!d->o.reject(e->legs, d->radius+max(e->lrad.x, e->lrad.y)) && !ellipsecollide(d, dir, e->legs, vec(0, 0, 0), e->yaw, e->lrad.x, e->lrad.y, e->lrad.z, e->lrad.z))
                hitflags |= HITFLAG_LEGS;
            if(!d->o.reject(e->torso, d->radius+max(e->trad.x, e->trad.y)) && !ellipsecollide(d, dir, e->torso, vec(0, 0, 0), e->yaw, e->trad.x, e->trad.y, e->trad.z, e->trad.z))
                hitflags |= HITFLAG_TORSO;
            if(!d->o.reject(e->head, d->radius+max(e->hrad.x, e->hrad.y)) && !ellipsecollide(d, dir, e->head, vec(0, 0, 0), e->yaw, e->hrad.x, e->hrad.y, e->hrad.z, e->hrad.z))
                hitflags |= HITFLAG_HEAD;
            return hitflags == HITFLAG_NONE;
        }
        if(!plcollide(d, dir, o))
        {
            hitflags |= HITFLAG_TORSO;
            return false;
        }
        return true;
    }

    bool xtracecollide(physent *d, const vec &from, const vec &to, float x1, float x2, float y1, float y2, float maxdist, float &dist, physent *o)
    {
        hitflags = HITFLAG_NONE;
        if(d && d->type == ENT_PROJ && (o->type == ENT_PLAYER || (o->type == ENT_AI && (!isaitype(((gameent *)o)->aitype) || aistyle[((gameent *)o)->aitype].maxspeed))))
        {
            gameent *e = (gameent *)o;
            float bestdist = 1e16f;
            if(e->legs.x+e->lrad.x >= x1 && e->legs.y+e->lrad.y >= y1 && e->legs.x-e->lrad.x <= x2 && e->legs.y-e->lrad.y <= y2)
            {
                vec bottom(e->legs), top(e->legs); bottom.z -= e->lrad.z; top.z += e->lrad.z; float d = 1e16f;
                if(linecylinderintersect(from, to, bottom, top, max(e->lrad.x, e->lrad.y), d)) { hitflags |= HITFLAG_LEGS; bestdist = min(bestdist, d); }
            }
            if(e->torso.x+e->trad.x >= x1 && e->torso.y+e->trad.y >= y1 && e->torso.x-e->trad.x <= x2 && e->torso.y-e->trad.y <= y2)
            {
                vec bottom(e->torso), top(e->torso); bottom.z -= e->trad.z; top.z += e->trad.z; float d = 1e16f;
                if(linecylinderintersect(from, to, bottom, top, max(e->trad.x, e->trad.y), d)) { hitflags |= HITFLAG_TORSO; bestdist = min(bestdist, d); }
            }
            if(e->head.x+e->hrad.x >= x1 && e->head.y+e->hrad.y >= y1 && e->head.x-e->hrad.x <= x2 && e->head.y-e->hrad.y <= y2)
            {
                vec bottom(e->head), top(e->head); bottom.z -= e->hrad.z; top.z += e->hrad.z; float d = 1e16f;
                if(linecylinderintersect(from, to, bottom, top, max(e->hrad.x, e->hrad.y), d)) { hitflags |= HITFLAG_HEAD; bestdist = min(bestdist, d); }
            }
            if(hitflags == HITFLAG_NONE) return true;
            dist = bestdist*from.dist(to);
            return false;
        }
        if(o->o.x+o->radius >= x1 && o->o.y+o->radius >= y1 && o->o.x-o->radius <= x2 && o->o.y-o->radius <= y2 && intersect(o, from, to, dist))
        {
            hitflags |= HITFLAG_TORSO;
            return false;
        }
        return true;
    }

    void complexboundbox(physent *d)
    {
        render3dbox(d->o, d->height, d->aboveeye, d->radius);
        renderellipse(d->o, d->xradius, d->yradius, d->yaw);
        if(d->type == ENT_PLAYER || (d->type == ENT_AI && (!isaitype(((gameent *)d)->aitype) || aistyle[((gameent *)d)->aitype].maxspeed)))
        {
            gameent *e = (gameent *)d;
            render3dbox(e->head, e->hrad.z, e->hrad.z, max(e->hrad.x, e->hrad.y));
            renderellipse(e->head, e->hrad.x, e->hrad.y, e->yaw);
            render3dbox(e->torso, e->trad.z, e->trad.z, max(e->trad.x, e->trad.y));
            renderellipse(e->torso, e->trad.x, e->trad.y, e->yaw);
            render3dbox(e->legs, e->lrad.z, e->lrad.z, max(e->lrad.x, e->lrad.y));
            renderellipse(e->legs, e->lrad.x, e->lrad.y, e->yaw);
            render3dbox(e->waist, 0.25f, 0.25f, 0.25f);
            render3dbox(e->lfoot, 1, 1, 1); render3dbox(e->rfoot, 1, 1, 1);
        }
    }

    bool entinmap(physent *d, bool avoidplayers)
    {
        if(d->state != CS_ALIVE) { d->resetinterp(); return insideworld(d->o); }
        vec orig = d->o;
        #define inmapchk(x,y) \
        { \
            loopi(x) \
            { \
                if(i) { y; } \
                if(collide(d) && !inside && (!avoidplayers || !hitplayer)) { d->resetinterp(); return true; } \
                d->o = orig; \
            } \
        }
        if(d->type == ENT_PLAYER || (d->type == ENT_AI && (!isaitype(((gameent *)d)->aitype) || aistyle[((gameent *)d)->aitype].maxspeed)))
        {
            vec dir; vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
            inmapchk(100, d->o.add(vec(dir).mul(i/10.f)));
        }
        inmapchk(100, d->o.add(vec((rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f)));
        d->o = orig; d->resetinterp();
        return false;
    }

    VAR(IDF_PERSIST, smoothmove, 0, 90, 1000);
    VAR(IDF_PERSIST, smoothdist, 0, 64, 1024);

    void predictplayer(gameent *d, bool domove, int res = 0, bool local = false)
    {
        d->o = d->newpos;
        d->o.z += d->height;

        d->yaw = d->newyaw;
        d->pitch = d->newpitch;

        d->aimyaw = d->newaimyaw;
        d->aimpitch = d->newaimpitch;

        if(domove)
        {
            move(d, res, local);
            d->newpos = d->o;
            d->newpos.z -= d->height;
        }

        float k = 1.0f - float(lastmillis - d->smoothmillis)/float(smoothmove);
        if(k>0)
        {
            d->o.add(vec(d->deltapos).mul(k));

            d->yaw += d->deltayaw*k;
            if(d->yaw<0) d->yaw += 360;
            else if(d->yaw>=360) d->yaw -= 360;
            d->pitch += d->deltapitch*k;

            d->aimyaw += d->deltaaimyaw*k;
            if(d->aimyaw<0) d->aimyaw += 360;
            else if(d->aimyaw>=360) d->aimyaw -= 360;
            d->aimpitch += d->deltaaimpitch*k;
        }
    }

    void smoothplayer(gameent *d, int res, bool local)
    {
        if(d->state == CS_ALIVE || d->state == CS_EDITING)
        {
            if(smoothmove && d->smoothmillis>0) predictplayer(d, true, res, local);
            else move(d, res, local);
        }
        else if(d->state==CS_DEAD || d->state == CS_WAITING)
        {
            if(d->ragdoll) moveragdoll(d, true);
            else if(lastmillis-d->lastpain<2000) move(d, res, local);
        }
    }

    bool droptofloor(vec &o, float radius, float height)
    {
        static struct dropent : physent
        {
            dropent()
            {
                type = ENT_DUMMY;
                collidetype = COLLIDE_AABB;
                vel = vec(0, 0, -1);
            }
        } d;
        d.o = o;
        if(!insideworld(d.o))
        {
            if(d.o.z < hdr.worldsize) return false;
            d.o.z = hdr.worldsize - 1e-3f;
            if(!insideworld(d.o)) return false;
        }
        vec v(0.0001f, 0.0001f, -1);
        v.normalize();
        if(raycube(d.o, v, hdr.worldsize) >= hdr.worldsize) return false;
        d.radius = d.xradius = d.yradius = radius;
        d.height = height;
        d.aboveeye = radius;
        if(!movecamera(&d, vec(0, 0, -1), hdr.worldsize, 1))
        {
            o = d.o;
            return true;
        }
        return false;
    }

    void update()
    {
        int diff = lastmillis - lastphysframe;
        if(diff <= 0) physsteps = 0;
        else
        {
            physsteps = (diff + physframetime - 1)/physframetime;
            lastphysframe += physsteps * physframetime;
        }
        cleardynentcache();
    }
}
