/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

/** @file
 *
 * Mathematical self-tests
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <string.h>
#include <strings.h>
#include <assert.h>
#include <ipxe/test.h>
#include <ipxe/isqrt.h>

/**
 * Force a call to the non-constant implementation of flsl()
 *
 * @v value		Value
 * @ret msb		Most significant bit set in value (LSB=1), or zero
 */
__attribute__ (( noinline )) int flsl_var ( long value ) {
	return flsl ( value );
}

/**
 * Check current stack pointer
 *
 * @ret stack		A value at a fixed offset from the current stack pointer
 *
 * Used by check_divmod()
 */
static __attribute__ (( noinline )) void * stack_check ( void ) {
	int a;
	void *ret;

	/* Hide the fact that we are returning the address of a local
	 * variable, to prevent a compiler warning.
	 */
	__asm__ ( "\n" : "=g" ( ret ) : "0" ( &a ) );

	return ret;
}

/**
 * Check division/modulus operation
 *
 * One aspect of the calling convention for the implicit arithmetic
 * functions (__udivmoddi4() etc) is whether the caller or the callee
 * is expected to pop any stack-based arguments.  This distinction can
 * be masked if the compiler chooses to uses a frame pointer in the
 * caller, since the caller will then reload the stack pointer from
 * the frame pointer and so can mask an error in the value of the
 * stack pointer.
 *
 * We run the division operation in a loop, and check that the stack
 * pointer does not change value on the second iteration.  To prevent
 * the compiler from performing various optimisations which might
 * invalidate our intended test (such as unrolling the loop, or moving
 * the division operation outside the loop), we include some dummy
 * inline assembly code.
 */
#define check_divmod( dividend, divisor, OP ) ( {			\
	uint64_t result;						\
	int count = 2;							\
	void *check = NULL;						\
									\
	/* Prevent compiler from unrolling the loop */			\
	__asm__ ( "\n" : "=g" ( count ) : "0" ( count ) );		\
									\
	do {								\
		/* Check that stack pointer does not change between	\
		 * loop iterations.					\
		 */							\
		if ( check ) {						\
			assert ( check == stack_check() );		\
		} else {						\
			check = stack_check();				\
		}							\
									\
		/* Perform division, preventing the compiler from	\
		 * moving the division out of the loop.			\
		 */							\
		__asm__ ( "\n" : "=g" ( dividend ), "=g" ( divisor )	\
			  : "0" ( dividend ), "1" ( divisor ) );	\
	        result = ( dividend OP divisor );			\
		__asm__ ( "\n" : "=g" ( result ) : "0" ( result ) );	\
									\
	} while ( --count );						\
	result; } )

/**
 * Force a use of runtime 64-bit unsigned integer division
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @ret quotient	Quotient
 */
__attribute__ (( noinline )) uint64_t u64div_var ( uint64_t dividend,
						   uint64_t divisor ) {

	return check_divmod ( dividend, divisor, / );
}

/**
 * Force a use of runtime 64-bit unsigned integer modulus
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @ret remainder	Remainder
 */
__attribute__ (( noinline )) uint64_t u64mod_var ( uint64_t dividend,
						   uint64_t divisor ) {

	return check_divmod ( dividend, divisor, % );
}

/**
 * Force a use of runtime 64-bit signed integer division
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @ret quotient	Quotient
 */
__attribute__ (( noinline )) int64_t s64div_var ( int64_t dividend,
						  int64_t divisor ) {

	return check_divmod ( dividend, divisor, / );
}

/**
 * Force a use of runtime 64-bit unsigned integer modulus
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @ret remainder	Remainder
 */
__attribute__ (( noinline )) int64_t s64mod_var ( int64_t dividend,
						  int64_t divisor ) {

	return check_divmod ( dividend, divisor, % );
}

/**
 * Report a flsl() test result
 *
 * @v value		Value
 * @v msb		Expected MSB
 * @v file		Test code file
 * @v line		Test code line
 */
static inline __attribute__ (( always_inline )) void
flsl_okx ( long value, int msb, const char *file, unsigned int line ) {

	/* Verify as a constant (requires to be inlined) */
	okx ( flsl ( value ) == msb, file, line );

	/* Verify as a non-constant */
	okx ( flsl_var ( value ) == msb, file, line );
}
#define flsl_ok( value, msb ) flsl_okx ( value, msb, __FILE__, __LINE__ )

