#pragma once

enum class EEpithAttribute : uint8
{
	TabGroup,
	BoxGroup,
	FoldoutGroup,
};


struct EPITH_API Epith
{
public:
	static void Set(EEpithAttribute Attribute, FString AttributeMetaData, FName Property);
	
	struct EPITH_API ScopeContext
	{
		ScopeContext(const UClass* InClass);

		~ScopeContext();
	};
	
private:
	static const TArray<FName> EpithAttributeNames;
	
	static const UClass* __OperatingClass;
	
	static void SetOperatingClass(const UClass* Class);
	
	static FName GetAttributeFName(EEpithAttribute Attribute)
	{
		return EpithAttributeNames[(uint8)Attribute];
	}
};

using enum EEpithAttribute;