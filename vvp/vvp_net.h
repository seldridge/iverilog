#ifndef __vvp_net_H
#define __vvp_net_H
/*
 * Copyright (c) 2004-2005 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ident "$Id: vvp_net.h,v 1.58 2007/06/12 02:36:58 steve Exp $"

# include  "config.h"
# include  <stddef.h>
# include  <assert.h>

#ifdef HAVE_IOSFWD
# include  <iosfwd>
#else
class ostream;
#endif

using namespace std;


/* Data types */
class  vvp_scalar_t;

/* Basic netlist types. */
class  vvp_net_t;
class  vvp_net_ptr_t;
class  vvp_net_fun_t;

/* Core net function types. */
class  vvp_fun_concat;
class  vvp_fun_drive;
class  vvp_fun_part;

class  vvp_delay_t;

/*
 * This is the set of Verilog 4-value bit values. Scalars have this
 * value along with strength, vectors are a collection of these
 * values. The enumeration has fixed numeric values that can be
 * expressed in 2 real bits, so that some of the internal classes can
 * pack them tightly.
 */
enum vvp_bit4_t {
      BIT4_0 = 0,
      BIT4_1 = 1,
      BIT4_X = 2,
      BIT4_Z = 3
};

extern vvp_bit4_t add_with_carry(vvp_bit4_t a, vvp_bit4_t b, vvp_bit4_t&c);

  /* Return TRUE if the bit is BIT4_X or BIT4_Z. The fast
     implementation here relies on the encoding of vvp_bit4_t values. */
inline bool bit4_is_xz(vvp_bit4_t a) { return a >= 2; }

  /* Some common boolean operators. These implement the Verilog rules
     for 4-value bit operations. */
extern vvp_bit4_t operator ~ (vvp_bit4_t a);
extern vvp_bit4_t operator & (vvp_bit4_t a, vvp_bit4_t b);
extern vvp_bit4_t operator | (vvp_bit4_t a, vvp_bit4_t b);
extern vvp_bit4_t operator ^ (vvp_bit4_t a, vvp_bit4_t b);
extern ostream& operator<< (ostream&o, vvp_bit4_t a);

  /* Return >0, ==0 or <0 if the from-to transition represents a
     posedge, no edge, or negedge. */
extern int edge(vvp_bit4_t from, vvp_bit4_t to);

/*
 * This class represents scalar values collected into vectors. The
 * vector values can be accessed individually, or treated as a
 * unit. in any case, the elements of the vector are addressed from
 * zero(LSB) to size-1(MSB).
 *
 * No strength values are stored here, if strengths are needed, use a
 * collection of vvp_scalar_t objects instead.
 */
class vvp_vector4_t {

    public:
      explicit vvp_vector4_t(unsigned size =0, vvp_bit4_t bits =BIT4_X);

	// Construct a vector4 from the subvalue of another vector4.
      explicit vvp_vector4_t(const vvp_vector4_t&that,
			     unsigned adr, unsigned wid);

      vvp_vector4_t(const vvp_vector4_t&that);
      vvp_vector4_t& operator= (const vvp_vector4_t&that);

      ~vvp_vector4_t();

      unsigned size() const { return size_; }
      void resize(unsigned new_size);

	// Get the bit at the specified address
      vvp_bit4_t value(unsigned idx) const;
	// Get the vector4 subvector starting at the address
      vvp_vector4_t subvalue(unsigned idx, unsigned size) const;
	// Get the 2-value bits for the subvector. This returns a new
	// array of longs, or a nil pointer if an XZ bit was detected
	// in the array.
      unsigned long*subarray(unsigned idx, unsigned size) const;

      void set_bit(unsigned idx, vvp_bit4_t val);
      void set_vec(unsigned idx, const vvp_vector4_t&that);

	// Test that the vectors are exactly equal
      bool eeq(const vvp_vector4_t&that) const;

	// Return true if there is an X or Z anywhere in the vector.
      bool has_xz() const;

	// Change all Z bits to X bits.
      void change_z2x();

	// Display the value into the buf as a string.
      char*as_string(char*buf, size_t buf_len);

      vvp_vector4_t& operator += (int64_t);

