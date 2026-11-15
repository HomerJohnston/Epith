#include "Epith/Widgets/EpithWindowElement.h"

void FEpithWindowElement_BoxDisplayParams::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && bBorder_DEPRECATED)
	{
		Border = 1;
		bBorder_DEPRECATED = false;
	}
}
