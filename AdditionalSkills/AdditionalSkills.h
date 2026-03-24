#pragma once
#ifdef USING_BML_PLUS
# include <BML/BMLAll.h>
# ifndef m_bml
#  define m_bml m_BML
#  define m_sprite m_Sprite
#  define VT21_REF(x) &(x)
# endif
typedef const char* ICKSTRING;
#else
# include <BML/BMLAll.h>
# define VT21_REF(x) (x)
typedef CKSTRING ICKSTRING;
#endif