    private:
	// Number of vvp_bit4_t bits that can be shoved into a word.
      enum { BITS_PER_WORD = 8*sizeof(unsigned long)/2 };
#if SIZEOF_UNSIGNED_LONG == 8
      enum { WORD_0_BITS = 0x0000000000000000UL };
      enum { WORD_1_BITS = 0x5555555555555555UL };
      enum { WORD_X_BITS = 0xaaaaaaaaaaaaaaaaUL };
      enum { WORD_Z_BITS = 0xffffffffffffffffUL };
#elif SIZEOF_UNSIGNED_LONG == 4
      enum { WORD_0_BITS = 0x00000000UL };
      enum { WORD_1_BITS = 0x55555555UL };
      enum { WORD_X_BITS = 0xaaaaaaaaUL };
      enum { WORD_Z_BITS = 0xffffffffUL };
#else
#error "WORD_X_BITS not defined for this architecture?"
#endif

	// Initialize and operator= use this private method to copy
	// the data from that object into this object.
      void copy_from_(const vvp_vector4_t&that);

      void allocate_words_(unsigned size, unsigned long init);

      unsigned size_;
      union {
	    unsigned long bits_val_;
	    unsigned long*bits_ptr_;
      };
};

inline vvp_vector4_t::vvp_vector4_t(const vvp_vector4_t&that)
{
      copy_from_(that);
}

inline vvp_vector4_t::vvp_vector4_t(unsigned size, vvp_bit4_t val)
: size_(size)
{
	/* note: this relies on the bit encoding for the vvp_bit4_t. */
      const static unsigned long init_table[4] = {
	    WORD_0_BITS,
	    WORD_1_BITS,
	    WORD_X_BITS,
	    WORD_Z_BITS };

      allocate_words_(size, init_table[val]);
}

inline vvp_vector4_t::~vvp_vector4_t()
{
      if (size_ > BITS_PER_WORD) {
	    delete[] bits_ptr_;
      }
}

inline vvp_vector4_t& vvp_vector4_t::operator= (const vvp_vector4_t&that)
{
      if (this == &that)
	    return *this;

      if (size_ > BITS_PER_WORD)
	    delete[] bits_ptr_;

      copy_from_(that);

      return *this;
}


inline vvp_bit4_t vvp_vector4_t::value(unsigned idx) const
{
      if (idx >= size_)
	    return BIT4_X;

      unsigned wdx = idx / BITS_PER_WORD;
      unsigned long off = idx % BITS_PER_WORD;

      unsigned long bits;
      if (size_ > BITS_PER_WORD) {
	    bits = bits_ptr_[wdx];
      } else {
	    bits = bits_val_;
      }

      bits >>= (off * 2UL);

	/* Casting is evil, but this cast matches the un-cast done
	   when the vvp_bit4_t value is put into the vector. */
      return (vvp_bit4_t) (bits & 3);
}

inline vvp_vector4_t vvp_vector4_t::subvalue(unsigned adr, unsigned wid) const
{
      return vvp_vector4_t(*this, adr, wid);
}

inline void vvp_vector4_t::set_bit(unsigned idx, vvp_bit4_t val)
{
      assert(idx < size_);

      unsigned long off = idx % BITS_PER_WORD;
      unsigned long mask = 3UL << (2UL*off);

      if (size_ > BITS_PER_WORD) {
	    unsigned wdx = idx / BITS_PER_WORD;
	    bits_ptr_[wdx] &= ~mask;
	    bits_ptr_[wdx] |= (unsigned long)val << (2UL*off);
      } else {
	    bits_val_ &= ~mask;
	    bits_val_ |=  (unsigned long)val << (2UL*off);
      }
}

extern vvp_vector4_t operator ~ (const vvp_vector4_t&that);
extern ostream& operator << (ostream&, const vvp_vector4_t&);

extern vvp_bit4_t compare_gtge(const vvp_vector4_t&a,
			       const vvp_vector4_t&b,
			       vvp_bit4_t val_if_equal);
extern vvp_bit4_t compare_gtge_signed(const vvp_vector4_t&a,
				      const vvp_vector4_t&b,
				      vvp_bit4_t val_if_equal);
template <class T> extern T coerce_to_width(const T&that, unsigned width);

/*
 * These functions extract the value of the vector as a native type,
 * if possible, and return true to indicate success. If the vector has
 * any X or Z bits, the resulting value will have 0 bits in their
 * place (this follows the rules of Verilog conversions from vector4
 * to real and integers) and the return value becomes false to
 * indicate an error.
 */
