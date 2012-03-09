/************************************************************************
 *   IRC - Internet Relay Chat, modules/m_svsmisc.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: m_svsmisc.c,v 1.4 2003/05/24 00:07:45 demond Exp $
 */
#include "stdinc.h"
#include "handlers.h"
#include "client.h"
#include "channel.h"
#include "channel_mode.h"
#include "ircd.h"
#include "hash.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_serv.h"
#include "s_user.h"
#include "send.h"
#include "msg.h"
#include "parse.h"
#include "modules.h"

static void ms_svsjoin(struct Client*, struct Client*, int, char**);
static void ms_svspart(struct Client*, struct Client*, int, char**);
static void ms_svsmode(struct Client*, struct Client*, int, char**);

static void svsjoin(struct Client *, char *);
static void svspart(struct Client *, char *);

struct Message svsjoin_msgtab = {
  "SVSJOIN", 0, 0, 3, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svsjoin, m_ignore}
};

struct Message svspart_msgtab = {
  "SVSPART", 0, 0, 2, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svspart, m_ignore}
};

struct Message svsmode_msgtab = {
  "SVSMODE", 0, 0, 4, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svsmode, m_ignore}
};

#ifndef STATIC_MODULES
void
_modinit(void)
{
  mod_add_cmd(&svsjoin_msgtab);
  mod_add_cmd(&svspart_msgtab);
  mod_add_cmd(&svsmode_msgtab);
}

void
_moddeinit(void)
{
  mod_del_cmd(&svsjoin_msgtab);
  mod_del_cmd(&svspart_msgtab);
  mod_del_cmd(&svsmode_msgtab);
}

const char *_version = "$Revision: 1.4 $";
#endif
/*
 * ms_svsjoin - SVSJOIN command handler
 *      parv[0] = sender prefix
 *      parv[1] = remote server
 *      parv[2] = nickname
 *      parv[3] = channel to join
 * -or-
 *      parv[0] = sender prefix
 *      parv[1] = channel/nick mask
 *      parv[2] = channel to join 
 */
static void ms_svsjoin(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
  struct Client *victim;
  struct Channel *jchan;
  
  if (ServerInfo.gateway && get_bridge_token(client_p))
  	return;

  if (parc > 3) {

  	if (hunt_server(client_p, source_p, ":%s SVSJOIN %s %s :%s", 
		  1, parc, parv) != HUNTED_ISME)
    	return;

  	if (!find_z_conf(parv[0]))
    	return;
    	
   	victim = find_client(parv[2]);
  	if (victim && !IsServer(victim) && MyConnect(victim))
  		svsjoin(victim, parv[3]);
   	
  } else {
    
    jchan = hash_find_channel(parv[2]);
  	sendto_server(client_p, source_p, jchan, CAP_SVS, NOCAPS, LL_ICLIENT|LL_ICHAN,
				":%s SVSJOIN %s :%s", parv[0], parv[1], parv[2]);

  	if (!find_z_conf(parv[0]))
		return;

	if (parv[1][0] != '#')
		do_local_clients(svsjoin, parv[1], parv[2]);
		
	else {
	
		if (!hash_find_channel(parv[1]))
			return;
		do_channel_local(svsjoin, parv[1], parv[2]);
	}
  }
}

/*
 * ms_svspart - SVSPART command handler
 *      parv[0] = sender prefix
 *      parv[1] = remote server
 *      parv[2] = nickname
 *      parv[3] = channel to part
 * -or-
 *      parv[0] = sender prefix
 *      parv[1] = channel/nick mask
 *      parv[2] = channel to part (might be missing if parv[1] is a chan) 
 */
static void ms_svspart(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
  struct Client *victim;
  
  if (ServerInfo.gateway && get_bridge_token(client_p))
  	return;

  if (parc > 3) {

  	if (hunt_server(client_p, source_p, ":%s SVSPART %s %s :%s", 
		  1, parc, parv) != HUNTED_ISME)
    	return;

  	if (!find_z_conf(parv[0]))
    	return;
    	
   	victim = find_client(parv[2]);
   	if (hash_find_channel(parv[3]))
  	if (victim && !IsServer(victim) && MyConnect(victim))
  		svspart(victim, parv[3]);
   	
  } else {
    
  	sendto_server(client_p, source_p, NULL, CAP_SVS, NOCAPS, LL_ICLIENT,
				":%s SVSPART %s %s", parv[0], parv[1], (parc > 2)? parv[2] : "");

  	if (!find_z_conf(parv[0]))
		return;
		
	if (parv[1][0] == '#')
		if (!hash_find_channel(parv[1]))
			return;
	if (parc > 2)
		if (!hash_find_channel(parv[2]))
			return;

	if (parv[1][0] != '#') {
	
		if (parc < 3) return;
		do_local_clients(svspart, parv[1], parv[2]);
		
	} else
		do_channel_local(svspart, parv[1], (parc > 2)? parv[2] : parv[1]);
  }
}

/*
 * ms_svsmode - SVSMODE command handler
 *      parv[0] = sender prefix
 *      parv[1] = remote server
 *      parv[2] = target nick
 *      parv[3] = umode change(s)
 */
