#pragma once

#include "CoreMinimal.h"
#include "CaveboundEnemyBase.h"
#include "CaveboundEnemy.generated.h"

/**
 * Basic pawn type enemy
 */
UCLASS()
class CAVEBOUND_API ACaveboundEnemy : public ACaveboundBaseEnemy
{
	GENERATED_BODY()

public:
	ACaveboundEnemy();
};