extern bool vector4_to_value(const vvp_vector4_t&a, unsigned long&val);
extern bool vector4_to_value(const vvp_vector4_t&a, double&val, bool is_signed);

/* vvp_vector2_t
 */
class vvp_vector2_t {

      friend vvp_vector2_t operator + (const vvp_vector2_t&,
				       const vvp_vector2_t&);
      friend vvp_vector2_t operator * (const vvp_vector2_t&,
				       const vvp_vector2_t&);
      friend bool operator >  (const vvp_vector2_t&, const vvp_vector2_t&);
      friend bool operator >= (const vvp_vector2_t&, const vvp_vector2_t&);
      friend bool operator <  (const vvp_vector2_t&, const vvp_vector2_t&);
      friend bool operator <= (const vvp_vector2_t&, const vvp_vector2_t&);
      friend bool operator == (const vvp_vector2_t&, const vvp_vector2_t&);

    public:
      vvp_vector2_t();
      vvp_vector2_t(const vvp_vector2_t&);
      vvp_vector2_t(const vvp_vector2_t&, unsigned newsize);
	// Make a vvp_vector2_t from a vvp_vector4_t. If there are X
	// or Z bits, then the result becomes a NaN value.
      explicit vvp_vector2_t(const vvp_vector4_t&that);
	// Make from a native long and a specified width.
      vvp_vector2_t(unsigned long val, unsigned wid);
	// Make with the width, and filled with 1 or 0 bits.
      enum fill_t {FILL0, FILL1};
      vvp_vector2_t(fill_t fill, unsigned wid);
      ~vvp_vector2_t();

      vvp_vector2_t&operator += (const vvp_vector2_t&that);
      vvp_vector2_t&operator -= (const vvp_vector2_t&that);
      vvp_vector2_t&operator <<= (unsigned shift);
      vvp_vector2_t&operator >>= (unsigned shift);
      vvp_vector2_t&operator = (const vvp_vector2_t&);

      bool is_NaN() const;
      unsigned size() const;
      int value(unsigned idx) const;
      void set_bit(unsigned idx, int bit);

    private:
      enum { BITS_PER_WORD = 8 * sizeof(unsigned long) };
      unsigned long*vec_;
      unsigned wid_;

    private:
      void copy_from_that_(const vvp_vector2_t&that);
};

extern bool operator >  (const vvp_vector2_t&, const vvp_vector2_t&);
extern bool operator >= (const vvp_vector2_t&, const vvp_vector2_t&);
extern bool operator <  (const vvp_vector2_t&, const vvp_vector2_t&);
extern bool operator <= (const vvp_vector2_t&, const vvp_vector2_t&);
extern bool operator == (const vvp_vector2_t&, const vvp_vector2_t&);
extern vvp_vector2_t operator + (const vvp_vector2_t&, const vvp_vector2_t&);
extern vvp_vector2_t operator * (const vvp_vector2_t&, const vvp_vector2_t&);
extern vvp_vector2_t operator / (const vvp_vector2_t&, const vvp_vector2_t&);
extern vvp_vector2_t operator % (const vvp_vector2_t&, const vvp_vector2_t&);
extern vvp_vector4_t vector2_to_vector4(const vvp_vector2_t&, unsigned wid);
/* A c4string is of the form C4<...> where ... are bits. */
extern vvp_vector4_t c4string_to_vector4(const char*str);

extern ostream& operator<< (ostream&, const vvp_vector2_t&);

/*
 * This class represents a scaler value with strength. These are
 * heavier then the simple vvp_bit4_t, but more information is
 * carried by that weight.
 *
 * The strength values are as defined here:
 *   HiZ    - 0
 *   Small  - 1
 *   Medium - 2
 *   Weak   - 3
 *   Large  - 4
 *   Pull   - 5
 *   Strong - 6
 *   Supply - 7
 *
 * There are two strengths for a value: strength0 and strength1. If
 * the value is Z, then strength0 is the strength of the 0-value, and
 * strength of the 1-value. If the value is 0 or 1, then the strengths
 * are the range for that value.
 */
class vvp_scalar_t {

      friend vvp_scalar_t resolve(vvp_scalar_t a, vvp_scalar_t b);

    public:
	// Make a HiZ value.
      explicit vvp_scalar_t();

	// Make an unambiguous value.
      explicit vvp_scalar_t(vvp_bit4_t val, unsigned str);
      explicit vvp_scalar_t(vvp_bit4_t val, unsigned str0, unsigned str1);

