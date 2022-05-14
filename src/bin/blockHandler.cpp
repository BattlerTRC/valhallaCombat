#include "include/blockHandler.h"
#include "include/data.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "include/stunHandler.h"
#include "include/balanceHandler.h"
#include "include/reactionHandler.h"
#include "include/AI.h"
#include "include/lib/BlockSpark.h"
#include "include/offsets.h"
#include "ValhallaCombat.hpp"
using HITFLAG = RE::HitData::Flag;
/*Called every frame.
Decrement the timer for actors either perfect blocking or cooling down.*/
void blockHandler::update() {
	if (!isPcTimedBlocking && !isPcBlockingCoolDown) {
		ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::blockHandler);
	}

	auto pc = RE::PlayerCharacter::GetSingleton();
	bool isPCBlocking = false;
	if (pc) {
		isPCBlocking = pc->IsBlocking();
	}
	auto deltaTime = *RE::Offset::g_deltaTime;
	if (isPcTimedBlocking) {
		pcBlockTimer -= deltaTime;
		if (pcBlockTimer <= 0) {
			isPcTimedBlocking = false;
			pcBlockTimer = 0;
		}
	}
	if (isPcBlockingCoolDown && !isPCBlocking) {
		pcCoolDownTimer -= deltaTime;
		if (pcCoolDownTimer <= 0) {
			switch (pcBlockWindowPenalty) {
			case blockWindowPenaltyLevel::light: pcBlockWindowPenalty = none; isPcBlockingCoolDown = false; break;
			case blockWindowPenaltyLevel::medium: pcBlockWindowPenalty = light; break;
			case blockWindowPenaltyLevel::heavy: pcBlockWindowPenalty = medium; break;
			}
			pcCoolDownTimer = settings::fTimedBlockCooldownTime;
		}
	}
	
}
void blockHandler::onPcTimedBlockEnd() {
	if (!isPcTimedBlockSuccess) {//apply penalty only if the current block is not a successful timed block.
		DEBUG("prev timed block not successful; applying penalty");
		isPcBlockingCoolDown = true; //not a successful timed block, start penalty cool down.
		pcCoolDownTimer = settings::fTimedBlockCooldownTime;
		//increment penalty.
		switch (pcBlockWindowPenalty) {
		case blockWindowPenaltyLevel::none: pcBlockWindowPenalty = light; DEBUG("light"); break;
		case blockWindowPenaltyLevel::light: pcBlockWindowPenalty = medium; DEBUG("medium"); break;
		case blockWindowPenaltyLevel::medium: pcBlockWindowPenalty = heavy; DEBUG("heavy"); break;
		}
		ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockHandler);

	}
	else {//previous timed block is successful
		isPcTimedBlockSuccess = false; //reset success state.
	}
	isPcTimedBlocking = false;
	pcBlockTimer = 0;
}

void blockHandler::onBlockKeyDown() {
	if (isBlockKeyUp_and_still_blocking) {
		onPcTimedBlockEnd();
		isBlockKeyUp_and_still_blocking = false;
	}

	isPcTimedBlocking = true;
	isBlockButtonPressed = true;
	isPcTimedBlockSuccess = false; //reset success state
	float blockWindow = settings::fTimedBlockWindow;
	
	switch (pcBlockWindowPenalty) {
	case blockWindowPenaltyLevel::none: break;
	case blockWindowPenaltyLevel::light: blockWindow *= 0.6; break;
	case blockWindowPenaltyLevel::medium: blockWindow *= 0.3; break;
	case blockWindowPenaltyLevel::heavy: blockWindow *= 0.15; break;
	}
	pcBlockTimer = blockWindow;
	DEBUG("block start with window {}", blockWindow);
	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockHandler);
}

void blockHandler::onBlockKeyUp() {
	isBlockButtonPressed = false;
	auto pc = RE::PlayerCharacter::GetSingleton();
	if (pc && pc->IsBlocking()) {
		isBlockKeyUp_and_still_blocking = true;
	}
}

void blockHandler::onBlockStop() {
	isBlockKeyUp_and_still_blocking = false;
	onPcTimedBlockEnd();
}

void blockHandler::onSuccessfulTimedBlock() {
	isPcTimedBlockSuccess = true;
	pcBlockWindowPenalty = blockWindowPenaltyLevel::none;
}

bool blockHandler::isInBlockAngle(RE::Actor* blocker, RE::TESObjectREFR* a_obj) {
	auto angle = blocker->GetHeadingAngle(a_obj->GetPosition(), false);
	auto desiredAngle = data::fCombatHitConeAngle / 2;
	return (angle <= desiredAngle && angle >= -desiredAngle);
}

