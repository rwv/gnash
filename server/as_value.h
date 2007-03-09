// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/* $Id: as_value.h,v 1.29 2007/03/09 15:19:26 strk Exp $ */

#ifndef GNASH_AS_VALUE_H
#define GNASH_AS_VALUE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmath>
#include <string>

#include "container.h"
#include "tu_config.h"

//#include "resource.h" // for inheritance of as_object

namespace gnash {

class as_object;
class fn_call;
class as_function;
class sprite_instance;

#ifndef HAVE_ISFINITE
# ifndef isfinite 
#  define isfinite finite
# endif 
#endif 

#ifndef isnan
# define isnan(x) \
(sizeof (x) == sizeof (long double) ? isnan_ld (x) \
: sizeof (x) == sizeof (double) ? isnan_d (x) \
: isnan_f (x))
static inline int isnan_f  (float       x) { return x != x; }
static inline int isnan_d  (double      x) { return x != x; }
static inline int isnan_ld (long double x) { return x != x; }
#endif
	  
#ifndef isinf
# define isinf(x) \
(sizeof (x) == sizeof (long double) ? isinf_ld (x) \
: sizeof (x) == sizeof (double) ? isinf_d (x) \
: isinf_f (x))
	static inline int isinf_f  (float       x) { return isnan (x - x); }
	static inline int isinf_d  (double      x) { return isnan (x - x); }
	static inline int isinf_ld (long double x) { return isnan (x - x); }
#endif
 
//#define ALLOW_C_FUNCTION_VALUES

//#ifdef ALLOW_C_FUNCTION_VALUES
typedef void (*as_c_function_ptr)(const fn_call& fn);
//#endif

// ActionScript value type.

//No private: ???
class DSOEXPORT as_value
{
public:
	enum type
	{
		/// Undefined value
		UNDEFINED,

		/// NULL value
		NULLTYPE,

		/// Boolean value
		BOOLEAN,

		/// String value
		STRING,

		/// Number value
		NUMBER, 

		/// Object reference
		OBJECT,

#ifdef ALLOW_C_FUNCTION_VALUES
		/// Internal function pointer (to drop)
		//
		/// TODO: deprecate this ! *every* function
		///       in actionscript must be equipped with
		///       ActionScript properties. Every as_value
		///       function MUST be an AS_FUNCTION !!
		C_FUNCTION,
#endif

		/// ActionScript function reference
		AS_FUNCTION,

		/// MovieClip reference
		MOVIECLIP
	};

	/// Construct an UNDEFINED value
	as_value()
		:
		m_type(UNDEFINED),
		m_number_value(0.0)
	{
	}

	as_value(const as_value& v)
		:
		m_type(UNDEFINED),
		m_number_value(0.0)
	{
		*this = v;
	}

	/// Construct a STRING value 
	as_value(const char* str)
		:
		m_type(STRING),
		m_string_value(str),
		m_number_value(0.0)
	{
	}

	/// Construct a STRING value
	as_value(const wchar_t* wstr)
		:
		m_type(STRING),
		m_string_value(""),
		m_number_value(0.0)
	{
		// Encode the string value as UTF-8.
		//
		// Is this dumb?  Alternatives:
		//
		// 1. store a tu_wstring instead of tu_string?
		// Bloats typical ASCII strings, needs a
		// tu_wstring type, and conversion back the
		// other way to interface with char[].
		// 
		// 2. store a tu_wstring as a union with
		// tu_string?  Extra complexity.
		//
		// 3. ??
		//
		// Storing UTF-8 seems like a pretty decent
		// way to do it.  Everything else just
		// continues to work.

#if (WCHAR_MAX != MAXINT)
		tu_string::encode_utf8_from_wchar(&m_string_value, (const uint16 *)wstr);
#else
# if (WCHAR_MAX != MAXSHORT)
# error "Can't determine the size of wchar_t"
# else
		tu_string::encode_utf8_from_wchar(&m_string_value, (const uint32 *)wstr);
# endif
#endif
	}