	// Get the vvp_bit4_t version of the value
      vvp_bit4_t value() const;
      unsigned strength0() const;
      unsigned strength1() const;

      bool eeq(vvp_scalar_t that) const { return value_ == that.value_; }
      bool is_hiz() const { return value_ == 0; }

    private:
      unsigned char value_;
};

inline vvp_scalar_t::vvp_scalar_t()
{
      value_ = 0;
}

inline vvp_scalar_t::vvp_scalar_t(vvp_bit4_t val, unsigned str)
{
      assert(str <= 7);

      if (str == 0) {
	    value_ = 0x00;
      } else switch (val) {
	  case BIT4_0:
	    value_ = str | (str<<4);
	    break;
	  case BIT4_1:
	    value_ = str | (str<<4) | 0x88;
	    break;
	  case BIT4_X:
	    value_ = str | (str<<4) | 0x80;
	    break;
	  case BIT4_Z:
	    value_ = 0x00;
	    break;
      }
}

extern vvp_scalar_t resolve(vvp_scalar_t a, vvp_scalar_t b);
extern ostream& operator<< (ostream&, vvp_scalar_t);

/*
 * This class is a way to carry vectors of strength modeled
 * values. The 8 in the name is the number of possible distinct values
 * a well defined bit may have. When you add in ambiguous values, the
 * number of distinct values span the vvp_scalar_t.
 *
 * a vvp_vector8_t object can be created from a vvp_vector4_t and a
 * strength value. The vvp_vector8_t bits have the values of the input
 * vector, all with the strength specified. If no strength is
 * specified, then the conversion from bit4 to a scalar will use the
 * Verilog convention default of strong (6).
 */
class vvp_vector8_t {

    public:
      explicit vvp_vector8_t(unsigned size =0);
	// Make a vvp_vector8_t from a vector4 and a specified strength.
      vvp_vector8_t(const vvp_vector4_t&that, unsigned str =6);
      explicit vvp_vector8_t(const vvp_vector4_t&that,
			     unsigned str0,
			     unsigned str1);

      ~vvp_vector8_t();

      unsigned size() const { return size_; }
      vvp_scalar_t value(unsigned idx) const;
      void set_bit(unsigned idx, vvp_scalar_t val);

	// Test that the vectors are exactly equal
      bool eeq(const vvp_vector8_t&that) const;

      vvp_vector8_t(const vvp_vector8_t&that);
      vvp_vector8_t& operator= (const vvp_vector8_t&that);

    private:
      unsigned size_;
      vvp_scalar_t*bits_;
};

  /* Resolve uses the default Verilog resolver algorithm to resolve
     two drive vectors to a single output. */
extern vvp_vector8_t resolve(const vvp_vector8_t&a, const vvp_vector8_t&b);
  /* This function implements the strength reduction implied by
     Verilog standard resistive devices. */
extern vvp_vector8_t resistive_reduction(const vvp_vector8_t&a);
  /* The reduce4 function converts a vector8 to a vector4, losing
     strength information in the process. */
extern vvp_vector4_t reduce4(const vvp_vector8_t&that);
  /* Print a vector8 value to a stream. */
extern ostream& operator<< (ostream&, const vvp_vector8_t&);

inline vvp_vector8_t::~vvp_vector8_t()
{
      if (size_ > 0)
	    delete[]bits_;
}

inline vvp_scalar_t vvp_vector8_t::value(unsigned idx) const
{
      assert(idx < size_);
      return bits_[idx];
}

inline void vvp_vector8_t::set_bit(unsigned idx, vvp_scalar_t val)
{
      assert(idx < size_);
      bits_[idx] = val;
}

/*
 * This class implements a pointer that points to an item within a
 * target. The ptr() method returns a pointer to the vvp_net_t, and
 * the port() method returns a 0-3 value that selects the input within
 * the vvp_net_t. Use this pointer to point only to the inputs of
 * vvp_net_t objects. To point to vvp_net_t objects as a whole, use
 * vvp_net_t* pointers.
 */
class vvp_net_ptr_t {

    public:
      vvp_net_ptr_t();
      vvp_net_ptr_t(vvp_net_t*ptr, unsigned port);
      ~vvp_net_ptr_t() { }

      vvp_net_t* ptr();
      const vvp_net_t* ptr() const;
      unsigned  port() const;

      bool nil() const;

