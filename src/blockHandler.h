#pragma once
#include "include/robin_hood.h"
/*Handling block stamina damage and perfect blocking.*/
class blockHandler {
public:

	/*Mapping of all actors in perfect blocking state =>> effective time of their perfect blocks.*/
	robin_hood::unordered_map <RE::Actor*, float> actorsPerfectBlocking;

	/*Mapping of all actors in perfect blocking cool down =>> remaining time of the cool down.*/
	robin_hood::unordered_map <RE::Actor*, float> actorsInBlockingCoolDown;

	/*Set of all actors who have successfully perfect blocked an attack.*/
	robin_hood::unordered_set <RE::Actor*> actorsPerfectblockSuccessful;

	static blockHandler* GetSingleton()
	{
		static blockHandler singleton;
		return  std::addressof(singleton);
	}

	void update();

	void registerPerfectBlock(RE::Actor* actor);

	/*Process a single block.
	@param blocker: Actor who blocks.
	@param aggressor: Actor whose attack gets blocked.
	@param iHitflag: hitflag of the blocked hit, in integer.
	@param hitData: hitdata of the blocked hit.
	@param realDamage: real damage of this blocked hit.
	@return if the block is a perfect block.*/
	bool processBlock(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag, RE::HitData& hitData, float realDamage);

private:
	inline void guardBreakLarge(RE::Actor* actor, RE::Actor* actorToPush);
	inline void guardBreakMedium(RE::Actor* actor);
	inline void guardBreakSmall(RE::Actor* deflector, RE::Actor* deflected);
	inline void processStaminaBlock(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag, RE::HitData& hitData, float realDamage);



	/*Process perfect block does not take in real damage, as parry damage is re-adjusted.*/
	inline void processPerfectBlock(RE::Actor* blocker, RE::Actor* aggressor, int iHitflag, RE::HitData& hitData);

};

namespace Utils
{
	namespace sound
	{
		static inline int sub_140BEEE70(void* manager, RE::BSSoundHandle* a2, int a3, int a4)
		{
			using func_t = decltype(&sub_140BEEE70);
			REL::Relocation<func_t> func{ REL::ID(66401) };
			return func(manager, a2, a3, a4);
		}

		static inline char set_sound_position(RE::BSSoundHandle* a1, float x, float y, float z)
		{
			using func_t = decltype(&set_sound_position);
			REL::Relocation<func_t> func{ REL::ID(66370) };
			return func(a1, x, y, z);
		}

		static inline void sub_140BEDB10(RE::BSSoundHandle* a1, RE::NiAVObject* source_node)
		{
			using func_t = decltype(&sub_140BEDB10);
			REL::Relocation<func_t> func{ REL::ID(66375) };
			return func(a1, source_node);
		}

		static inline char __fastcall sub_140BED530(RE::BSSoundHandle* a1)
		{
			using func_t = decltype(&sub_140BED530);
			REL::Relocation<func_t> func{ REL::ID(66355) };
			return func(a1);
		}

		static inline void* BSAudioManager__GetSingleton()
		{
			using func_t = decltype(&BSAudioManager__GetSingleton);
			REL::Relocation<func_t> func{ RE::Offset::BSAudioManager::GetSingleton };
			return func();
		}
		/*Play sound with formid at actor.
		@param a: actor on which to play sonud.
		@param formid: formid of the sound descriptor.*/
		static inline void playSound(RE::Actor* a, int formid)
		{
			RE::BSSoundHandle handle;
			handle.soundID = static_cast<uint32_t>(-1);
			handle.assumeSuccess = false;
			*(uint32_t*)&handle.state = 0;

			auto manager = BSAudioManager__GetSingleton();
			sub_140BEEE70(manager, &handle, formid, 16);
			if (set_sound_position(&handle, a->data.location.x, a->data.location.y, a->data.location.z)) {
				sub_140BEDB10(&handle, a->Get3D());
				sub_140BED530(&handle);
			}
		}
	}
}