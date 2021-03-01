#include "Primitives.h"

#if defined (__GNUC__) && defined (__unix__)
	// Fix 'LONG' doest not name a type
	uint32 max(uint16 a, uint32& b) { return std::max(uint32(a),b);}
	uint32 min(uint32& a, int b) {return std::min(a,(uint32)b);}

#endif // defined (__GNUC__) && defined (__unix__)
