#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"

#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Styling/SlateIconFinder.h"

void SEpithViewportObjectPaneHeader::Construct(const FArguments& InArgs)
{
	const FSlateBrush* ActorIcon = FSlateIconFinder::FindIconBrushForClass(InArgs._Target->GetClass());
	
	// The master pane's header already shows the actor's label, so sections default to what the object *is*:
	// the class name for the actor itself and other objects, the component's name for components.
	FText Title = InArgs._TitleOverride;
	
	if (Title.IsEmpty())
	{
		if (UActorComponent* TargetActorComponent = Cast<UActorComponent>(InArgs._Target))
		{
			Title = FText::FromString(TargetActorComponent->GetName());
		}
		else
		{
			Title = InArgs._Target->GetClass()->GetDisplayNameText();
		}
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