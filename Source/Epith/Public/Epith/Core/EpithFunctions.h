#pragma once

enum class EEpithWindowElementType : uint8
{
	Parent,
	VerticalGroup,
	HorizontalGroup,
	TabGroup,
	FoldoutGroup,
	Property,
};


struct EPITH_API Epith
{
public:
	//static TSharedPtr<FEpithWindowElement> BeginLayout(UClass* Class);
	
	//static void SetGroup(FName Property, FName GroupName);
		
private:
	//static const TArray<FName> EpithAttributeNames;
	
	//static void SetOperatingClass(const UClass* Class);
};

using enum EEpithWindowElementType;