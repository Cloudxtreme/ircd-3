/************************************************************************
 *   IRC - Internet Relay Chat, modules/m_svshint.c
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
 *   $Id: m_svshint.c,v 1.6 2003/05/24 00:07:45 demond Exp $
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

static void ms_svshint(struct Client*, struct Client*, int, char**);

struct Message svshint_msgtab = {
  "SVSHINT", 0, 0, 0, 0, 0, 0,
  {m_unregistered, m_ignore, ms_svshint, m_ignore}
};

#ifndef STATIC_MODULES
void
_modinit(void)
{
  mod_add_cmd(&svshint_msgtab);
}

void
_moddeinit(void)
{
  mod_del_cmd(&svshint_msgtab);
}

const char *_version = "$Revision: 1.6 $";
#endif
/*
 * ms_svshint - SVSHINT command handler
 *      parv[0] = sender prefix
 *      parv[1] = hint flags
 *      parv[2] = nick
 */
static void ms_svshint(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
  int hint;
  struct Client *victim;
  
  if (ServerInfo.gateway && get_bridge_token(client_p))
  	return;

  if (parc < 2)
    {
      sendto_server(client_p, source_p, NULL, CAP_SVS, NOCAPS, LL_ICLIENT, 
      				":%s SVSHINT", parv[0]);

	  if (!find_z_conf(parv[0]))
		return;

      clear_svs_client_hash_table();
      return;
    }

  if (parc < 3)
    return;

  hint = atoi(parv[1]);

  sendto_server(client_p, source_p, NULL, CAP_SVS, NOCAPS, LL_ICLIENT,
				":%s SVSHINT %s :%s", parv[0], parv[1], parv[2]);

  if (!find_z_conf(parv[0]))
	return;

  victim = find_client(parv[2]);

  if (victim && !IsServer(victim))
    {
      if (hint == 0) {
      	victim->svsflags &= ~FLAGS_SVS_IDENT;
      	if (--Count.identified < 0)
      		Count.identified = 0;
      }
      
      else if (hint > 0) {

      	if (hint & FLAGS_SVS_IDENT) {
        	victim->svsflags |= FLAGS_SVS_IDENT;
        	if (++Count.identified > Count.max_identified)
          		Count.max_identified = Count.identified;
      	}
      	if (hint & FLAGS_SVS_ADMIN)
        	victim->svsflags |= FLAGS_SVS_ADMIN;

      } else {

      	hint = -hint;
      	if (hint & FLAGS_SVS_IDENT) {
        	victim->svsflags &= ~FLAGS_SVS_IDENT;
        	if (--Count.identified < 0)
        		Count.identified = 0;
      	}
      	if (hint & FLAGS_SVS_ADMIN)
        	victim->svsflags &= ~FLAGS_SVS_ADMIN;

      }
    }
}
