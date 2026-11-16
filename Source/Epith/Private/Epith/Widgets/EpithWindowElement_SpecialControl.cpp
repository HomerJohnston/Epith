#include "Epith/Widgets/EpithWindowElement_SpecialControl.h"

#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_SpecialControl::Draw(const FEpithDrawContext& Context) const
{
	UObject* Target = Context.PropertyData.IsValid() ? Context.PropertyData->GetTarget() : nullptr;
	
	const FEpithSpecialControl* ControlPtr = Control.GetPtr<FEpithSpecialControl>();
	
	TSharedPtr<SWidget> ControlWidget = ControlPtr ? ControlPtr->Draw(FEpithControlContext(Target, Context.IsDesigning())) : nullptr;
	
	if (!Context.IsDesigning())
	{
		return ControlWidget;
	}
	
	// Designer: a control picker until a control is chosen (right-click > Unset Control to change it), then a preview of it
	if (!ControlPtr)
	{
		FEpithWindowElement_SpecialControl* MutableThis = const_cast<FEpithWindowElement_SpecialControl*>(this);
		
		return Context.Designer->MakeControlPicker([MutableThis](const UScriptStruct* Type)
		{
			MutableThis->Control.InitializeAsScriptStruct(Type);
		});
	}
	
	if (!ControlWidget.IsValid())
	{
		ControlWidget = SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("SpecialControl_NoWidget", "{0} (draws nothing)"), Control.GetScriptStruct()->GetDisplayNameText()));
	}
	
	return SNew(SBox)
		.Visibility(EVisibility::HitTestInvisible)
		[
			ControlWidget.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE
