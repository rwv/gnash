// Mouse.cpp:  ActionScript "Mouse" input device class, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "Mouse_as.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "VM.h" // for registerNative
#include "Object.h" // for getObjectInterface
#include "AsBroadcaster.h" // for initializing self as a broadcaster
#include "movie_root.h" // for GUI callback

namespace gnash {

// Forward declarations
namespace {    
    as_value mouse_hide(const fn_call& fn);
    as_value mouse_show(const fn_call& fn);

    void attachMouseInterface(as_object& o);
}

/// Mouse isn't a proper class in AS
//
/// Gnash's Mouse_as just has static methods.
void
Mouse_as::registerNative(as_object& o)
{
    VM& vm = o.getVM();

    vm.registerNative(mouse_show, 5, 0);
    vm.registerNative(mouse_hide, 5, 1);
}


// extern (used by Global.cpp)
void
Mouse_as::init(as_object& global)
{
    // This is going to be the global Mouse "class"/"function"
    boost::intrusive_ptr<as_object> obj = new as_object(getObjectInterface());
    attachMouseInterface(*obj);

    // Register _global.Mouse
    global.init_member("Mouse", obj.get());

}


namespace {

void
attachMouseInterface(as_object& o)
{
    VM& vm = o.getVM();

    const int flags = as_prop_flags::dontEnum |
                      as_prop_flags::dontDelete |
                      as_prop_flags::readOnly;

    o.init_member("show", vm.getNative(5, 0), flags);
    o.init_member("hide", vm.getNative(5, 1), flags);
 
    // Mouse is always initialized as an AsBroadcaster, even for
    // SWF5.   
    AsBroadcaster::initialize(o);
}

/// Returns whether the mouse was visible before the call.
//
/// The return is not a boolean, but rather 1 or 0.
as_value
mouse_hide(const fn_call& fn)
{
    boost::intrusive_ptr<as_object> obj = ensureType<as_object>(fn.this_ptr);

    movie_root& m = obj->getVM().getRoot();

    const int success = (m.callInterface("Mouse.hide") == "true") ? 1 : 0;

    // returns 1 if mouse was visible before call.
    return as_value(success);
}

/// Returns whether the mouse was visible before the call.
//
/// The return is not a boolean, but rather 1 or 0.
as_value
mouse_show(const fn_call& fn)
{
    boost::intrusive_ptr<as_object> obj=ensureType<as_object>(fn.this_ptr);

    movie_root& m = obj->getVM().getRoot();

    const int success = (m.callInterface("Mouse.show") == "true") ? 1 : 0;

    // returns 1 if Mouse was visible before call.
    return as_value(success);
}

} // anonymous namespace
} // end of gnash namespace