// Unlicensed. This file is public domain.

#include "Epith/Style/EpithStyle.h"

#include "Developer/Windows/LiveCoding/Public/ILiveCodingModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Epith/Style/EpithColor.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SegmentedControlStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "Epith"

TArray<TStrongObjectPtr<UTexture2D>> FEpithStyle::Textures;
FDelegateHandle FEpithStyle::OnPatchCompleteHandle;
TSharedPtr<FSlateStyleSet> FEpithStyle::StyleInstance = nullptr;

//FEpithFonts EpithFonts;
FEpithBrushes EpithBrushes;
FEpithStyles EpithStyles;
FEpithFonts EpithFonts;

// ================================================================================================

#define EPITH_QUOTE(X) #X

// ------------------------------------------------------------------------------------------------

/** Makes a simple font definition copying default font */
#define EPITH_DEFINE_FONT(NAME, STYLE, SIZE)\
	EpithFonts.NAME = DEFAULT_FONT(STYLE, SIZE);\
	FSlateFontInfo& NAME = EpithFonts.NAME

// ------------------------------------------------------------------------------------------------

/** Define a new brush */
#define EPITH_DEFINE_BRUSH(TYPE, BRUSHNAME, FILENAME, EXTENSION, ...)\
	EpithBrushes.BRUSHNAME = EPITH_QUOTE(BRUSHNAME);\
	StyleInstance->Set(EPITH_QUOTE(BRUSHNAME), new TYPE(StyleInstance->RootToContentDir(FILENAME, TEXT(EXTENSION)), __VA_ARGS__));\
	const TYPE& BRUSHNAME = *static_cast<const TYPE*>(StyleInstance->GetBrush(EPITH_QUOTE(BRUSHNAME)))

// ------------------------------------------------------------------------------------------------

/** Define a new style */
#define EPITH_DEFINE_STYLE(TYPE, STYLENAME, TEMPLATE, MODS)\
	EpithStyles.STYLENAME = EPITH_QUOTE(STYLENAME);\
	StyleInstance->Set(EPITH_QUOTE(STYLENAME), TYPE(TEMPLATE));\
	TYPE& STYLENAME = const_cast<TYPE&>(StyleInstance->GetWidgetStyle<TYPE>(EPITH_QUOTE(STYLENAME)));\
	STYLENAME MODS

// ------------------------------------------------------------------------------------------------

/** Used to copy an existing UE brush into Epith style for easier use */
#define EPITH_REDEFINE_UE_BRUSH(TYPE, EpithNAME, UESTYLESET, UENAME, ...)\
	EpithBrushes.EpithNAME = EPITH_QUOTE(EpithNAME);\
	const TYPE& EpithNAME = *(new TYPE(UESTYLESET::GetBrush(UENAME)->GetResourceName().ToString(), __VA_ARGS__));\
	StyleInstance->Set(EPITH_QUOTE(EpithNAME), const_cast<TYPE*>(&EpithNAME))

// ================================================================================================

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define BOX_BRUSH_SVG( RelativePath, ... ) FSlateVectorBoxBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define BORDER_BRUSH_SVG( RelativePath, ... ) FSlateVectorBorderBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

#define CORE_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

#define CORE_IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define CORE_BOX_BRUSH_SVG( RelativePath, ... ) FSlateVectorBoxBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define CORE_BORDER_BRUSH_SVG( RelativePath, ... ) FSlateVectorBorderBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

// ================================================================================================


ISlateStyle& FEpithStyle::Get()
{
	// The style is created in FEpithModule::StartupModule. This is only a fallback; creating it lazily mid-frame is what we want to avoid.
	if (!StyleInstance.IsValid())
	{
		Initialize();
	}

	return *StyleInstance;
}

const FSlateBrush* FEpithStyle::GetImageBrush(FName BrushName)
{
	return Get().GetBrush(BrushName);
}

const FSlateBrush* FEpithStyle::GetBorderBrush(uint8 Border, bool bOpenTop)
{
	switch (Border)
	{
	case 1:
		return GetImageBrush(bOpenTop ? EpithBrushes.Border_2px_Deburred_U : EpithBrushes.Border_2px_Deburred);
	case 2:
		return GetImageBrush(bOpenTop ? EpithBrushes.Border_3px_Deburred_U : EpithBrushes.Border_3px_Deburred);
	default:
		return FAppStyle::GetBrush("NoBorder");
	}
}

FMargin FEpithStyle::GetBorderPadding(uint8 Border, bool bOpenTop)
{
	// Thin is 1 solid + 1 half-solid pixel, thick is 2 solid + 1 half-solid
	const float Thickness = Border == 1 ? 2.0f : Border == 2 ? 3.0f : 0.0f;

	return FMargin(Thickness, bOpenTop ? 0.0f : Thickness, Thickness, Thickness);
}

