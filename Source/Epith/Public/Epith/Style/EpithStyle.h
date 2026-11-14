// Copyright Ghost Pepper Games, Inc. All Rights Reserved.
// This work is MIT-licensed. Feel free to use it however you wish, within the confines of the MIT license.

#pragma once

#include "Styling/SlateStyle.h"
#include "Delegates/IDelegateInstance.h"
#include "Textures/SlateIcon.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "EpithEditor"

// ==============================================

struct FEpithBrushes
{
	FName Border_2px_Deburred;
	FName Border_TabShape;
	FName Background_ViewportMasterPanelHeader;
	
	FName Background_WhiteSquare;
};

struct FEpithStyles
{
	FName CheckBoxStyle_Tab;
	FName SegmentedControlStyle_Tab;
};

struct FEpithFonts
{
	FSlateFontInfo Font_ViewportPopup_Title;
};

extern FEpithBrushes EpithBrushes;
extern FEpithStyles EpithStyles;
extern FEpithFonts EpithFonts;

class FEpithStyle
{
public:
    
public:
    FEpithStyle();
	
    virtual ~FEpithStyle();
	
public:
    static ISlateStyle& Get();

    static const FSlateBrush* GetImageBrush(FName BrushName);

    static FName GetStyleSetName();

    /** Creates and registers the style set. Called once from FEpithModule::StartupModule. Safe to call more than once. */
    static void Initialize();

    /** Unregisters and destroys the style set. Called from FEpithModule::ShutdownModule. */
    static void Shutdown();
	
protected:
    static TSharedRef< class FSlateStyleSet > Create();
	
    static void SetupStyles();
	
    static void OnPatchComplete();

    static const ISlateStyle* GetParentStyle();
	
    static TArray<TStrongObjectPtr<UTexture2D>> Textures;

    static FDelegateHandle OnPatchCompleteHandle;

    static TSharedPtr<FSlateStyleSet> StyleInstance;

};

#undef LOCTEXT_NAMESPACE