      bool operator == (vvp_net_ptr_t that) const;
      bool operator != (vvp_net_ptr_t that) const;

    private:
      unsigned long bits_;
};

/*
 * Alert! Ugly details. Protective clothing recommended!
 * The vvp_net_ptr_t encodes the bits of a C pointer, and two bits of
 * port identifer into an unsigned long. This works only if vvp_net_t*
 * values are always aligned on 4byte boundaries.
 */

inline vvp_net_ptr_t::vvp_net_ptr_t()
{
      bits_ = 0;
}

inline vvp_net_ptr_t::vvp_net_ptr_t(vvp_net_t*ptr, unsigned port)
{
      bits_ = reinterpret_cast<unsigned long> (ptr);
      assert( (bits_ & 3) == 0 );
      assert( (port & ~3) == 0 );
      bits_ |= port;
}

inline vvp_net_t* vvp_net_ptr_t::ptr()
{
      return reinterpret_cast<vvp_net_t*> (bits_ & ~3UL);
}

inline const vvp_net_t* vvp_net_ptr_t::ptr() const
{
      return reinterpret_cast<const vvp_net_t*> (bits_ & ~3UL);
}

inline unsigned vvp_net_ptr_t::port() const
{
      return bits_ & 3;
}

inline bool vvp_net_ptr_t::nil() const
{
      return bits_ == 0;
}

inline bool vvp_net_ptr_t::operator == (vvp_net_ptr_t that) const
{
      return bits_ == that.bits_;
}

inline bool vvp_net_ptr_t::operator != (vvp_net_ptr_t that) const
{
      return bits_ != that.bits_;
}


/*
 * This is the basic unit of netlist connectivity. It is a fan-in of
 * up to 4 inputs, and output pointer, and a pointer to the node's
 * functionality.
 *
 * A net output that is non-nil points to the input of one of its
 * destination nets. If there are multiple destinations, then the
 * referenced input port points to the next input. For example:
 *
 *   +--+--+--+--+---+
 *   |  |  |  |  | . |  Output from this vvp_net_t points to...
 *   +--+--+--+--+-|-+
 *                 |
 *                /
 *               /
 *              /
 *             |
 *             v
 *   +--+--+--+--+---+
 *   |  |  |  |  | . |  ... the fourth input of this vvp_net_t, and...
 *   +--+--+--+--+-|-+
 *             |   |
 *            /    .
 *           /     .
 *          |      .
 *          v
 *   +--+--+--+--+---+
 *   |  |  |  |  | . |  ... the third input of this vvp_net_t.
 *   +--+--+--+--+-|-+
 *
 * Thus, the fan-in of a vvp_net_t node is limited to 4 inputs, but
 * the fan-out is unlimited.
 *
 * The vvp_send_*() functions take as input a vvp_net_ptr_t and follow
 * all the fan-out chain, delivering the specified value.
 */
struct vvp_net_t {
      vvp_net_ptr_t port[4];
      vvp_net_ptr_t out;
      vvp_net_fun_t*fun;
      long fun_flags;
};

/*
 * Instances of this class represent the functionality of a
 * node. vvp_net_t objects hold pointers to the vvp_net_fun_t
 * associated with it. Objects of this type take inputs that arrive at
 * a port and perform some sort of action in response.
 *
 * Whenever a bit is delivered to a vvp_net_t object, the associated
 * vvp_net_fun_t::recv_*() method is invoked with the port pointer and
 * the bit value. The port pointer is used to figure out which exact
 * input receives the bit.
 *
 * In this context, a "bit" is the thing that arrives at a single
 * input. The bit may be a single data bit, a bit vector, various
 * sorts of numbers or aggregate objects.
 *
 * recv_vec4 is the most common way for a datum to arrive at a
 * port. The value is a vvp_vector4_t.
 *
 * Most nodes do not care about the specific strengths of bits, so the
 * default behavior for recv_vec8 is to reduce the operand to a
 * vvp_vector4_t and pass it on to the recv_vec4 method.
 */
class vvp_net_fun_t {

    public:
      vvp_net_fun_t();
      virtual ~vvp_net_fun_t();

      virtual void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
      virtual void recv_vec8(vvp_net_ptr_t port, vvp_vector8_t bit);
      virtual void recv_real(vvp_net_ptr_t port, double bit);
      virtual void recv_long(vvp_net_ptr_t port, long bit);

