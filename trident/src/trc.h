/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 2005 University of Cambridge Computer Laboratory.               *
*                                                                             *
*                                                                             *
*   This file is part of Trident.                                             *
*                                                                             *
*   Trident is free software; you can redistribute it and/or modify           *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   Trident is distributed in the hope that it will be useful,                 *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/

/******************************************************************************
 * Trace macros                                                               *
 *                                                                            *
 * (C) Richard Mortier, Cambridge University Computer Laboratory, 2000        *
 *     All Rights Reserved.                                                   *
 *                                                                            *
 * Based on Tim Deegan's (tjd21) and Keir Fraser's (kaf24).                   *
 *                                                                            *
 ******************************************************************************
*/

#ifdef __KERNEL__
# define dprintf printk
#else
# define dprintf printf
#endif

#define ERROR(fmt,args...) dprintf("*** [%s:%i]:%s: " ## fmt,		\
				   __FILE__, __LINE__,			\
				   __PRETTY_FUNCTION__ , ## args)

#ifdef _ENTER_EXIT_
# define ENTER   dprintf("[%s:%i]: +++ %s\n",				\
			 __FILE__, __LINE__, __PRETTY_FUNCTION__)
# define LEAVE   dprintf("[%s:%i]: --- %s\n",				\
			 __FILE__, __LINE__, __PRETTY_FUNCTION__)
# define RETURN  LEAVE; return
#else
# define ENTER
# define LEAVE
# define RETURN  return
#endif // _ENTER_EXIT_

#ifdef _TRACE_
# define TRC(fmt,args...) dprintf("### [%s:%i]:%s: " ## fmt,		\
				  __FILE__, __LINE__,			\
				  __PRETTY_FUNCTION__ , ## args)
#else
# define TRC(fmt,args...)
#endif // _TRACE_
