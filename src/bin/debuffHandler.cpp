#include "include/debuffHandler.h"
#include "include/data.h"
#include "include/settings.h"
#include "include/offsets.h"
#include "valhallaCombat.hpp"
//TODO:make sure to resume the debuff state on reloading the actor i.e. the actor has debuff perk, but it's no longer in the debuff map, so they have to be put back onto the map.
/*Called every frame.
Iterate through the set of actors debuffing.
Check the actors' stamina. If the actor's stamina has fully recovered, remove the actor from the set.
Check the actor's UI counter, if the counter is less than 0, flash the actor's UI.*/
void debuffHandler::update() {
	mtx.lock();
	auto it = actorDebuffMap.begin();
	while (it != actorDebuffMap.end()) {
		auto actor = it->first;
		if (!actor || !actor->currentProcess || !actor->currentProcess->InHighProcess()) {//actor no longer loaded
			//DEBUG("Actor no longer loaded");
			it = actorDebuffMap.erase(it);//erase actor from debuff set.
			if (actorDebuffMap.size() == 0) {
				ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::debuffHandler);
			}
			continue;
		}
		if (actor->GetActorValue(RE::ActorValue::kStamina) >= 
			actor->GetPermanentActorValue(RE::ActorValue::kStamina) 
			+ actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina)) { //offset max stamina based on modifier
			//DEBUG("{}'s stamina has fully recovered.", actor->GetName());
			stopStaminaDebuff(actor);
			//DEBUG("erasing actor");
			it = actorDebuffMap.erase(it);
			if (actorDebuffMap.size() == 0) {
				ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::debuffHandler);
			}
			continue;
		}
		if (settings::bUIAlert
			&& settings::TrueHudAPI){ //flash the actor's meter
			if (it->second <= 0) {
				if (settings::TrueHudAPI) {
					ValhallaCombat::GetSingleton()->g_trueHUD->FlashActorValue(actor->GetHandle(), RE::ActorValue::kStamina, true);
				}
				it->second = 0.5;
			}
			else {
				it->second -= *RE::Offset::g_deltaTime;
			}
		}
		++it;
	}

	mtx.unlock();
	
}

/*Initialize a stmaina debuff for actor, giving them exhaustion perk, and putting them into the debuff map.
If the actor is already in the debuff map(i.e. they are already experiencing debuff), do nothing.
@param actor actor who will receive debuff.*/
void debuffHandler::initStaminaDebuff(RE::Actor* actor) {
	mtx.lock();
	if (actorDebuffMap.find(actor) != actorDebuffMap.end()) {
		//DEBUG("{} is already in debuff", actor->GetName());
		mtx.unlock();
		return;
	}
	actorDebuffMap.emplace(actor, 0);
	mtx.unlock();
	addDebuffPerk(actor);
	if (settings::bUIAlert && settings::TrueHudAPI) {
		greyOutStaminaMeter(actor);
	}
	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::debuffHandler);
}

/*Stamina the actor's stamina debuff, remove their debuff perk, and revert their UI meter.
@param actor actor whose stamina debuff will stop.*/
void debuffHandler::stopStaminaDebuff(RE::Actor* actor) {
	//DEBUG("Stopping stamina debuff for {}", actor->GetName());
	removeDebuffPerk(actor);
	if (settings::bUIAlert && settings::TrueHudAPI) {
		revertStaminaMeter(actor);
	}
}

void debuffHandler::quickStopStaminaDebuff(RE::Actor* actor) {
	mtx.lock();
	actorDebuffMap.erase(actor);
	if (actorDebuffMap.size() == 0) {
		ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::debuffHandler);
	}
	mtx.unlock();
	stopStaminaDebuff(actor);
}

/*Attach stamina debuff perk to actor.
@param a_actor actor who will receive the debuff perk.*/
void debuffHandler::addDebuffPerk(RE::Actor* a_actor) {
	Utils::safeApplyPerk(data::GetSingleton()->debuffPerk, a_actor);
}

/*Remove stamina debuff perk from actor.
@param a_actor actor who will gets the perk removed.*/
void debuffHandler::removeDebuffPerk(RE::Actor* a_actor) {
	Utils::safeRemovePerk(data::GetSingleton()->debuffPerk, a_actor);
}
	

bool debuffHandler::isInDebuff(RE::Actor* a_actor) {
	mtx.lock();
	if (actorDebuffMap.find(a_actor) != actorDebuffMap.end()) {
		mtx.unlock();
		return true;
	}
	mtx.unlock();
	return false;
} 

#pragma region staminaBarTweak

/*Turn flashColor red, turn barcolor and phantom color grey.*/
void debuffHandler::greyOutStaminaMeter(RE::Actor* actor) {
	ValhallaCombat::GetSingleton()->g_trueHUD->OverrideBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::FlashColor, 0xd72a2a);
	ValhallaCombat::GetSingleton()->g_trueHUD->OverrideBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::BarColor, 0x7d7e7d);
	ValhallaCombat::GetSingleton()->g_trueHUD->OverrideBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::PhantomColor, 0xb30d10);
}

void debuffHandler::revertStaminaMeter(RE::Actor* actor) {
	ValhallaCombat::GetSingleton()->g_trueHUD->RevertBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::FlashColor);
	ValhallaCombat::GetSingleton()->g_trueHUD->RevertBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::BarColor);
	ValhallaCombat::GetSingleton()->g_trueHUD->RevertBarColor(actor->GetHandle(), RE::ActorValue::kStamina, TRUEHUD_API::BarColorType::PhantomColor);
	//DEBUG("{}s stamia meter reverted", actor->GetName());
}

#pragma endregion