	/// Construct a BOOLEAN value
	as_value(bool val)
		:
		m_type(BOOLEAN),
		m_boolean_value(val)
	{
	}

	/// Construct a NUMBER value
	as_value(int val)
		:
		m_type(NUMBER),
		m_number_value(double(val))
	{
	}

	/// Construct a NUMBER value
	as_value(unsigned int val)
		:
		m_type(NUMBER),
		m_number_value(double(val))
	{
	}

	/// Construct a NUMBER value
	as_value(float val)
		:
		m_type(NUMBER),
		m_number_value(double(val))
	{
	}

	/// Construct a NUMBER value
	as_value(double val)
		:
		m_type(NUMBER),
		m_number_value(val)
	{
	}

	/// Construct a NULL, OBJECT, MOVIECLIP or AS_FUNCTION value
	//
	/// See as_object::to_movie and as_object::to_function
	///
	/// Internally adds a reference to the ref-counted as_object, 
	/// if not-null
	///
	as_value(as_object* obj)
		:
		// Initialize to non-object type here,
		// or set_as_object will call
		// drop_ref on undefined memory !!
		m_type(UNDEFINED)
	{
		set_as_object(obj);
	}

#ifdef ALLOW_C_FUNCTION_VALUES
	/// Construct a C_FUNCTION value
	//
	/// TODO: deprecate this ! *every* function
	///       in actionscript must be equipped with
	///       ActionScript properties. Every as_value
	///       function MUST be an AS_FUNCTION !!
	///
	as_value(as_c_function_ptr func)
		:
		m_type(C_FUNCTION),
		m_c_function_value(func)
	{
	}
#endif

	/// Construct a NULL or AS_FUNCTION value
	as_value(as_function* func);

	~as_value() { drop_refs(); }

	/// Drop any ref counts we have.
	//
	/// This happens prior to changing our value.
	/// Useful when changing types/values.
	///
	void	drop_refs();

	/// Return the primitive type of this value, as a string.
	const char* typeOf() const;

	/// \brief
	/// Return true if this value is callable
	/// (C_FUNCTION or AS_FUNCTION).
	bool is_function() const
	{
#ifdef ALLOW_C_FUNCTION_VALUES
		return m_type == C_FUNCTION || m_type == AS_FUNCTION;
#else
		return m_type == AS_FUNCTION;
#endif
	}

#ifdef ALLOW_C_FUNCTION_VALUES
	/// Return true if this value is a C function
	//
	/// This is currently only important to know from the
	/// ActionNew tag hander, in that C_FUNCTION constructors
	/// and AS_FUNCTION constructor act in a sligtly different
	/// way. We should likely *drop* support for C_FUNCTIONS
	/// OR make them to act exactly like the AS_FUNCTION
	/// counterparts.
	///
	bool is_c_function() const
	{
		return m_type == C_FUNCTION;
	}
#endif

	/// Return true if this value is a AS function
	bool is_as_function() const
	{
		return m_type == AS_FUNCTION;
	}

	/// Return true if this value is strictly a string
	//
	/// Note that you usually DON'T need to call this
	/// function, as if you really want a string you
	/// can always call the to_string() or to_std_string()
	/// method to perform a conversion.
	///
	bool is_string() const
	{
		return m_type == STRING;
	}

	/// Return true if this value is strictly a number
	//
	/// Note that you usually DON'T need to call this
	/// function, as if you really want a number you
	/// can always call the to_number()
	/// method to perform a conversion.
	///
	bool is_number() const
	{
		return m_type == NUMBER;
	}

	/// \brief
	/// Return true if this value is an object
	/// (OBJECT, AS_FUNCTION or MOVIECLIP).
	bool is_object() const
	{
		return m_type == OBJECT || m_type == AS_FUNCTION || m_type == MOVIECLIP;
	}

	/// Get a C string representation of this value.
	const char*	to_string() const;

	/// Get a tu_string representation for this value.
	const tu_string&	to_tu_string() const;

	/// Get a std::string representation for this value.
	std::string to_std_string() const;