	// Part select variants of above
      virtual void recv_vec4_pv(vvp_net_ptr_t p, const vvp_vector4_t&bit,
				unsigned base, unsigned wid, unsigned vwid);

    private: // not implemented
      vvp_net_fun_t(const vvp_net_fun_t&);
      vvp_net_fun_t& operator= (const vvp_net_fun_t&);
};

/* **** Some core net functions **** */

/* vvp_fun_concat
 * This node function creates vectors (vvp_vector4_t) from the
 * concatenation of the inputs. The inputs (4) may be vector or
 * vector8 objects, but they are reduced to vector4 values and
 * strength information lost.
 *
 * The expected widths of the input vectors must be given up front so
 * that the positions in the output vector (and also the size of the
 * output vector) can be worked out. The input vectors must match the
 * expected width.
 */
class vvp_fun_concat  : public vvp_net_fun_t {

    public:
      vvp_fun_concat(unsigned w0, unsigned w1,
		     unsigned w2, unsigned w3);
      ~vvp_fun_concat();

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);

    private:
      unsigned wid_[4];
      vvp_vector4_t val_;
};

/* vvp_fun_repeat
 * This node function create vectors by repeating the input. The width
 * is the width of the output vector, and the repeat is the number of
 * times to repeat the input. The width of the input vector is
 * implicit from these values.
 */
class vvp_fun_repeat  : public vvp_net_fun_t {

    public:
      vvp_fun_repeat(unsigned width, unsigned repeat);
      ~vvp_fun_repeat();

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);

    private:
      unsigned wid_;
      unsigned rep_;
};

/* vvp_fun_drive
 * This node function takes an input vvp_vector4_t as input, and
 * repeats that value as a vvp_vector8_t with all the bits given the
 * strength of the drive. This is the way vvp_scaler8_t objects are
 * created. Input 0 is the value to be drived (vvp_vector4_t) and
 * inputs 1 and two are the strength0 and strength1 values to use. The
 * strengths may be taken as constant values, or adjusted as longs
 * from the network.
 *
 * This functor only propagates vvp_vector8_t values.
 */
class vvp_fun_drive  : public vvp_net_fun_t {

    public:
      vvp_fun_drive(vvp_bit4_t init, unsigned str0 =6, unsigned str1 =6);
      ~vvp_fun_drive();

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
	//void recv_long(vvp_net_ptr_t port, long bit);

    private:
      unsigned char drive0_;
      unsigned char drive1_;
};

/*
 * EXTEND functors expand an input vector to the desired output
 * width. The extend_signed functor sign extends the input. If the
 * input is already wider then the desired output, then it is passed
 * unmodified.
 */
class vvp_fun_extend_signed  : public vvp_net_fun_t {

    public:
      explicit vvp_fun_extend_signed(unsigned wid);
      ~vvp_fun_extend_signed();

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);

    private:
      unsigned width_;
};

/* vvp_fun_signal
 * This node is the place holder in a vvp network for signals,
 * including nets of various sort. The output from a signal follows
 * the type of its port-0 input. If vvp_vector4_t values come in
 * through port-0, then vvp_vector4_t values are propagated. If
 * vvp_vector8_t values come in through port-0, then vvp_vector8_t
 * values are propaged. Thus, this node is sloghtly polymorphic.
 *
 * If the signal is a net (i.e. a wire or tri) then this node will
 * have an input that is the data source. The data source will connect
 * through port-0
 *
 * If the signal is a reg, then there will be no netlist input, the
 * values will be written by behavioral statements. The %set and
 * %assign statements will write through port-0
 *
 * In any case, behavioral code is able to read the value that this
 * node last propagated, by using the value() method. That is important
 * functionality of this node.
 *
 * Continuous assignments are made through port-1. When a value is
 * written here, continuous assign mode is activated, and input
 * through port-0 is ignored until continuous assign mode is turned
 * off again. Writing into this port can be done in behavioral code
 * using the %cassign/v instruction, or can be done by the network by
 * hooking the output of a vvp_net_t to this port.
 *
 * Force assignments are made through port-2. When a value is written
 * here, force mode is activated. In force mode, port-0 data (or
 * port-1 data if in continuous assign mode) is tracked but not
 * propagated. The force value is propagated and is what is readable
 * through the value method.
 *
 * Port-3 is a command port, intended for use by procedural
 * instructions. The client must write long values to this port to
 * invoke the command of interest. The command values are:
 *
 *  1  -- deassign
 *          The deassign command takes the node out of continuous
 *          assignment mode. The output value is unchanged, and force
 *          mode, if active, remains in effect.
 *
 *  2  -- release/net
 *          The release/net command takes the node out of force mode,
 *          and propagates the tracked port-0 value to the signal
 *          output. This acts like a release of a net signal.
 *
 *  3  -- release/reg
 *          The release/reg command is similar to the release/net
 *          command, but the port-0 value is not propagated. Changes
 *          to port-0 (or port-1 if continuous assing is active) will
 *          propagate starting at the next input change.
 */

