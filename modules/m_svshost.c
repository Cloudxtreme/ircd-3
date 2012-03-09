/************************************************************************
 *   IRC - Internet Relay Chat, modules/m_svshost.c
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
 *   $Id: m_svshost.c,v 1.10 2003/05/24 00:07:45 demond Exp $
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
#include "vchannel.h"

static void ms_svshost(struct Client*, struct Client*, int, char**);

struct Message svshost_msgtab = {
  "SVSHOST", 0, 0, 4, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svshost, m_ignore}
};

#ifndef STATIC_MODULES
void
_modinit(void)
{
  mod_add_cmd(&svshost_msgtab);
}

void
_moddeinit(void)
{
  mod_del_cmd(&svshost_msgtab);
}

const char *_version = "$Revision: 1.10 $";
#endif
/*
 * ms_svshost - SVSHOST command handler
 *      parv[0] = sender prefix
 *      parv[1] = remote server
 *      parv[2] = client nick
 *      parv[3] = vhost to spoof
 */
static void ms_svshost(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
  char *flag;
  struct Client *victim;
  struct Channel *chptr, *top_chptr, *banchans[100];
  static char comment[] = "Spoofing with vhost...";
  static char urbanned[] = "You are banned";
  char modebuf[MODEBUFLEN], parabuf[MODEBUFLEN];
  int hide_or_not;
  dlink_node *ptr;
  int i, num = 0;
  
  if (ServerInfo.gateway && get_bridge_token(client_p))
  	return;

  if (hunt_server(client_p, source_p, ":%s SVSHOST %s %s :%s", 
		  1, parc, parv) != HUNTED_ISME)
    return;

  if (!find_z_conf(parv[0]))
    return;
    
  victim = find_client(parv[2]);
  if (victim && !IsServer(victim) && MyConnect(victim))
  {
  	extern int introduce_client(struct Client *, struct Client *,
  		struct User *, char *);
  
  	sendto_server(NULL, victim, NULL, NOCAPS, NOCAPS,
		NOFLAGS, ":%s QUIT :%s", victim->name, comment);

/*	sendto_common_channels_local(victim, ":%s!%s@%s QUIT :%s",
		victim->name, victim->username, victim->host, comment);*/
		
/*	++current_serial;	not needed i suppose */

	if (victim->user->channel.head)
	for (ptr = victim->user->channel.head; ptr; ptr = ptr->next) {
		chptr = (struct Channel *)ptr->data;
		
		sendto_channel_local_butone(victim, ALL_MEMBERS, 
			chptr, ":%s!%s@%s QUIT :%s",
			victim->name, victim->username,
			victim->host, comment);
	}
	
	strncpy(victim->host, parv[3], HOSTLEN)[HOSTLEN] = 0;
	
	sendto_one(victim, ":%s NOTICE %s :*** Notice -- Spoofing your host as %s",
		me.name, victim->name, victim->host);
	
	if (victim->user->channel.head)
	for (ptr = victim->user->channel.head; ptr; ptr = ptr->next) {
		chptr = (struct Channel *)ptr->data;
		
#ifdef VCHANS
		top_chptr = RootChan(chptr);
#else
		top_chptr = chptr;
#endif		
		if (is_banned(chptr, victim) & (CHFL_BAN|CHFL_DENY)) {
			sendto_one(victim, ":%s KICK %s %s :%s",
				me.name, top_chptr->chname, victim->name,
				urbanned);
			
			/*remove_user_from_channel(chptr, victim);*/
			banchans[num++] = chptr;
			continue;
		}
		
		if (is_chan_op(chptr, victim)) 
			flag = "+o";
#ifdef HALFOPS			
		else if (is_half_op(chptr, victim)) 
			flag = "+h";
#endif			
		else if (is_voiced(chptr, victim))
			flag = "+v";
		else flag = "";
#ifdef REQUIRE_OANDV
		if (find_user_link(&chptr->chanops_voiced, victim))
			flag = "+ov";
#endif
		sendto_channel_local_butone(victim, ALL_MEMBERS, 
			chptr, ":%s!%s@%s JOIN :%s",
			victim->name, victim->username,
			victim->host, top_chptr->chname);
			
#ifdef ANONOPS
		if (chptr->mode.mode & MODE_HIDEOPS)
			hide_or_not = ONLY_CHANOPS_HALFOPS;
		else
#endif
			hide_or_not = ALL_MEMBERS;
			
		if (flag[0])
		sendto_channel_local_butone(victim, hide_or_not,
			chptr, ":%s MODE %s %s %s %s",
			me.name, top_chptr->chname, flag,
			victim->name, flag[2]? victim->name : "");		
	}
	
	for (i=0; i < num; i++)
		remove_user_from_channel(banchans[i], victim);
	
	introduce_client(NULL, victim, victim->user, victim->name);
	
	victim->lazyLinkClientExists = 0;	/* force introducing to LL servers below... */
	
	if (victim->user->channel.head)
	for (ptr = victim->user->channel.head; ptr; ptr = ptr->next) {
		chptr = (struct Channel *)ptr->data;
		
		if (is_chan_op(chptr, victim)) 
			flag = "@";
#ifdef HALFOPS			
		else if (is_half_op(chptr, victim)) 
			flag = "%";
#endif			
		else if (is_voiced(chptr, victim))
			flag = "+";
		else flag = "";
#ifdef REQUIRE_OANDV
		if (find_user_link(&chptr->chanops_voiced, victim))
			flag = "@+";
#endif
		channel_modes(chptr, victim, modebuf, parabuf);
		sendto_server(NULL, victim, chptr, NOCAPS, NOCAPS,
			LL_ICLIENT|LL_ICHAN, ":%s SJOIN %lu %s %s %s :%s%s",
			me.name, (unsigned long)chptr->channelts,
			chptr->chname, modebuf, parabuf, 
			flag, victim->name);
	}
  }
}