	/// Get a tu_string representation for this value.
	//
	/// This differs from to_tu_string() in that returned
	/// representation will depend on version of the SWF
	/// source. 
	/// @@ shouldn't this be the default ?
	///
	const tu_string&	to_tu_string_versioned(int version) const;

	/// Calls to_tu_string() returning a cast to tu_stringi
	const tu_stringi&	to_tu_stringi() const;

	/// Conversion to double.
	double	to_number() const;

	/// Conversion to boolean.
	bool	to_bool() const;

	/// Return value as a primitive type
	//
	/// Primitive types are: undefined, null, boolean, string, number.
	/// See ECMA-2.6.2 (section 4.3.2).
	as_value to_primitive() const;

	/// Return value as an object, converting primitive values as needed.
	//
	/// Make sure you store the returned pointer in a boost::intrusive_ptr
	/// as it might be a newly allocated one in case of a conversion from
	/// a primitive string, number or boolean value.
	///
	/// string values will be converted to String objects,
	/// numeric values will be converted to Number objects,
	/// boolean values are currently NOT converted...
	///
	/// If you want to avoid the conversion, check with is_object() before
	/// calling this function.
	///
	as_object* to_object() const;

	/// Return value as a sprite or NULL if this is not possible.
	//
	/// If the value is a MOVIECLIP value, the stored sprite target
	/// is evaluated using the root movie's environment
	/// (see gnash::as_environment::find_target). If the target
	/// points to something that doesn't cast to a sprite,
	/// NULL is returned.
	///
	/// Note that if the value is NOT a MOVIECLIP, NULL is always
	/// returned.
	///
	sprite_instance* to_sprite() const;

#ifdef ALLOW_C_FUNCTION_VALUES
	/// \brief
	/// Return value as a C function ptr
	/// or NULL if it is not a C function.
	as_c_function_ptr	to_c_function() const;
#endif

	/// \brief
	/// Return value as an ActionScript function ptr
	/// or NULL if it is not an ActionScript function.
	as_function*	to_as_function() const;

	/// Force type to number.
	void	convert_to_number();

	/// Force type to string.
	void	convert_to_string();
    
	/// Force type to string.
	//
	/// uses swf-version-aware converter
	///
	/// @see to_tu_string_versionioned
	///
	void	convert_to_string_versioned(int version);

	// These set_*()'s are more type-safe; should be used
	// in preference to generic overloaded set().  You are
	// more likely to get a warning/error if misused.

	void	set_tu_string(const tu_string& str) {
          drop_refs();
          m_type = STRING;
          m_string_value = str;
        }

	void	set_std_string(const std::string& str) {
          drop_refs();
          m_type = STRING;
          m_string_value = str.c_str();
        }

	void	set_string(const char* str) {
          drop_refs();
          m_type = STRING;
          m_string_value = str;
        }
	void	set_double(double val) {
          drop_refs();
          m_type = NUMBER;
          m_number_value = val;
        }
	void	set_bool(bool val) {
          drop_refs();
          m_type = BOOLEAN;
          m_boolean_value = val;
        }
	void	set_sprite(const std::string& path);
	void	set_sprite(const sprite_instance& sp);
	void	set_int(int val) { set_double(val); }
	void	set_nan() { double x = 0.0; set_double(x/x); }

	/// Make this value a NULL, OBJECT, MOVIECLIP or AS_FUNCTION value
	//
	/// See as_object::to_movie and as_object::to_function
	///
	/// Internally adds a reference to the ref-counted as_object, 
	/// if not-null
	///
	void	set_as_object(as_object* obj);

#ifdef ALLOW_C_FUNCTION_VALUES
	void	set_as_c_function_ptr(as_c_function_ptr func)
	{
		drop_refs(); m_type = C_FUNCTION; m_c_function_value = func;
	}
#endif

	/// Make this a NULL or AS_FUNCTION value
	void	set_as_function(as_function* func);

	void	set_undefined() { drop_refs(); m_type = UNDEFINED; }

