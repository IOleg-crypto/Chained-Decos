#ifndef CH_MOUSE_CODES_H
#define CH_MOUSE_CODES_H

namespace CHEngine
{
	using MouseCode = uint16_t;

	namespace Mouse
	{
		enum : MouseCode
		{
			// From raylib.h
			ButtonLeft      = 0,       // Mouse button left
			ButtonRight     = 1,       // Mouse button right
			ButtonMiddle    = 2,       // Mouse button middle
			ButtonSide      = 3,       // Mouse button side (for mice with 4+ buttons)
			ButtonExtra     = 4,       // Mouse button extra (for mice with 5+ buttons)
			ButtonForward   = 5,       // Mouse button forward (for mice with 6+ buttons)
			ButtonBack      = 6,       // Mouse button back (for mice with 7+ buttons)
		};
	}
}

#endif // CH_MOUSE_CODES_H