void blockHandler::processProjectileParry(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	RE::TESObjectREFR* a_target = nullptr;
	if (a_projectile->shooter && a_projectile->shooter.get()) {
		a_target = a_projectile->shooter.get().get();
		if (a_target->GetFormType() != RE::FormType::ActorCharacter) {
			a_target = nullptr;
		}
	}
	ValhallaUtils::resetProjectileOwner(a_projectile, a_blocker, a_projectile_collidable);
	if (getIsPcPerfectBlocking()) {//only deflect projectile on perfect block/parry
		deflectProjectile(a_blocker, a_projectile, a_projectile_collidable, a_target->As<RE::Actor>());
	}
	else {
		parryProjectile(a_blocker, a_projectile);
	}
}

bool blockHandler::processProjectileBlock_Spell(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::MagicItem* a_spell) {
	auto cost = a_spell->CalculateMagickaCost(a_blocker);
	//Utils::offsetRealDamageForPc(cost);
	if (Utils::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
		parryProjectile(a_blocker, a_projectile);
		return true;
	}
}

bool blockHandler::processProjectileBlock_Arrow(RE::Actor* a_blocker, RE::Projectile* a_projectile) {
	auto launcher = a_projectile->weaponSource;
	auto ammo = a_projectile->ammoSource;
	if (launcher && ammo) {
		auto cost = launcher->GetAttackDamage() + ammo->data.damage;
		Utils::offsetRealDamageForPc(cost);
		if (Utils::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
			parryProjectile(a_blocker, a_projectile);
			return true;
		}
	}
}

bool blockHandler::processRegularSpellBlock(RE::Actor* a_blocker, RE::MagicItem* a_spell, RE::Projectile* a_projectile) {
	if (!a_spell) {
		return false;
	}
	if (!a_blocker->IsBlocking() || !isInBlockAngle(a_blocker, a_projectile)) {
		return false;
	}
	if (a_blocker->IsPlayerRef()) {
		auto cost = a_spell->CalculateMagickaCost(a_blocker);
		if (Utils::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
			playBlockEffects(a_blocker, nullptr, blockType::perfect);
			return true;
		}
	}
}

bool blockHandler::preProcessProjectileBlock(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	if (a_blocker->IsPlayerRef()) {
		if (isInBlockAngle(a_blocker, a_projectile) && a_blocker->IsBlocking()) {
			//bool isPcParrying = getIsPcParrying();
			//Perfect block
			if (settings::bTimedBlockProjectileToggle && getIsPcTimedBlocking()) {
				onSuccessfulTimedBlock();
				processProjectileParry(a_blocker, a_projectile, a_projectile_collidable);
				return true;
			}
			else if (settings::bBlockProjectileToggle) {//none-perfect block
				auto spell = a_projectile->spell;
				if (spell) {
					return processProjectileBlock_Spell(a_blocker, a_projectile, spell);
				} else if (!a_blocker->isHoldingShield()) { //physical projectile blocking only applies to none-shield.
					return processProjectileBlock_Arrow(a_blocker, a_projectile);
				}
			}
		}
		
	}
	return false;
}

void blockHandler::deflectProjectile(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable, RE::Actor* a_target) {
	if (a_blocker->IsBlocking()) {
		a_blocker->NotifyAnimationGraph("BlockHitStart");
	}
	playBlockEffects(a_blocker, nullptr, blockHandler::blockType::perfect);
	if (a_target && a_target->Is3DLoaded()) {
		ValhallaUtils::DeflectProjectile(a_blocker, a_projectile, a_target);
	}
	else {
		ValhallaUtils::ReflectProjectile(a_projectile);
	}
}

void blockHandler::parryProjectile(RE::Actor* a_blocker, RE::Projectile* a_projectile) {
	if (a_blocker->IsBlocking()) {
		a_blocker->NotifyAnimationGraph("BlockHitStart");
	}
	playBlockEffects(a_blocker, nullptr, blockHandler::blockType::timed);
	RE::Offset::destroyProjectile(a_projectile);
}

#pragma region Process Block
bool blockHandler::processPhysicalBlock(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag, RE::HitData& hitData, float realDamage) {
	//DEBUG("Process blocking. Blocker: {} Aggressor: {}", blocker->GetName(), aggressor->GetName());
	DEBUG("processing block. Real damage: {}", realDamage);
	if (settings::bTimedBlockToggle) {
		if (blocker->IsPlayerRef()) {
			if (blockHandler::getIsPcTimedBlocking()) {
				onSuccessfulTimedBlock();
				processPhysicalTimedBlock(blocker, aggressor, iHitflag, hitData, realDamage, pcBlockTimer);
				return true;
			}
		}
		//}
		//else {
			//mtx_actors_PerfectBlocking.unlock();
		//}
	}
	if (settings::bBlockStaminaToggle) {
		processStaminaBlock(blocker, aggressor, iHitflag, hitData, realDamage);
	}
	return false;
}

