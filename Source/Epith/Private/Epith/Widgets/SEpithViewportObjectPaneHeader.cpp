#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"

#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Styling/SlateIconFinder.h"

void SEpithViewportObjectPaneHeader::Construct(const FArguments& InArgs)
{
	const FSlateBrush* ActorIcon = FSlateIconFinder::FindIconBrushForClass(InArgs._Target->GetClass());
	
	FText Title = INVTEXT("Properties");
	
	if (AActor* TargetActor = Cast<AActor>(InArgs._Target))
	{
		Title = FText::FromString(TargetActor->GetActorLabel() + " Properties");
	}
	else
	{
		Title = FText::FromString(InArgs._Target->GetClass()->GetName());
	}
	
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SImage)
			.Image(ActorIcon)
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(EpithFonts.Font_ViewportPopup_Title)
		]
	];
}