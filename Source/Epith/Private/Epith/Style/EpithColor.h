// Unlicensed. This file is public domain.

#pragma once

#include "Math/Color.h"

#define HOVER		1.20f
#define PRESS		0.80f
#define SEMITRANS	0.90f
#define BARELYTRANS	0.95f
#define TRANS		0.70f
#define SEMIGLASS	0.45f
#define GLASS		0.20f
#define SUPERGLASS	0.05f

#define DECLARE_COLOR_TRAN(Name, RR, GG, BB, AA)\
inline FLinearColor Name = FLinearColor(RR, GG, BB, AA);\
inline FLinearColor Name##Hovered = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, AA);\
inline FLinearColor Name##Pressed = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, AA);\
inline FLinearColor Name##_BarelyTrans = FLinearColor(RR, GG, BB, BARELYTRANS);\
inline FLinearColor Name##_SemiTrans = FLinearColor(RR, GG, BB, SEMITRANS);\
inline FLinearColor Name##Hovered_SemiTrans = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, SEMITRANS * AA);\
inline FLinearColor Name##Pressed_SemiTrans = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, SEMITRANS * AA);\
inline FLinearColor Name##_Trans = FLinearColor(RR, GG, BB, TRANS);\
inline FLinearColor Name##Hovered_Trans = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, TRANS * AA);\
inline FLinearColor Name##Pressed_Trans = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, TRANS * AA);\
inline FLinearColor Name##_SemiGlass = FLinearColor(RR, GG, BB, SEMIGLASS);\
inline FLinearColor Name##Hovered_SemiGlass = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, SEMIGLASS * AA);\
inline FLinearColor Name##Pressed_SemiGlass = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, SEMIGLASS * AA);\
inline FLinearColor Name##_Glass = FLinearColor(RR, GG, BB, GLASS);\
inline FLinearColor Name##Hovered_Glass = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, GLASS * AA);\
inline FLinearColor Name##Pressed_Glass = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, GLASS * AA);\
inline FLinearColor Name##_SuperGlass = FLinearColor(RR, GG, BB, SUPERGLASS);\
inline FLinearColor Name##Hovered_SuperGlass = FLinearColor(HOVER * RR, HOVER * GG, HOVER * BB, SUPERGLASS * AA);\
inline FLinearColor Name##Pressed_SuperGlass = FLinearColor(PRESS * RR, PRESS * GG, PRESS * BB, SUPERGLASS * AA)

#define DECLARE_COLOR(Name, RR, GG, BB) DECLARE_COLOR_TRAN(Name, RR, GG, BB, 1.000f)

#define LOCTEXT_NAMESPACE "EpithEditor"

namespace EpithColor
{
	DECLARE_COLOR(LightOrange,		0.850f, 0.550f, 0.150f);
	DECLARE_COLOR(BrightOrange,		1.000f, 0.650f, 0.050f);
	DECLARE_COLOR(Orange,			0.780f, 0.380f, 0.050f);
	DECLARE_COLOR(DarkOrange,		0.280f, 0.180f, 0.040f);
	DECLARE_COLOR(DeepOrange,		0.100f, 0.050f, 0.010f);

	DECLARE_COLOR(LightYellow,		0.900f, 0.850f, 0.250f);
	DECLARE_COLOR(Yellow,			0.850f, 0.800f, 0.050f);
	DECLARE_COLOR(YellowGray,		0.500f, 0.450f, 0.250f);

	DECLARE_COLOR(OrangeRed,		0.920f, 0.070f, 0.020f);
	DECLARE_COLOR(DarkOrangeRed,	0.600f, 0.050f, 0.010f);
	DECLARE_COLOR(DeepOrangeRed,	0.300f, 0.030f, 0.005f);
	DECLARE_COLOR(OrangeRedGray,	0.300f, 0.130f, 0.050f);
	
	DECLARE_COLOR(LightRed,			0.950f, 0.340f, 0.420f);
	DECLARE_COLOR(Red,				0.950f, 0.040f, 0.060f);
	DECLARE_COLOR(DarkRed,			0.250f, 0.010f, 0.030f);
	DECLARE_COLOR(DeepRed,			0.050f, 0.010f, 0.020f);

	DECLARE_COLOR(LightGreen,		0.340f, 1.000f, 0.420f);
	DECLARE_COLOR(Green,			0.180f, 0.950f, 0.220f);
	DECLARE_COLOR(DarkGreen,		0.020f, 0.150f, 0.050f);
	DECLARE_COLOR(DarkGreenGray,	0.080f, 0.200f, 0.100f);
	DECLARE_COLOR(DeepGreen,		0.015f, 0.040f, 0.020f);

	DECLARE_COLOR(LightBlue,		0.340f, 0.420f, 1.000f);
	DECLARE_COLOR(BrightBlue,		0.020f, 0.350f, 0.750f);
	DECLARE_COLOR(Blue,				0.040f, 0.060f, 0.950f);
	DECLARE_COLOR(DimBlue,			0.030f, 0.040f, 0.350f);
	DECLARE_COLOR(DarkBlue,			0.010f, 0.010f, 0.100f);
	DECLARE_COLOR(DarkBlueGray,		0.100f, 0.080f, 0.200f);
	DECLARE_COLOR(DeepBlue,			0.010f, 0.010f, 0.500f);

	DECLARE_COLOR(LightCyan,		0.500f, 0.950f, 0.950f);
	DECLARE_COLOR(Cyan,		0.050f, 0.800f, 0.800f);
	
	DECLARE_COLOR(White,			1.000f, 1.000f, 1.000f);
	DECLARE_COLOR(DimWhite,			0.900f, 0.900f, 0.900f);
	DECLARE_COLOR(LightGray,		0.500f, 0.500f, 0.500f);
	DECLARE_COLOR(Gray,				0.200f, 0.200f, 0.200f);
	DECLARE_COLOR(DimGray,			0.100f, 0.100f, 0.100f);
	DECLARE_COLOR(DarkGray,			0.030f, 0.030f, 0.030f);
	DECLARE_COLOR(DeepGray,			0.010f, 0.010f, 0.010f);
	DECLARE_COLOR(Noir,				0.005f, 0.005f, 0.005f);
	DECLARE_COLOR(Black,			0.000f, 0.000f, 0.000f);

	DECLARE_COLOR_TRAN(Transparent,	1.000f, 1.000f, 1.000f, 0.000f);
	DECLARE_COLOR(Error,		1.000f, 0.000f, 1.000f);

	
	/** FLinearColor::Desaturate also lerps alpha towards zero for some reason... use my own instead. */
//	FLinearColor Desaturate(FLinearColor InColor, float Desaturation );

//	FLinearColor Darken(FLinearColor InColor, float Darken);


	// Other hardcoded values
	const FLinearColor WarningText = Yellow;
	
	inline const FLinearColor Button_Unset() { return DarkGray; }

	// Functions

	const FLinearColor GetNominalFontColor(FLinearColor NodeColor, FLinearColor DarkNodeFontColor, FLinearColor LightNodeFontColor); 
}

#undef LOCTEXT_NAMESPACE