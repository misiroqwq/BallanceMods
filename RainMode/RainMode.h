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


extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class RainMode : public IMod {
public:
	RainMode(IBML* bml) : IMod(bml) {}

	virtual ICKSTRING GetID() override { return "RainMode"; }
	virtual ICKSTRING GetVersion() override { return BML_VERSION; }
	virtual ICKSTRING GetName() override { return "RainMode"; }
	virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
	virtual ICKSTRING GetDescription() override { return "RainMode"; }
	DECLARE_BML_VERSION;
};