/*
* Things derived from vvp_vpi_callback may also be array'ed, so it
* includes some members that arrays use.
*/
class vvp_vpi_callback {

    public:
      vvp_vpi_callback();
      virtual ~vvp_vpi_callback();

      void run_vpi_callbacks();
      void add_vpi_callback(struct __vpiCallback*);

      virtual void get_value(struct t_vpi_value*value) =0;

      void attach_as_word(class __vpiArray* arr, unsigned long addr);

    private:
      struct __vpiCallback*vpi_callbacks_;
      class __vpiArray* array_;
      unsigned long array_word_;
};

class vvp_fun_signal_base : public vvp_net_fun_t, public vvp_vpi_callback {

    public:
      vvp_fun_signal_base();
      void recv_long(vvp_net_ptr_t port, long bit);

    public:

	/* The %force/link instruction needs a place to write the
	   source node of the force, so that subsequent %force and
	   %release instructions can undo the link as needed. */
      struct vvp_net_t*force_link;
      struct vvp_net_t*cassign_link;

    protected:

	// This is true until at least one propagation happens.
      bool needs_init_;
      bool continuous_assign_active_;
      vvp_vector2_t force_mask_;

      void deassign();
      virtual void release(vvp_net_ptr_t ptr, bool net) =0;
};

/*
 * This abstract class is a little more specific the the signa_base
 * class, in that in adds vector access methods.
 */
class vvp_fun_signal_vec : public vvp_fun_signal_base {

    public:
	// For vector signal types, this returns the vector count.
      virtual unsigned size() const =0;
      virtual vvp_bit4_t value(unsigned idx) const =0;
      virtual vvp_scalar_t scalar_value(unsigned idx) const =0;
      virtual vvp_vector4_t vec4_value() const =0;
};

class vvp_fun_signal  : public vvp_fun_signal_vec {

    public:
      explicit vvp_fun_signal(unsigned wid, vvp_bit4_t init=BIT4_X);

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
      void recv_vec8(vvp_net_ptr_t port, vvp_vector8_t bit);

	// Part select variants of above
      void recv_vec4_pv(vvp_net_ptr_t port, const vvp_vector4_t&bit,
			unsigned base, unsigned wid, unsigned vwid);

	// Get information about the vector value.
      unsigned   size() const;
      vvp_bit4_t value(unsigned idx) const;
      vvp_scalar_t scalar_value(unsigned idx) const;
      vvp_vector4_t vec4_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);

      void get_value(struct t_vpi_value*value);

    private:
      void calculate_output_(vvp_net_ptr_t ptr);

      vvp_vector4_t bits4_;
      vvp_vector4_t force_;
};

class vvp_fun_signal8  : public vvp_fun_signal_vec {

    public:
      explicit vvp_fun_signal8(unsigned wid);

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
      void recv_vec8(vvp_net_ptr_t port, vvp_vector8_t bit);

	// Part select variants of above
	//void recv_vec4_pv(vvp_net_ptr_t port, const vvp_vector4_t&bit,
	//			unsigned base, unsigned wid, unsigned vwid);

	// Get information about the vector value.
      unsigned   size() const;
      vvp_bit4_t value(unsigned idx) const;
      vvp_scalar_t scalar_value(unsigned idx) const;
      vvp_vector4_t vec4_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);

      void get_value(struct t_vpi_value*value);

    private:
      void calculate_output_(vvp_net_ptr_t ptr);

      vvp_vector8_t bits8_;
      vvp_vector8_t force_;
};

class vvp_fun_signal_real  : public vvp_fun_signal_base {

    public:
      explicit vvp_fun_signal_real();

	//void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
      void recv_real(vvp_net_ptr_t port, double bit);

	// Get information about the vector value.
      double real_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);

      void get_value(struct t_vpi_value*value);

    private:
      double bits_;
      double force_;
};

