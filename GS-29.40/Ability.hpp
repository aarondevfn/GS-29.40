#pragma once



FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* component, FGameplayAbilitySpecHandle Handle)
{
	auto ActivatableAbilities = component->ActivatableAbilities.Items;


	for (int i = 0; i < ActivatableAbilities.Num(); i++) {
		auto CurrentSpec = ActivatableAbilities[i];
		auto CurrentHandle = CurrentSpec.Handle;
		

		if (CurrentHandle.Handle == Handle.Handle) {
			return &CurrentSpec;
		}
	}
	return nullptr;
}


void MarkAbilitySpecDirty(UAbilitySystemComponent* component, FGameplayAbilitySpec* Spec)
{
	static auto ActivatableAbilities = component->ActivatableAbilities;
	ActivatableAbilities.MarkItemDirty(Spec);
}



void InternalServerTryActiveAbility(UAbilitySystemComponent* component, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey* PredictionKey, FGameplayEventData* TriggerEventData) {
	//__int64* Spec = this->FindAbilitySpecFromHandle(Handle);
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(component, Handle);

	if (!Spec)
	{
		std::cout << "InternalServerTryActiveAbility. Rejecting ClientActivation of ability with invalid SpecHandle!" << std::endl;;
		component->ClientActivateAbilityFailed(Handle, PredictionKey->Current);
		return;
	}
	UGameplayAbility* InstancedAbility = nullptr;

	Spec->InputPressed = InputPressed;

	if (!InternalTryActivateAbility(component, Handle, *PredictionKey, &InstancedAbility, nullptr, TriggerEventData)) {
		//std::cout << "InternalServerTryActiveAbility. Rejecting ClientActivation of ability with invalid SpecHandle!" << std::endl;
		component->ClientActivateAbilityFailed(Handle, PredictionKey->Current);
		Spec->InputPressed = false;
	}
	MarkAbilitySpecDirty(component, Spec);

	// std::cout << "InternalServerTryActiveAbility called!" << std::endl;;
}





FGameplayAbilitySpecHandle GiveAbilityEasy(UAbilitySystemComponent* Component, UClass* AbilityClass, UObject* SourceObject, bool bDoNotRegive)
{


	auto DefaultAbility = (UGameplayAbility*)AbilityClass->GetDefaultObj();;
	if (!DefaultAbility)
		return FGameplayAbilitySpecHandle();


	auto GenerateNewSpec = [&]() -> FGameplayAbilitySpec
		{
			FGameplayAbilitySpecHandle Handle{ rand() };

			FGameplayAbilitySpec Spec{ -1, -1, -1, Handle,DefaultAbility, 1, -1, 0, 0, false, false, false };


			return Spec;
		};

	auto NewSpec = GenerateNewSpec();



	FGameplayAbilitySpecHandle Handle;

	GiveAbility0(Component, &Handle, &NewSpec);

	return Handle;
}


void ApplyGrantedGameplayAffectsToAbilitySystem(UFortAbilitySet* AbilitySet, UAbilitySystemComponent* AbilitySystemComponent) 
{

	auto GrantedGameplayEffects = AbilitySet->GrantedGameplayEffects;

	for (int i = 0; i < GrantedGameplayEffects.Num(); ++i)
	{
		auto& EffectToGrant = GrantedGameplayEffects[i];

		if (!EffectToGrant.GameplayEffect)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext{}; // AbilitySystemComponent->MakeEffectContext()
		AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(EffectToGrant.GameplayEffect, EffectToGrant.Level, EffectContext);
	}
}

void GiveToAbilitySystem(UFortAbilitySet* AbilitySet, UAbilitySystemComponent* AbilitySystemComponent, UObject* SourceObject = nullptr)
{

	auto GameplayAbilities = AbilitySet->GameplayAbilities;

	for (int i = 0; i < GameplayAbilities.Num(); ++i)
	{
		UClass* AbilityClass = GameplayAbilities[i];

		if (!AbilityClass)
			continue;

		
		GiveAbilityEasy(AbilitySystemComponent,AbilityClass, SourceObject,true);

	}

	ApplyGrantedGameplayAffectsToAbilitySystem(AbilitySet, AbilitySystemComponent);
}