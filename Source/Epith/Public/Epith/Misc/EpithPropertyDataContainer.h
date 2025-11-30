#pragma once

#include "IDetailTreeNode.h"
#include "PropertyHandle.h"

struct FEpithPropertyCategoryContainer
{
	TMap<FName, TSharedPtr<IDetailTreeNode>> TreeNodes;
	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;
};

struct FEpithPropertyDataContainer
{
	FEpithPropertyDataContainer(AActor* Target);

	FName DefaultCategory;

	TSharedPtr<class IPropertyRowGenerator> PropertyRowGenerator;
	
	TMap<FName, TSharedPtr<FEpithPropertyCategoryContainer>> Categories;

	void Build(AActor* Actor);
};