// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// Linking Gnash statically or dynamically with other modules is making a
// combined work based on Gnash. Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Gnash give you
// permission to combine Gnash with free software programs or libraries
// that are released under the GNU LGPL and with code included in any
// release of Talkback distributed by the Mozilla Foundation. You may
// copy and distribute such a system following the terms of the GNU GPL
// for all but the LGPL-covered parts and Talkback, and following the
// LGPL for the LGPL-covered parts.
//
// Note that people who make modified versions of Gnash are not obligated
// to grant this special exception for their modified versions; it is their
// choice whether to do so. The GNU General Public License gives permission
// to release a modified version without this exception; this exception
// also makes it possible to release a modified version which carries
// forward this exception.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"
#include "Key.h"
#include "action.h" // for action_init
#include "fn_call.h"
#include "sprite_instance.h"

namespace gnash {

Key::Key() {
}

Key::~Key() {
}


void
Key::addListener()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

void
Key::getAscii()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

void
Key::getCode()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

void
Key::isDown()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

void
Key::isToggled()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

void
Key::removeListener()
{
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void
key_new(const fn_call& fn)
{
    key_as_object *key_obj = new key_as_object;

    key_obj->set_member("addlistener", &key_addlistener);
    key_obj->set_member("getascii", &key_getascii);
    key_obj->set_member("getcode", &key_getcode);
    key_obj->set_member("isdown", &key_isdown);
    key_obj->set_member("istoggled", &key_istoggled);
    key_obj->set_member("removelistener", &key_removelistener);

    fn.result->set_as_object(key_obj);
}
void key_addlistener(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void key_getascii(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void key_getcode(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void key_isdown(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void key_istoggled(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}
void key_removelistener(const fn_call& fn) {
    log_msg("%s:unimplemented \n", __FUNCTION__);
}

/************************************************************************
 *
 * This has been moved from action.cpp, when things are clean
 * everything should have been moved up
 *
 ************************************************************************/

key_as_object::key_as_object()
	:
	m_last_key_pressed(0)
{
    memset(m_keymap, 0, sizeof(m_keymap));
}

bool
key_as_object::is_key_down(int code)
{
	    if (code < 0 || code >= key::KEYCOUNT) return false;

	    int	byte_index = code >> 3;
	    int	bit_index = code - (byte_index << 3);
	    int	mask = 1 << bit_index;

	    assert(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

	    if (m_keymap[byte_index] & mask)
		{
		    return true;
		}
	    else
		{
		    return false;
		}
}

void
key_as_object::set_key_down(int code)
{
	    if (code < 0 || code >= key::KEYCOUNT) return;

	    m_last_key_pressed = code;

	    int	byte_index = code >> 3;
	    int	bit_index = code - (byte_index << 3);
	    int	mask = 1 << bit_index;

	    assert(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

	    m_keymap[byte_index] |= mask;

	    notify_listeners(event_id(event_id::KEY_DOWN).get_function_name());
}


void
key_as_object::set_key_up(int code)
{
	    if (code < 0 || code >= key::KEYCOUNT) return;

	    int	byte_index = code >> 3;
	    int	bit_index = code - (byte_index << 3);
	    int	mask = 1 << bit_index;

	    assert(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

	    m_keymap[byte_index] &= ~mask;

	    notify_listeners(event_id(event_id::KEY_UP).get_function_name());
}

void
key_as_object::notify_listeners(const tu_stringi& funcname)
{
    // Notify listeners.
    for (std::vector<weak_ptr<as_object> >::iterator iter = m_listeners.begin();
         iter != m_listeners.end(); ++iter) {
      if (*iter == NULL)
        continue;

      smart_ptr<as_object>  listener = *iter; // Hold an owning reference.
      as_value method;

      if (listener->get_member(funcname, &method))
        call_method(method, NULL /* or root? */, listener.get_ptr(), 0, 0);
    }
}

#if 0
// XXXbjacques
//   Well, I think cleanup_listeners() is a bad idea for several reasons:
//   1) Since m_listeners is a vector of weak pointers, it is no way guaranteed
//      that m_listeners won't be "dirty" immediately after cleanup_listeners()
//      is called. Any users of m_listeners will have to NULL check pointers
//      (and add a ref) retrieved from m_listeners anyway.
//   2) std::vector.erase() is a potentially expensive operation.
//   3) std::vector.erase() does no reallocation. We gain no memory by doing
//      this.
//   4) Thread safety may be an issue (due to the change of m_listeners.size(),
//      as a result of std::vector.erase()). We really, really should be using
//      iterators instead of size()/indicing to iterate these vectors, though.

void
key_as_object::cleanup_listeners()
{
	    for (int i = m_listeners.size() - 1; i >= 0; i--)
		{
		    if (m_listeners[i] == NULL)
			{
			    m_listeners.erase(m_listeners.begin() + i);
			}
		}
}

#endif

void
key_as_object::add_listener(as_object* listener)
{
#if 0
//XXXbjacques: see comment above
	    cleanup_listeners();
#endif

    // Should we bother doing this every time someone calls add_listener(),
    // or should we perhaps skip this check and use unique later?
    std::vector<weak_ptr<as_object> >::const_iterator end = m_listeners.end();
    for (std::vector<weak_ptr<as_object> >::iterator iter = m_listeners.begin();
         iter != end; ++iter) {
      if (*iter == NULL) {
        // Already in the list.
        return;
      }
    }

    m_listeners.push_back(listener);
}

void
key_as_object::remove_listener(as_object* listener)
{
#if 0
XXXbjacques: see above comment
	    cleanup_listeners();
#endif

  for (std::vector<weak_ptr<as_object> >::iterator iter = m_listeners.begin(); iter != m_listeners.end(); )
	{
    if (*iter == listener)
		{
      iter = m_listeners.erase(iter);
			continue;
    }
		iter++;
	}
}

int
key_as_object::get_last_key_pressed() const
{
	return m_last_key_pressed;
}


void
key_add_listener(const fn_call& fn)
{
    if (fn.nargs < 1)
	{
	    log_error("key_add_listener needs one argument (the listener object)\n");
	    return;
	}

    as_object*	listener = fn.arg(0).to_object();
    if (listener == NULL)
	{
	    log_error("key_add_listener passed a NULL object; ignored\n");
	    return;
	}

    key_as_object*	ko = static_cast<key_as_object*>( fn.this_ptr );
    assert(ko);

    ko->add_listener(listener);
}

void	key_get_ascii(const fn_call& fn)
    // Return the ascii value of the last key pressed.
{
    key_as_object*	ko = static_cast<key_as_object*>( fn.this_ptr );
    assert(ko);

    fn.result->set_undefined();

    int	code = ko->get_last_key_pressed();
    if (code > 0)
	{
	    // @@ Crude for now; just jamming the key code in a string, as a character.
	    // Need to apply shift/capslock/numlock, etc...
	    char	buf[2];
	    buf[0] = (char) code;
	    buf[1] = 0;

	    fn.result->set_string(buf);
	}
}

void	key_get_code(const fn_call& fn)
    // Returns the keycode of the last key pressed.
{
    key_as_object*	ko = static_cast<key_as_object*>( fn.this_ptr );
    assert(ko);

    fn.result->set_int(ko->get_last_key_pressed());
}

void	key_is_down(const fn_call& fn)
    // Return true if the specified (first arg keycode) key is pressed.
{
    if (fn.nargs < 1)
	{
	    log_error("key_is_down needs one argument (the key code)\n");
	    return;
	}

    int	code = (int) fn.arg(0).to_number();

    key_as_object*	ko = static_cast<key_as_object*>( fn.this_ptr );
    assert(ko);

    fn.result->set_bool(ko->is_key_down(code));
}

void	key_is_toggled(const fn_call& fn)
    // Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
    // the associated state is on.
{
    // @@ TODO
    fn.result->set_bool(false);
}

void	key_remove_listener(const fn_call& fn)
    // Remove a previously-added listener.
{
    if (fn.nargs < 1)
	{
	    log_error("key_remove_listener needs one argument (the listener object)\n");
	    return;
	}

    as_object*	listener = fn.arg(0).to_object();
    if (listener == NULL)
	{
	    log_error("key_remove_listener passed a NULL object; ignored\n");
	    return;
	}

    key_as_object*	ko = static_cast<key_as_object*>( fn.this_ptr );
    assert(ko);

    ko->remove_listener(listener);
}

static std::vector<weak_ptr<as_object> >	s_keypress_listeners;

void add_keypress_listener(as_object* listener)
{
	std::vector<weak_ptr<as_object> >::const_iterator end = s_keypress_listeners.end();
	for (std::vector<weak_ptr<as_object> >::iterator iter = s_keypress_listeners.begin();
			iter != end; ++iter) 
	{
		if (*iter == NULL)
		{
			// Already in the list.
			return;
		}
	}
	s_keypress_listeners.push_back(listener);
}

void remove_keypress_listener(as_object* listener)
{
	for (std::vector<weak_ptr<as_object> >::iterator iter = s_keypress_listeners.begin();
			iter != s_keypress_listeners.end(); )
	{
		if (*iter == listener)
		{
			iter = s_keypress_listeners.erase(iter);
			continue;
		}
		iter++;
	}
}

void	notify_key_event(key::code k, bool down)
    // External interface for the host to report key events.
{
//	    GNASH_REPORT_FUNCTION;
	    
    action_init();	// @@ put this in some global init somewhere else...

	// Notify keypress listeners.
	if (down) 
	{
		for (std::vector<weak_ptr<as_object> >::iterator iter = s_keypress_listeners.begin();
				 iter != s_keypress_listeners.end(); ++iter)
		{
			if (*iter == NULL)
			{
					continue;
			}

			smart_ptr<as_object>  listener = *iter; // Hold an owning reference.

			sprite_instance* sprite = (sprite_instance*) listener.get_ptr();
			sprite->on_event(event_id(event_id::KEY_PRESS, (key::code) k));
		}
	}

    static tu_string	key_obj_name("Key");

    as_value	kval;
    s_global->get_member(key_obj_name, &kval);
    if (kval.get_type() == as_value::OBJECT)
	{
	    key_as_object*	ko = static_cast<key_as_object*>( kval.to_object() );
	    assert(ko);

	    if (down) ko->set_key_down(k);
	    else ko->set_key_up(k);
	}
    else
	{
	    log_error("gnash::notify_key_event(): no Key built-in\n");
	}
}

// This has to be moved to Key.{cpp,h}
void key_init(as_object* s_global)
{
//	    GNASH_REPORT_FUNCTION;
    // Create built-in key object.
    as_object*	key_obj = new key_as_object;

    // constants
#define KEY_CONST(k) key_obj->set_member(#k, key::k)
    KEY_CONST(BACKSPACE);
    KEY_CONST(CAPSLOCK);
    KEY_CONST(CONTROL);
    KEY_CONST(DELETEKEY);
    KEY_CONST(DOWN);
    KEY_CONST(END);
    KEY_CONST(ENTER);
    KEY_CONST(ESCAPE);
    KEY_CONST(HOME);
    KEY_CONST(INSERT);
    KEY_CONST(LEFT);
    KEY_CONST(PGDN);
    KEY_CONST(PGUP);
    KEY_CONST(RIGHT);
    KEY_CONST(SHIFT);
    KEY_CONST(SPACE);
    KEY_CONST(TAB);
    KEY_CONST(UP);

    // methods
    key_obj->set_member("addListener", &key_add_listener);
    key_obj->set_member("getAscii", &key_get_ascii);
    key_obj->set_member("getCode", &key_get_code);
    key_obj->set_member("isDown", &key_is_down);
    key_obj->set_member("isToggled", &key_is_toggled);
    key_obj->set_member("removeListener", &key_remove_listener);

    s_global->set_member("Key", key_obj);
}

} // end of gnash namespace