/*Return the final stamina cost multiplier based on the blocker and aggressor.*/
inline float getBlockStaminaCostMult(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag) {
	if (iHitflag & (int)HITFLAG::kBlockWithWeapon) {
		//DEBUG("hit blocked with weapon");
		if (blocker->IsPlayerRef()) {
			return settings::fBckWpnStaminaMult_PC_Block_NPC;
		}
		else {
			if (aggressor->IsPlayerRef()) {
				return settings::fBckWpnStaminaMult_NPC_Block_PC;
			}
			else {
				return settings::fBckWpnStaminaMult_NPC_Block_NPC;
			}
		}
	}
	else {
		//DEBUG("hit blocked with shield");
		if (blocker->IsPlayerRef()) {
			return settings::fBckShdStaminaMult_PC_Block_NPC;
		}
		else {
			if (aggressor->IsPlayerRef()) {
				return settings::fBckShdStaminaMult_NPC_Block_PC;
			}
			else {
				return settings::fBckShdStaminaMult_NPC_Block_NPC;
			}
		}
	}
}

void blockHandler::processStaminaBlock(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag, RE::HitData& hitData, float realDamage) {
	using HITFLAG = RE::HitData::Flag;
	DEBUG("real damage: {}", realDamage);
	float staminaDamageMult = getBlockStaminaCostMult(blocker, aggressor, iHitflag);
	DEBUG("stamina damagem mult: {}", staminaDamageMult);
	float staminaDamage = staminaDamageMult * realDamage;
	DEBUG("stamina damage: {}", staminaDamage);

	float targetStamina = blocker->GetActorValue(RE::ActorValue::kStamina);

	//check whether there's enough stamina to block incoming attack
	if (targetStamina < staminaDamage) {
		if (settings::bGuardBreak) {
			if (iHitflag & (int)HITFLAG::kPowerAttack) {
				reactionHandler::triggerStagger(aggressor, blocker, reactionHandler::kLarge);
			}
			else {
				reactionHandler::triggerStagger(aggressor, blocker, reactionHandler::kMedium);
			}
		}
		hitData.totalDamage =
			(realDamage - (targetStamina / staminaDamageMult)) //real damage actor will be receiving.
			* (hitData.totalDamage) / realDamage; //offset real damage back into raw damage to be converted into real damage again later.
		Utils::damageav(blocker, RE::ActorValue::kStamina,
			targetStamina);
		debuffHandler::GetSingleton()->initStaminaDebuff(blocker); //initialize debuff for the failed blocking attempt
	}
	else {
		hitData.totalDamage = 0;
		Utils::damageav(blocker, RE::ActorValue::kStamina,
			staminaDamage);

	}
}
bool blockHandler::getIsPcTimedBlocking() {
	return (isPcTimedBlocking && RE::PlayerCharacter::GetSingleton()->IsBlocking());
}

bool blockHandler::getIsPcPerfectBlocking() {
	return pcBlockTimer >= 0.35 || isBlockKeyUp_and_still_blocking;
}

bool blockHandler::getIsPcParrying() {
	return RE::PlayerCharacter::GetSingleton()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash;
}

