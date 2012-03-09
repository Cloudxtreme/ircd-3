/************************************************************************
 *   IRC - Internet Relay Chat, modules/m_svsnick.c
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
 *   $Id: m_svsnick.c,v 1.3 2003/05/24 00:07:45 demond Exp $
 */
#include "stdinc.h"
#include "handlers.h"
#include "client.h"
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

static void ms_svsnick(struct Client*, struct Client*, int, char**);

struct Message svsnick_msgtab = {
  "SVSNICK", 0, 0, 4, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svsnick, m_ignore}
};

#ifndef STATIC_MODULES
void
_modinit(void)
{
  mod_add_cmd(&svsnick_msgtab);
}

void
_moddeinit(void)
{
  mod_del_cmd(&svsnick_msgtab);
}

const char *_version = "$Revision: 1.3 $";
#endif
/*
 * ms_svsnick - SVSNICK command handler
 *      parv[0] = sender prefix
 *      parv[1] = remote server
 *      parv[2] = old nick
 *      parv[3] = new nick
 */
static void ms_svsnick(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
  struct Client *victim;
  
  if (ServerInfo.gateway && get_bridge_token(client_p))
  	return;

  if (hunt_server(client_p, source_p, ":%s SVSNICK %s %s :%s", 
		  1, parc, parv) != HUNTED_ISME)
    return;

  if (!find_z_conf(parv[0]))
    return;
    
  victim = find_client(parv[2]);
  if (victim && !IsServer(victim) && MyConnect(victim))
  {
    if (find_client(parv[3]))
	{
	  exit_client(victim, victim, &me, "SVSNICK collision");
	  return;
	}

    change_local_nick(victim, victim, parv[3]);
  }
}