FEpithStyle::FEpithStyle()
{

}

FEpithStyle::~FEpithStyle()
{
}

FName FEpithStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("EpithEditorStyle"));
	return StyleSetName;
}

void FEpithStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	// No Slate in commandlets (cooking etc.), and FAppStyle may not exist yet.
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	StyleInstance = Create();

	// Brushes are registered before the style set is, and their textures load lazily on first draw.
	// Do NOT call ReloadTextureResources() here: it destroys every Slate texture in the editor, and if this runs mid-frame
	// the render thread crashes drawing batches that still point at the destroyed textures.
	SetupStyles();

	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);

#if WITH_LIVE_CODING
	if (ILiveCodingModule* LiveCoding = FModuleManager::LoadModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		OnPatchCompleteHandle = LiveCoding->GetOnPatchCompleteDelegate().AddStatic(&FEpithStyle::OnPatchComplete);
	}
#endif
}

void FEpithStyle::Shutdown()
{
#if WITH_LIVE_CODING
	if (ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		LiveCoding->GetOnPatchCompleteDelegate().Remove(OnPatchCompleteHandle);
	}
#endif
	OnPatchCompleteHandle.Reset();

	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		StyleInstance.Reset();
	}

	Textures.Empty();
}

TSharedRef<class FSlateStyleSet> FEpithStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetParentStyleName(FAppStyle::GetAppStyleSetName());
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("Epith")->GetBaseDir() / TEXT("Resources"));
	Style->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));

	return Style;
}

const ISlateStyle* FEpithStyle::GetParentStyle()
{
	return &FAppStyle::Get();
}

#define EPITH_COMMON_BRUSH "Common/ButtonHoverHint"
#define EPITH_COMMON_MARGIN FMargin(4.0 / 16.0)
#define EPITH_COMMON_PRESSED_PADDING FMargin(0, 1, 0, -1) // Push down by one pixel
#define EPITH_COMMON_CHECKBOXSTYLE FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox")