void blockHandler::processPhysicalTimedBlock(RE::Actor* blocker, RE::Actor* attacker, int iHitflag, RE::HitData& hitData, float realDamage, float timeLeft) {
	float reflectedDamage = 0;
	auto a_weapon = blocker->getWieldingWeapon();
	bool isPerfectblock = blocker->IsPlayerRef() && getIsPcPerfectBlocking();
	if (a_weapon) {
		reflectedDamage = a_weapon->GetAttackDamage();//get attack damage of blocker's weapon
	}
	Utils::offsetRealDamage(reflectedDamage, blocker, attacker);
	float stunDmg = reflectedDamage;
	float balanceDmg = reflectedDamage;
	if (isPerfectblock) {
		balanceDmg += realDamage; //reflect attacker's damage back as balance dmg
	}
	stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::parry, nullptr, blocker, attacker, stunDmg);
	balanceHandler::GetSingleton()->processBalanceDamage(balanceHandler::DMGSOURCE::parry, nullptr, blocker, attacker, balanceDmg);
	hitData.totalDamage = 0;
	bool isAttackerGuardBroken = balanceHandler::GetSingleton()->isBalanceBroken(attacker)
		|| stunHandler::GetSingleton()->isActorStunned(attacker);

	if (isAttackerGuardBroken) {
		playBlockEffects(blocker, attacker, blockType::guardBreaking);
	}
	else {
		if (isPerfectblock) {
			playBlockEffects(blocker, attacker, blockType::perfect);
		}
		else {
			playBlockEffects(blocker, attacker, blockType::timed);
		}
	}

	if (isPerfectblock || isAttackerGuardBroken) {//stagger opponent immediately on perfect block.
		reactionHandler::triggerStagger(blocker, attacker, reactionHandler::reactionType::kLarge);
		debuffHandler::GetSingleton()->quickStopStaminaDebuff(blocker);
		Utils::refillActorValue(blocker, RE::ActorValue::kStamina); //perfect blocking completely restores actor value.
	}
	else {
		Utils::damageav(blocker, RE::ActorValue::kStamina,
			realDamage * getBlockStaminaCostMult(blocker, attacker, iHitflag) * settings::fTimedBlockStaminaCostMult);
	}
	//damage blocker's stamina
}
#pragma endregion
/*Unused for now. Reserved for souls like parry.*/
void blockHandler::processMeleeParry(RE::Actor* a_blocker, RE::Actor* a_attacker) {
	float reflectedDamage = 0;
	auto a_weapon = a_blocker->getWieldingWeapon();
	if (a_weapon) {
		reflectedDamage = a_weapon->GetAttackDamage();//get attack damage of blocker's weapon
	}
	Utils::offsetRealDamage(reflectedDamage, a_blocker, a_attacker);
	stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::parry, nullptr, a_blocker, a_attacker, reflectedDamage);
	balanceHandler::GetSingleton()->processBalanceDamage(balanceHandler::DMGSOURCE::parry, nullptr, a_blocker, a_attacker, reflectedDamage);
	bool isAttackerGuardBroken = balanceHandler::GetSingleton()->isBalanceBroken(a_attacker)
		|| stunHandler::GetSingleton()->isActorStunned(a_attacker);

	if (isAttackerGuardBroken) {
		playBlockEffects(a_blocker, a_attacker, blockType::guardBreaking);
	}
	else {
		playBlockEffects(a_blocker, a_attacker, blockType::perfect);

	}
	debuffHandler::GetSingleton()->quickStopStaminaDebuff(a_blocker);
	reactionHandler::triggerStagger(a_blocker, a_attacker, reactionHandler::reactionType::kLarge);
}
void blockHandler::playBlockSFX(RE::Actor* blocker, blockType blockType, bool blockedWithWeapon) {
	if (blockedWithWeapon) {
		switch (blockType) {
		case blockType::guardBreaking: ValhallaUtils::playSound(blocker, data::soundParryWeapon_gb); break;
		case blockType::perfect: ValhallaUtils::playSound(blocker, data::soundParryWeapon_perfect); break;
		case blockType::timed: ValhallaUtils::playSound(blocker, data::soundParryWeaponV); break;
		}
	}
	else {
		switch (blockType) {
		case blockType::guardBreaking: ValhallaUtils::playSound(blocker, data::soundParryShield_gb); break;
		case blockType::perfect: ValhallaUtils::playSound(blocker, data::soundParryShield_perfect); break;
		case blockType::timed: ValhallaUtils::playSound(blocker, data::soundParryShieldV); break;
		}
	}
}
void blockHandler::playBlockVFX(RE::Actor* blocker, blockType blockType) {
	MaxsuBlockSpark::blockSpark::playPerfectBlockSpark(blocker);
}
void blockHandler::playBlockScreenShake(RE::Actor* blocker, blockType blockType) {
	switch (blockType) {
	case blockType::guardBreaking:RE::Offset::shakeCamera(1.7, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.8f); break;
	case blockType::timed: RE::Offset::shakeCamera(1.5, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.3f); break;
	case blockType::perfect: RE::Offset::shakeCamera(1.5, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.5f); break;
	}
}

void blockHandler::playBlockSlowTime(blockType blockType) {
	if (blockType == blockType::timed) {
		return;
	}
	//DEBUG("start slow!");
	Utils::SGTM(0.1); 
	auto resetSlowTime = [](int stopTime_MS) {
		std::this_thread::sleep_for(std::chrono::milliseconds(stopTime_MS));
		Utils::SGTM(1);
	};
	float a_time;
	switch (blockType) {
	case blockType::guardBreaking: a_time = 500; break;
	case blockType::perfect: a_time = 300; break;
	}
	std::jthread t(resetSlowTime, a_time);
	t.detach();
}

void blockHandler::playBlockEffects(RE::Actor* blocker, RE::Actor* attacker, blockType blockType) {
	DEBUG("playing effects");
	if (settings::bTimedBlockVFX) {
		playBlockVFX(blocker, blockType);
	}
	if (settings::bTimedBlockScreenShake
		&&(blocker->IsPlayerRef() || (attacker && attacker->IsPlayerRef()))) {
		playBlockScreenShake(blocker, blockType);
	}
	if (settings::bTimeBlockSFX) {
		playBlockSFX(blocker, blockType, !blocker->isHoldingShield());
	}
	if (settings::bTimedBlockSlowTime
		&&((attacker && attacker->IsPlayerRef()) || blocker->IsPlayerRef())) {
		playBlockSlowTime(blockType);
	}
}