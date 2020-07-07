#pragma once

#ifdef MAKE_SURE
#define make_sure(condition, reporter, ...) do {	\
			if(!(condition)) { reporter(__VA_ARGS__); }		\
		} while(0)
#else
#define make_sure(condition, reporter, ...)
#endif
	 