/*
 * Wide Functors:
 * Wide functors represent special devices that may have more than 4
 * input ports. These devices need a set of N/4 actual functors to
 * catch the inputs, and use another to deliver the output.
 *
 *  vvp_wide_fun_t --+--> vvp_wide_fun_core --> ...
 *                   |
 *  vvp_wide_fun_t --+
 *                   |
 *  vvp_wide_fun_t --+
 *
 * There are enough input functors to take all the functor inputs, 4
 * per functor. These inputs deliver the changed input value to the
 * wide_fun_core, which carries the infastructure for the thread. The
 * wide_fun_core is also a functor whose output is connected to the rest
 * of the netlist. This is where the result is delivered back to the
 * netlist.
 *
 * The vvp_wide_fun_core keeps a store of the inputs from all the
 * input functors, and makes them available to the derived class that
 * does the processing.
 */

class vvp_wide_fun_core : public vvp_net_fun_t {

    public:
      vvp_wide_fun_core(vvp_net_t*net, unsigned nports);
      virtual ~vvp_wide_fun_core();

    protected:
      void propagate_vec4(const vvp_vector4_t&bit, vvp_time64_t delay =0);
      unsigned port_count() const;

      vvp_vector4_t& value(unsigned);
      double value_r(unsigned);

    private:
	// the derived class implements this to receive an indication
	// that one of the port input values changed.
      virtual void recv_vec4_from_inputs(unsigned port) =0;
      virtual void recv_real_from_inputs(unsigned port);

      friend class vvp_wide_fun_t;
      void dispatch_vec4_from_input_(unsigned port, vvp_vector4_t bit);
      void dispatch_real_from_input_(unsigned port, double bit);

    private:
	// Back-point to the vvp_net_t that points to me.
      vvp_net_t*ptr_;
	// Structure to track the input values from the input functors.
      unsigned nports_;
      vvp_vector4_t*port_values_;
      double*port_rvalues_;
};

/*
 * The job of the input functor is only to monitor inputs to the
 * function and pass them to the ufunc_core object. Each functor takes
 * up to 4 inputs, with the base the port number for the first
 * function input that this functor represents.
 */
class vvp_wide_fun_t : public vvp_net_fun_t {

    public:
      vvp_wide_fun_t(vvp_wide_fun_core*c, unsigned base);
      ~vvp_wide_fun_t();

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit);
      void recv_real(vvp_net_ptr_t port, double bit);

    private:
      vvp_wide_fun_core*core_;
      unsigned port_base_;
};


inline void vvp_send_vec4(vvp_net_ptr_t ptr, const vvp_vector4_t&val)
{
      while (struct vvp_net_t*cur = ptr.ptr()) {
	    vvp_net_ptr_t next = cur->port[ptr.port()];

	    if (cur->fun)
		  cur->fun->recv_vec4(ptr, val);

	    ptr = next;
      }
}

extern void vvp_send_vec8(vvp_net_ptr_t ptr, vvp_vector8_t val);
extern void vvp_send_real(vvp_net_ptr_t ptr, double val);
extern void vvp_send_long(vvp_net_ptr_t ptr, long val);

/*
 * Part-vector versions of above functions. This function uses the
 * corresponding recv_vec4_pv method in the vvp_net_fun_t functor to
 * deliver parts of a vector.
 *
 * The ptr is the destination input port to write to.
 *
 * <val> is the vector to be written. The width of this vector must
 * exactly match the <wid> vector.
 *
 * The <base> is where in the receiver the bit vector is to be
 * written. This address is given in canonical units; 0 is the LSB, 1
 * is the next bit, and so on.
 *
 * The <vwid> is the width of the destination vector that this part is
 * part of. This is used by intermediate nodes, i.e. resolvers, to
 * know how wide to pad with Z, if it needs to transform the part to a
 * mirror of the destination vector.
 */
inline void vvp_send_vec4_pv(vvp_net_ptr_t ptr, const vvp_vector4_t&val,
		      unsigned base, unsigned wid, unsigned vwid)
{
      while (struct vvp_net_t*cur = ptr.ptr()) {
	    vvp_net_ptr_t next = cur->port[ptr.port()];

	    if (cur->fun)
		  cur->fun->recv_vec4_pv(ptr, val, base, wid, vwid);

	    ptr = next;
      }
}

#endif