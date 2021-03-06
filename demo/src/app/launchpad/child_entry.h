/*
 * \brief  Child entry widget
 * \author Norman Feske
 * \date   2006-09-13
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CHILD_ENTRY_H_
#define _CHILD_ENTRY_H_

#include <util/string.h>
#include <util/list.h>

#include <launchpad/launchpad.h>
#include "loadbar.h"

#define KILL_ICON_RGBA   _binary_kill_icon_rgba_start
#define OPENED_ICON_RGBA _binary_opened_icon_rgba_start
#define CLOSED_ICON_RGBA _binary_closed_icon_rgba_start
extern unsigned char KILL_ICON_RGBA[];
extern unsigned char OPENED_ICON_RGBA[];
extern unsigned char CLOSED_ICON_RGBA[];


class Kill_event_handler : public Event_handler
{
	private:

		Launchpad       *_launchpad;
		Launchpad_child *_launchpad_child;

	public:

		Kill_event_handler(Launchpad *launchpad, Launchpad_child *launchpad_child):
			_launchpad(launchpad), _launchpad_child(launchpad_child) { }

		/**
		 * Event handler interface
		 */
		void handle(Event &ev)
		{
			static int key_cnt;

			if (ev.type == Event::PRESS)   key_cnt++;
			if (ev.type == Event::RELEASE) key_cnt--;

			if (ev.type == Event::RELEASE && key_cnt == 0)
				_launchpad->exit_child(_launchpad_child);
		}
};


template <typename PT>
class Child_entry : public Parent_element, public Genode::List<Child_entry<PT> >::Element
{
	private:

		enum { _IW       = 16 };      /* icon width               */
		enum { _IH       = 16 };      /* icon height              */
		enum { _PTW      = 100 };     /* program text width       */
		enum { _PADX     = 10 };      /* horizontal padding       */
		enum { _NAME_LEN = 64 };      /* max length of child name */

		Block             _block;
		Kbyte_loadbar<PT> _loadbar;

		char              _name[_NAME_LEN];

		Fade_icon<PT, _IW, _IH> _kill_icon;
		Fade_icon<PT, _IW, _IH> _fold_icon;

		Kill_event_handler _kill_event_handler;

	public:

		/**
		 * Constructor
		 */
		Child_entry(const char *name, int quota_kb, int max_quota_kb,
		            Launchpad *launchpad, Launchpad_child *launchpad_child)
		:
			_block(Block::RIGHT), _loadbar(0, &label_font),
			_kill_event_handler(launchpad, launchpad_child)
		{
			Genode::strncpy(_name, name, sizeof(_name));
			_block.append_plaintext(_name, &plain_style);

			_loadbar.max_value(max_quota_kb);
			_loadbar.value(quota_kb);

			_kill_icon.rgba(KILL_ICON_RGBA, 0, 0);
			_kill_icon.alpha(100);
			_kill_icon.focus_alpha(200);
			_kill_icon.event_handler(&_kill_event_handler);

			_fold_icon.rgba(CLOSED_ICON_RGBA, 0, 0);
			_fold_icon.alpha(100);
			_fold_icon.focus_alpha(200);

			append(&_loadbar);
			append(&_block);
			append(&_kill_icon);
			append(&_fold_icon);

			_min_w = _PTW + 100;
		}


		/**
		 * Accessors
		 */
		const char *name() { return _name; }


		/******************************
		 ** Parent element interface **
		 ******************************/

		void format_fixed_width(int w)
		{
			_block.format_fixed_width(_PTW);
			int bh = _block.min_h();
			int iy = max(0, (bh - _loadbar.min_h())/2);

			_fold_icon.geometry(0, iy, _IW, _IH);
			_kill_icon.geometry(w - _IW - 8, iy, _IW, _IH);

			_block.geometry(max(10, _PTW - _block.min_w()),
			                max(0, (bh - _block.min_h())/2),
			                min((int)_PTW, _block.min_w()), bh);

			int lw = w - 2*_PADX - _PTW - _IW;
			_loadbar.format_fixed_width(lw);
			_loadbar.geometry(_PADX + _PTW, iy, lw, 16);
			_min_h = bh;
			_min_w = w;
		}
};

#endif