void FEpithStyle::SetupStyles()
{
	// ============================================================================================
	// FONTS
	// Format: EPITH_DEFINE_FONT(FName DefinitionName, FString StyleName, int Size);
	// Sample: EPITH_DEFINE_FONT(Font_Basic, "Normal", 10);
	// ============================================================================================
	EPITH_DEFINE_FONT(Font_ViewportPopup_PanelTitle, "Bold", 13);
	EPITH_DEFINE_FONT(Font_ViewportPopup_Title, "Bold", 10);

	// ============================================================================================
	// BRUSHES
	// Format: EPITH_DEFINE_BRUSH(TypeName, FNameIcon_AudioTime, "DialogueNodeIcons/AudioTime", ".png", FVector2f(16, 16));
	// Sample: EPITH_DEFINE_BRUSH(FSlateImageBrush, Icon_AudioTime, "DialogueNodeIcons/AudioTime", ".png", FVector2f(16, 16));
	// ============================================================================================
	// EPITH_DEFINE_BRUSH(FSlateImageBrush,			Icon_AudioTime,					"DialogueNodeIcons/AudioTime", ".png",	FVector2f(16, 16));

	// ============================================================================================
	// BRUSHES - SVGs
	// ============================================================================================
	//	EPITH_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_Chevron_Right,				"Icon_Chevron_Right", ".svg",			FVector2f(16, 16), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBorderBrush,		Border_2px_Deburred,				"Border_2px_Deburred", ".png",					FMargin(0.5f, 0.5f), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBorderBrush,		Border_3px_Deburred,				"Border_3px_Deburred", ".png",					FMargin(0.5f, 0.5f), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBorderBrush,		Border_2px_Deburred_U,				"Border_2px_Deburred_U", ".png",				FMargin(0.5f, 0.5f), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBorderBrush,		Border_3px_Deburred_U,				"Border_3px_Deburred_U", ".png",				FMargin(0.5f, 0.5f), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBoxBrush,			Border_TabShape,					"Border_TabShape", ".png",						FMargin(0.5f, 0.5f), EpithColor::White);

	EPITH_DEFINE_BRUSH(FSlateBoxBrush,			Background_WhiteSquare,				"WhiteSquare", ".png",							FMargin(0.5f, 0.5f), EpithColor::White);
	EPITH_DEFINE_BRUSH(FSlateBoxBrush,			Background_ViewportMasterPanelHeader,	"Background_ViewportMasterPanelHeader", ".png",	FMargin(10.f/16.f, 4.f/16.f, 4.f/16.f, 12.f/16.f), EpithColor::White);

	// ============================================================================================
	// SLIDER STYLES
	// ============================================================================================

	/*
	EPITH_DEFINE_STYLE(FSliderStyle, SliderStyle_FragmentTimePadding, FSliderStyle::GetDefault(),
		.SetBarThickness(0.f)
		.SetNormalThumbImage(IMAGE_BRUSH("ProgressBar_Fill", CoreStyleConstants::Icon8x8, EpithColor::Gray))
		.SetHoveredThumbImage(IMAGE_BRUSH("ProgressBar_Fill", CoreStyleConstants::Icon8x8, EpithColor::LightGray))
	);
	*/

	// ============================================================================================
	// BUTTON STYLES
	// ============================================================================================

	// ============================================================================================
	// COMBO BUTTON STYLES
	// ============================================================================================

	/*
	EPITH_DEFINE_STYLE(FComboButtonStyle, ComboButtonStyle_EpithGameplayTagTypedPicker, FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton"),
		.SetButtonStyle(ButtonStyle_TagButton)
		.SetDownArrowPadding(FMargin(0, 2, 0, 0))
		.SetDownArrowAlignment(VAlign_Top)
		);
	*/

	// ============================================================================================
	// CHECKBOX STYLES
	// ============================================================================================

	EPITH_DEFINE_STYLE(FCheckBoxStyle, CheckBoxStyle_Tab, EPITH_COMMON_CHECKBOXSTYLE,
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(FSlateNoResource())
		.SetUncheckedHoveredImage(Border_TabShape)
		.SetUncheckedPressedImage(Border_TabShape)
		.SetCheckedImage(Border_TabShape)
		.SetCheckedHoveredImage(Border_TabShape)
		.SetCheckedPressedImage(Border_TabShape)
		.SetForegroundColor(FStyleColors::Foreground)
		.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
		.SetPressedForegroundColor(FStyleColors::ForegroundHover)
		.SetCheckedForegroundColor(FStyleColors::Primary)
		.SetCheckedHoveredForegroundColor(FStyleColors::Primary)
		.SetCheckedPressedForegroundColor(FStyleColors::Primary)
		.SetBorderBackgroundColor(EpithColor::DeepGray_BarelyTrans)
		.SetPadding(FMargin(12.f, 6.f));

		/*
		.SetCheckedImage(Icon_Switch_On)
		.SetCheckedHoveredImage(Icon_Switch_On)
		.SetCheckedPressedImage(Icon_Switch_On)

		.SetUncheckedImage(Icon_Switch_Off)
		.SetUncheckedHoveredImage(Icon_Switch_Off)
		.SetUncheckedPressedImage(Icon_Switch_Off)
		*/
	);

	// ============================================================================================
	// SCROLLBAR STYLES
	// ============================================================================================

	/*
	EPITH_DEFINE_STYLE(FScrollBarStyle, ScrollBarStyle_DialogueBox, FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"),
		.SetThickness(2)
		.SetHorizontalBackgroundImage(Box_SolidBlack)
		.SetHorizontalBottomSlotImage(Box_SolidWhite)
		.SetDraggedThumbImage(Box_SolidWhite)
		.SetHoveredThumbImage(Box_SolidWhite)
		.SetNormalThumbImage(Box_SolidLightGray)
	);
	*/

	// ============================================================================================
	// TEXT BLOCK STYLES
	// ============================================================================================

	/*
	EPITH_DEFINE_STYLE(FTextBlockStyle, TextBlockStyle_DialogueText, GetParentStyle()->GetWidgetStyle<FTextBlockStyle>("NormalText"),
		.SetFont(Font_DialogueText)
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetFontSize(10)
	);
	*/

	int32 DefaultSize = FCoreStyle::RegularTextSize + 2;


	//StyleInstance->Set("RichTextBlock.TextHighlight", FTextBlockStyle(TextStyle_Normal));

	//StyleInstance->Set("RichTextBlock.Bold", FTextBlockStyle(TextStyle_Normal));

	//StyleInstance->Set("RichTextBlock.BoldHighlight", FTextBlockStyle(TextStyle_Normal));


	// ============================================================================================
	// EDITABLE TEXT BLOCK STYLES
	// ============================================================================================

	// ============================================================================================
	// PROGRESS BAR STYLES
	// ============================================================================================

	// ============================================================================================
	// HYPERLINK STYLES
	// ============================================================================================

	// ============================================================================================
	// OTHER
	// ============================================================================================
	EPITH_DEFINE_STYLE(FSegmentedControlStyle, SegmentedControlStyle_Tab, FSegmentedControlStyle(),
		.SetControlStyle(CheckBoxStyle_Tab)
		.SetFirstControlStyle(CheckBoxStyle_Tab)
		.SetLastControlStyle(CheckBoxStyle_Tab)
	);
}

void FEpithStyle::OnPatchComplete()
{
	// Live coding: re-run SetupStyles() on the existing style set so edits show up without restarting the editor.
	// Don't destroy the style set here: open widgets hold raw pointers to its brushes.
	if (StyleInstance.IsValid())
	{
		SetupStyles();
	}
}

#undef LOCTEXT_NAMESPACE
