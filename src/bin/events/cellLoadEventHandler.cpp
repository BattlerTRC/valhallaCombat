#include "include/events.h"

EventResult cellLoadEventHandler::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* a_eventSource) {
	//DEBUG("cell load event");
	return EventResult::kContinue;
}
