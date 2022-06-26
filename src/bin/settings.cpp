#include "include/stunHandler.h"
#include "include/settings.h"
#include "ValhallaCombat.hpp"
using namespace inlineUtils;
void settings::ReadIntSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting) {
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		//logger::info("found {} with value {}", a_settingName, bFound);
		a_setting = static_cast<int>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}
void settings::ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		//logger::info("found {} with value {}", a_settingName, bFound);
		a_setting = static_cast<float>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}

void settings::ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound)
	{
		//logger::info("found {} with value {}", a_settingName, bFound);
		a_setting = a_ini.GetBoolValue(a_sectionName, a_settingName);
	}
}


void settings::init() {
	logger::info("Initilize settings...");
	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (dataHandler) {
#define LOOKGLOBAL dataHandler->LookupForm<RE::TESGlobal>
		glob_Nemesis_EldenCounter_Damage = LOOKGLOBAL(0x56A23, "ValhallaCombat.esp");
		glob_Nemesis_EldenCounter_NPC = LOOKGLOBAL(0x56A24, "ValhallaCombat.esp");
		glob_TrueHudAPI = LOOKGLOBAL(0x56A25, "ValhallaCombat.esp");
		glob_TrueHudAPI_SpecialMeter = LOOKGLOBAL(0x56A26, "ValhallaCombat.esp");
	}
	logger::info("...done");
}
void settings::updateGlobals() {
	logger::info("Update globals...");
	if (glob_TrueHudAPI) {
		glob_TrueHudAPI->value = ValhallaCombat::GetSingleton()->ersh != nullptr ? 1.f : 0.f;
	}
	if (glob_TrueHudAPI_SpecialMeter) {
		glob_TrueHudAPI_SpecialMeter->value = facts::TrueHudAPI_HasSpecialBarControl ? 1.f : 0.f;
	}
	auto pc = RE::PlayerCharacter::GetSingleton();
	if (pc) {
		if (glob_Nemesis_EldenCounter_Damage) {
			bool bDummy;
			glob_Nemesis_EldenCounter_Damage->value = pc->GetGraphVariableBool("is_Guard_Countering", bDummy);
		}
		if (glob_Nemesis_EldenCounter_NPC) {
			bool bDummy;
			glob_Nemesis_EldenCounter_NPC->value = pc->GetGraphVariableBool("IsValGC", bDummy);
		}
	}
	logger::info("...done");
}
/*read settings from ini, and update them into game settings.*/
void settings::readSettings() {
	logger::info("Read ini settings...");

#define SETTINGFILE_PATH "Data\\MCM\\Settings\\ValhallaCombat.ini"
	CSimpleIniA mcm;
	mcm.LoadFile(SETTINGFILE_PATH);

	std::list<CSimpleIniA::Entry> ls;
	/*Read stamina section*/
	ReadBoolSetting(mcm, "Stamina", "bUIalert", bUIAlert);
	ReadBoolSetting(mcm, "Stamina", "bNonCombatStaminaCost", bNonCombatStaminaCost);
	ReadFloatSetting(mcm, "Stamina", "fStaminaRegenMult", fStaminaRegenMult);
	ReadFloatSetting(mcm, "Stamina", "fStaminaRegenLimit", fStaminaRegenLimit);
	ReadFloatSetting(mcm, "Stamina", "fCombatStaminaRegenMult", fCombatStaminaRegenMult);
	ReadFloatSetting(mcm, "Stamina", "fStaminaRegenDelay", fStaminaRegenDelay);

	ReadBoolSetting(mcm, "Stamina", "bBlockStaminaToggle", bBlockStaminaToggle);
	ReadBoolSetting(mcm, "Stamina", "bGuardBreak", bGuardBreak);
	ReadFloatSetting(mcm, "Stamina", "fBckShdStaminaMult_NPC_Block_NPC", fBckShdStaminaMult_NPC_Block_NPC);
	ReadFloatSetting(mcm, "Stamina", "fBckShdStaminaMult_NPC_Block_PC", fBckShdStaminaMult_NPC_Block_PC);
	ReadFloatSetting(mcm, "Stamina", "fBckShdStaminaMult_PC_Block_NPC", fBckShdStaminaMult_PC_Block_NPC);
	ReadFloatSetting(mcm, "Stamina", "fBckWpnStaminaMult_NPC_Block_NPC", fBckWpnStaminaMult_NPC_Block_NPC);
	ReadFloatSetting(mcm, "Stamina", "fBckWpnStaminaMult_NPC_Block_PC", fBckWpnStaminaMult_NPC_Block_PC);
	ReadFloatSetting(mcm, "Stamina", "fBckWpnStaminaMult_PC_Block_NPC", fBckWpnStaminaMult_PC_Block_NPC);

	ReadBoolSetting(mcm, "Stamina", "bStaminaDebuffToggle", bStaminaDebuffToggle);
	ReadBoolSetting(mcm, "Stamina", "bAttackStaminaToggle", bAttackStaminaToggle);
	ReadBoolSetting(mcm, "Stamina", "bBlockedHitRegenStamina", bBlockedHitRegenStamina);
	ReadFloatSetting(mcm, "Stamina", "fMeleeCostLightMiss_Point", fMeleeCostLightMiss_Point); 
	ReadFloatSetting(mcm, "Stamina", "fMeleeRewardLightHit_Percent", fMeleeRewardLightHit_Percent);
	ReadFloatSetting(mcm, "Stamina", "fMeleeCostHeavyMiss_Percent", fMeleeCostHeavyMiss_Percent);
	ReadFloatSetting(mcm, "Stamina", "fMeleeCostHeavyHit_Percent", fMeleeCostHeavyHit_Percent);

	/*Read perfect blocking section*/
	ReadBoolSetting(mcm, "TimedBlocking", "bBlockProjectileToggle", bBlockProjectileToggle);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimedBlockToggle", bTimedBlockToggle);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimedBlockProjectileToggle", bTimedBlockProjectileToggle);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimedBlockScreenShake", bTimedBlockScreenShake);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimeBlockSFX", bTimeBlockSFX);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimedBlockVFX", bTimedBlockVFX);
	ReadBoolSetting(mcm, "TimedBlocking", "bTimedBlockSlowTime", bTimedBlockSlowTime);
	ReadFloatSetting(mcm, "TimedBlocking", "fTimedBlockWindow", fTimedBlockWindow);
	ReadFloatSetting(mcm, "TimedBlocking", "fTimedBlockCooldownTime", fTimedBlockCooldownTime);
	ReadFloatSetting(mcm, "TimedBlocking", "fTimedBlockStaminaCostMult", fTimedBlockStaminaCostMult);

	ReadIntSetting(mcm, "TimedBlocking", "uAltBlockKey", uAltBlockKey);
	/*Read stun section*/
	ReadBoolSetting(mcm, "Stun", "bStunToggle", bStunToggle);
	ReadBoolSetting(mcm, "Stun", "bStunMeterToggle", bStunMeterToggle);
	ReadBoolSetting(mcm, "Stun", "bExecutionLimit", bExecutionLimit);

	ReadFloatSetting(mcm, "Stun", "fStunTimedBlockMult", fStunTimedBlockMult);
	ReadFloatSetting(mcm, "Stun", "fStunBashMult", fStunBashMult);
	ReadFloatSetting(mcm, "Stun", "fStunPowerBashMult", fStunPowerBashMult);
	ReadFloatSetting(mcm, "Stun", "fStunNormalAttackMult", fStunNormalAttackMult);
	ReadFloatSetting(mcm, "Stun", "fStunPowerAttackMult", fStunPowerAttackMult);
	ReadFloatSetting(mcm, "Stun", "fStunParryMult", fStunParryMult);
	ReadFloatSetting(mcm, "Stun", "fStunUnarmedMult", fStunUnarmedMult);
	ReadFloatSetting(mcm, "Stun", "fStunDaggerMult", fStunDaggerMult);
	ReadFloatSetting(mcm, "Stun", "fStunSwordMult", fStunSwordMult);
	ReadFloatSetting(mcm, "Stun", "fStunWarAxeMult", fStunWarAxeMult);
	ReadFloatSetting(mcm, "Stun", "fStunMaceMult", fStunMaceMult);
	ReadFloatSetting(mcm, "Stun", "fStunGreatSwordMult", fStunGreatSwordMult);
	ReadFloatSetting(mcm, "Stun", "fStun2HBluntMult", fStun2HBluntMult);

	ReadBoolSetting(mcm, "Stun", "bAutoExecution", bAutoExecution);
	ReadIntSetting(mcm, "Stun", "uExecutionKey", uExecutionKey);
	ReadBoolSetting(mcm, "Compatibility", "bPoiseCompatibility", bPoiseCompatibility);

	/*Read Balance section*/
	ReadBoolSetting(mcm, "Balance", "bBalanceToggle", bBalanceToggle);
	logger::info("...done");


	logger::info("Apply game settings...");
	/*Set some game settings*/
	setGameSettingf("fDamagedStaminaRegenDelay", fStaminaRegenDelay);
	setGameSettingf("fCombatStaminaRegenRateMult", fCombatStaminaRegenMult);
	multStaminaRegen(fStaminaRegenMult, fStaminaRegenLimit);
	
	/*Release truehud meter if set so.*/
	if (bStunMeterToggle && bStunToggle){
		ValhallaCombat::GetSingleton()->requestTrueHudSpecialBarControl();
	}
	else {
		ValhallaCombat::GetSingleton()->releaseTrueHudSpecialBarControl();
	}
	logger::info("...done");
}

