#pragma once

#include "UObject/Object.h"

#include "EpithTestObject.generated.h"

/** Used by Epith's automation tests only. */
USTRUCT()
struct FEpithTestStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Test")
	float Value = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Test")
	int32 Count = 0;
};

/**
 * Used by Epith's automation tests only: one property of each kind the panel has to handle.
 * Transient and hidden from class pickers, so it never shows up for users.
 */
UCLASS(Transient, HideDropdown, NotBlueprintable, NotBlueprintType)
class UEpithTestObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Test")
	float PlainFloat = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Test")
	bool bCondition = false;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (EditCondition = "bCondition"))
	float GreyedWhenFalse = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (EditCondition = "bCondition", EditConditionHides))
	float HiddenWhenFalse = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (InlineEditConditionToggle))
	bool bInlineToggle = false;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (EditCondition = "bInlineToggle"))
	float InlineGated = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Test")
	float DefaultsOnly = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Test")
	FEpithTestStruct Struct;

	UPROPERTY(EditDefaultsOnly, Category = "Test")
	FEpithTestStruct DefaultsOnlyStruct;

	UPROPERTY(EditAnywhere, Category = "Test")
	TArray<int32> Numbers;

	UPROPERTY(EditAnywhere, Category = "Test")
	FLinearColor Color = FLinearColor::White;
};
