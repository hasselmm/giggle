/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <signal.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#endif

#include "giggle-sysdeps.h"

void 
giggle_sysdeps_kill_pid (GPid pid)
{
#ifndef G_OS_WIN32
	kill (pid, SIGKILL);
#else
	/* From Gimp (app/plugin/gimpplugin.c) */
	{
		DWORD dwExitCode = STILL_ACTIVE;
		DWORD dwTries    = 10;

		while (dwExitCode == dwExitCode &&
		       GetExitCodeProcess ((HANDLE) pid, &dwExitCode) &&
		       (dwTries > 0)) {
			Sleep (10);
			dwTries--;
		}

		if (dwExitCode == STILL_ACTIVE)	{
			TerminateProcess ((HANDLE) pid, 0);
		}
	}
#endif
}