	/// Set this value to the NULL value
	//
	/// @return a reference to this instance
	///
	as_value& set_null() { drop_refs(); m_type = NULLTYPE; return *this; }

	void	operator=(const as_value& v)
	{
		if (v.m_type == UNDEFINED) set_undefined();
		else if (v.m_type == NULLTYPE) set_null();
		else if (v.m_type == BOOLEAN) set_bool(v.m_boolean_value);
		else if (v.m_type == STRING) set_tu_string(v.m_string_value);
		else if (v.m_type == NUMBER) set_double(v.m_number_value);
		else if (v.m_type == OBJECT) set_as_object(v.m_object_value);

		//TODO: don't use c_str() when m_string_value will be a std::string
		else if (v.m_type == MOVIECLIP) set_sprite(v.m_string_value.c_str());

#ifdef ALLOW_C_FUNCTION_VALUES
		else if (v.m_type == C_FUNCTION) set_as_c_function_ptr(v.m_c_function_value);
#endif
		else if (v.m_type == AS_FUNCTION) set_as_function(v.m_as_function_value);
		else assert(0);
	}

	bool	is_nan() const { return (m_type == NUMBER && isnan(m_number_value)); }
	bool	is_inf() const { return (m_type == NUMBER && isinf(m_number_value)); }

	bool	is_undefined() const { return (m_type == UNDEFINED); }

	bool	is_null() const { return (m_type == NULLTYPE); }

	bool is_finite() const { return (m_type == NUMBER && isfinite(m_number_value)); }

	/// Return true if this value is strictly equal to the given one
	//
	/// Strict equality is defined as the two values being of the
	/// same type and the same value.
	///
	/// TODO: check what makes two MOVIECLIP values strictly equal
	///
	bool strictly_equals(const as_value& v) const;

	bool	operator==(const as_value& v) const;
	bool	operator!=(const as_value& v) const;
	bool	operator<(const as_value& v) const { return to_number() < v.to_number(); }
	void	operator+=(const as_value& v) { set_double(this->to_number() + v.to_number()); }
	void	operator-=(const as_value& v) { set_double(this->to_number() - v.to_number()); }
	void	operator*=(const as_value& v) { set_double(this->to_number() * v.to_number()); }
	void	operator/=(const as_value& v) { set_double(this->to_number() / v.to_number()); }  // @@ check for div/0
	void	operator&=(const as_value& v) { set_int(int(this->to_number()) & int(v.to_number())); }
	void	operator|=(const as_value& v) { set_int(int(this->to_number()) | int(v.to_number())); }
	void	operator^=(const as_value& v) { set_int(int(this->to_number()) ^ int(v.to_number())); }
	void	shl(const as_value& v) { set_int(int(this->to_number()) << int(v.to_number())); }
	void	asr(const as_value& v) { set_int(int(this->to_number()) >> int(v.to_number())); }
	void	lsr(const as_value& v) { set_int((uint32_t(this->to_number()) >> int(v.to_number()))); }

	/// Sets this value to this string plus the given string.
	void	string_concat(const tu_string& str);

	tu_string* get_mutable_tu_string() { assert(m_type == STRING); return &m_string_value; }

private:

	// TODO: make private. The rationale is that callers of this functions
	//       should use is_WHAT() instead, or changes in the available
	//       primitive value types will require modifications in all callers.
	//       This happened when adding MOVIECLIP.
	//
	type	get_type() const { return m_type; }


	type	m_type;

	// TODO: switch to std::string
	mutable tu_string	m_string_value;

	union
	{
		bool m_boolean_value;
		// @@ hm, what about PS2, where double is bad?  should maybe have int&float types.
		mutable	double	m_number_value;
		as_object*	m_object_value;
#ifdef ALLOW_C_FUNCTION_VALUES
		as_c_function_ptr	m_c_function_value;
#endif
		as_function*	m_as_function_value;
	};

};

inline std::ostream& operator<< (std::ostream& os, const as_value& v) {
	return os << v.to_string();
}

} // namespace gnash

#endif // GNASH_AS_VALUE_H

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