static void ms_svsmode(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
	struct Client *victim;
	
	if (ServerInfo.gateway && get_bridge_token(client_p))
		return;

  	if (hunt_server(client_p, source_p, ":%s SVSMODE %s %s :%s", 
		  1, parc, parv) != HUNTED_ISME)
    	return;

  	if (!find_z_conf(parv[0]) || !(victim = find_person(parv[2])))
    	return;

	if (MyConnect(victim)) {
		char *argv[4];
	
		argv[0] = parv[2];
		argv[1] = parv[2];
		argv[2] = parv[3];
		argv[3] = NULL;
		
		user_mode(victim, victim, 3, argv);
	}
}

static void svsjoin(struct Client *victim, char *chan)
{
	int flags = 0;
	struct Channel *chptr;
	char modebuf[MODEBUFLEN];
	char parabuf[MODEBUFLEN];

	if (!IsChannelName(chan) || !check_channel_name(chan) 
		|| (strlen(chan) > CHANNELLEN))
		return;
		
	modebuf[0] = '+';
	modebuf[1] = '\0';
	parabuf[0] = '\0';

	if ((chptr = hash_find_channel(chan))) {
	
		if (IsMember(victim, chptr)) return;
		
		if (chptr->users == 0)
			flags = CHFL_CHANOP;
		else
			flags = 0;
			
	} else {
	
		flags = CHFL_CHANOP;
		
		if (!ServerInfo.hub && uplink && IsCapable(uplink, CAP_LL)) {
		
			sendto_one(uplink, ":%s CBURST %s %s",
				me.name, chan, victim->name);
			return;									/* wait for LLJOIN */
		}
	
		if (!(chptr = get_or_create_channel(victim, chan, NULL)))
			return;
	}
	
	add_user_to_channel(chptr, victim, flags);
	
	if (flags & CHFL_CHANOP) {
	
		chptr->channelts = CurrentTime;
		chptr->mode.mode |= MODE_TOPICLIMIT;
		chptr->mode.mode |= MODE_NOPRIVMSGS;
		modebuf[0] = '+';
		modebuf[1] = 'n';
		modebuf[2] = 't';
		modebuf[3] = '\0';
	
		sendto_server(NULL, victim, chptr, NOCAPS, NOCAPS, LL_ICLIENT,
			":%s SJOIN %lu %s %s %s:@%s",
			me.name, (unsigned long) chptr->channelts,
			chptr->chname, modebuf, parabuf, victim->name);
			
	  	sendto_channel_local(ALL_MEMBERS, chptr, ":%s!%s@%s JOIN :%s",
			       victim->name, victim->username,
			       victim->host, chptr->chname);
      
		sendto_channel_local(ONLY_CHANOPS_HALFOPS, chptr, ":%s MODE %s %s %s",
			       me.name, chptr->chname,
			       modebuf, parabuf);
	} else {

		sendto_server(NULL, victim, chptr, NOCAPS, NOCAPS, LL_ICLIENT,
			":%s SJOIN %lu %s %s %s:%s",
			me.name, (unsigned long) chptr->channelts,
			chptr->chname, modebuf, parabuf, victim->name);

		sendto_channel_local(ALL_MEMBERS, chptr, ":%s!%s@%s JOIN :%s",
			       victim->name, victim->username,
			       victim->host, chptr->chname);
	}
	
	del_invite(chptr, victim);

	if (chptr->topic != NULL) {
	  	char foo[TOPICLEN]; 
	  
	  	strlcpy(foo, chptr->topic, TOPICLEN);
	  	sendto_one(victim, form_str(victim,RPL_TOPIC),
		     me.name, victim->name, chptr->chname, 
		     ConfigFileEntry.totaltrans?
		     translate(victim, foo) : chptr->topic);

		if (!(chptr->mode.mode & MODE_HIDEOPS) ||
			(flags & CHFL_CHANOP) || (flags & CHFL_HALFOP))
            {
              sendto_one(victim, form_str(victim,RPL_TOPICWHOTIME),
                         me.name, victim->name, chptr->chname,
                         chptr->topic_info, chptr->topic_time);
            }
          else /* Hide from nonops */
            {
              sendto_one(victim, form_str(victim,RPL_TOPICWHOTIME),
                         me.name, victim->name, chptr->chname,
                         me.name, chptr->topic_time);
            }
	}

	channel_member_names(victim, chptr, chptr->chname, 1);
	victim->localClient->last_join_time = CurrentTime;
}

static void svspart(struct Client *victim, char *chan)
{
	struct Channel *chptr;
	static char reason[] = "SVSPART";

	if (!(chptr = hash_find_channel(chan)) || !IsMember(victim, chptr))
		return;

	sendto_server(NULL, NULL, chptr, CAP_UID, NOCAPS, NOFLAGS,
                    ":%s PART %s :%s", ID(victim), chptr->chname,
                    reason);
	sendto_server(NULL, NULL, chptr, NOCAPS, CAP_UID, NOFLAGS,
                    ":%s PART %s :%s", victim->name, chptr->chname,
                    reason);
                    
	sendto_channel_local(ALL_MEMBERS, chptr, ":%s!%s@%s PART %s :%s",
                           victim->name,
                           victim->username,
                           victim->host,
                           chptr->chname,
                           reason);
                           
	remove_user_from_channel(chptr, victim);
	victim->localClient->last_leave_time = CurrentTime;
}
