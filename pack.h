#ifndef __PACK_H
	#define __PACK_H

	/* Windows. */
	#ifdef WIN_SYSTEM
		/* GCC. */
		#ifdef __GNUC__
			#define PACK(D) D __attribute__((__packed__))

		/* Microsoft Compiler. */
		#elif _MSC_VER
			#define PACK(D) __pragma(pack(push, 1)) D __pragma(pack(pop))
		#endif

	/* Unix. */
	#elif UNIX_SYSTEM
		#define PACK(D) D __attribute__((__packed__))
	#endif

#endif