/**
 * Report a 64-bit unsigned integer division test result
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @v quotient		Quotient
 * @v remainder		Remainder
 * @v file		Test code file
 * @v line		Test code line
 */
static void u64divmod_okx ( uint64_t dividend, uint64_t divisor,
			    uint64_t quotient, uint64_t remainder,
			    const char *file, unsigned int line ) {

	/* Sanity check */
	okx ( ( ( divisor * quotient ) + remainder ) == dividend, file, line );

	/* Check division */
	okx ( u64div_var ( dividend, divisor ) == quotient, file, line );

	/* Check modulus */
	okx ( u64mod_var ( dividend, divisor ) == remainder, file, line );
}
#define u64divmod_ok( dividend, divisor, quotient, remainder )	\
	u64divmod_okx ( dividend, divisor, quotient, remainder,	\
			__FILE__, __LINE__ )

/**
 * Report a 64-bit signed integer division test result
 *
 * @v dividend		Dividend
 * @v divisor		Divisor
 * @v quotient		Quotient
 * @v remainder		Remainder
 * @v file		Test code file
 * @v line		Test code line
 */
static void s64divmod_okx ( int64_t dividend, int64_t divisor,
			    int64_t quotient, int64_t remainder,
			    const char *file, unsigned int line ) {

	/* Sanity check */
	okx ( ( ( divisor * quotient ) + remainder ) == dividend, file, line );

	/* Check division */
	okx ( s64div_var ( dividend, divisor ) == quotient, file, line );

	/* Check modulus */
	okx ( s64mod_var ( dividend, divisor ) == remainder, file, line );
}
#define s64divmod_ok( dividend, divisor, quotient, remainder )	\
	s64divmod_okx ( dividend, divisor, quotient, remainder,	\
			__FILE__, __LINE__ )

/**
 * Perform mathematical self-tests
 *
 */
static void math_test_exec ( void ) {

	/* Test flsl() */
	flsl_ok ( 0, 0 );
	flsl_ok ( 1, 1 );
	flsl_ok ( 255, 8 );
	flsl_ok ( 256, 9 );
	flsl_ok ( 257, 9 );
	flsl_ok ( 0x69505845, 31 );
	flsl_ok ( -1U, ( 8 * sizeof ( int ) ) );
	flsl_ok ( -1UL, ( 8 * sizeof ( long ) ) );

	/* Test 64-bit arithmetic
	 *
	 * On a 64-bit machine, these tests are fairly meaningless.
	 *
	 * On a 32-bit machine, these tests verify the correct
	 * operation of our libgcc functions __udivmoddi4()
	 * etc. (including checking that the implicit calling
	 * convention assumed by gcc matches our expectations).
	 */
	u64divmod_ok ( 0x2b90ddccf699f765ULL, 0xed9f5e73ULL,
		       0x2eef6ab4ULL, 0x0e12f089ULL );
	s64divmod_ok ( 0x2b90ddccf699f765ULL, 0xed9f5e73ULL,
		       0x2eef6ab4ULL, 0x0e12f089ULL );
	u64divmod_ok ( 0xc09e00dcb9e34b54ULL, 0x35968185cdc744f3ULL,
		       3, 0x1fda7c4b508d7c7bULL );
	s64divmod_ok ( -0x3f61ff23461cb4acLL, 0x35968185cdc744f3ULL,
		       -1LL, -0x9cb7d9d78556fb9LL );
	u64divmod_ok ( 0, 0x5b2f2737f4ffULL, 0, 0 );
	s64divmod_ok ( 0, 0xbb00ded72766207fULL, 0, 0 );

	/* Test integer square root */
	ok ( isqrt ( 0 ) == 0 );
	ok ( isqrt ( 1 ) == 1 );
	ok ( isqrt ( 255 ) == 15 );
	ok ( isqrt ( 256 ) == 16 );
	ok ( isqrt ( 257 ) == 16 );
	ok ( isqrt ( 0xa53df2adUL ) == 52652 );
	ok ( isqrt ( 0x123793c6UL ) == 17482 );
	ok ( isqrt ( -1UL ) == ( -1UL >> ( 8 * sizeof ( unsigned long ) / 2 )));
}

/** Mathematical self-tests */
struct self_test math_test __self_test = {
	.name = "math",
	.exec = math_test_exec,
};