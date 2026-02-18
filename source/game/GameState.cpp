/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/game/GameState.cpp
 *****************************************************************************/

#include "GameState.h"
#include "CEntryExitManager.h"
#include "CEntryExit.h"

bool GameState::IsPlayerInInterior()
{
    CEntryExit* currentInterior = CEntryExitManager::mp_Active;
    if (currentInterior == nullptr)
    {
        int stackPosn = CEntryExitManager::ms_entryExitStackPosn;
        if (stackPosn > 0 && stackPosn <= 4)
        {
            try
            {
                currentInterior = CEntryExitManager::ms_entryExitStack[stackPosn - 1];
            }
            catch (...)
            {
            }
        }
    }
    return (currentInterior != nullptr);
}
