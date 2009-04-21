/*
 * ============================================================================
 *
 *   Zombie:Reloaded
 *
 *   File:        roundend.inc
 *   Description: (Core) Handles round end actions.
 *
 * ============================================================================
 */

/**
 * @section All round end reasons.
 */
#define ROUNDEND_TARGET_BOMBED							1		// Target Successfully Bombed!
#define ROUNDEND_VIP_ESCAPED							2		// The VIP has escaped!
#define ROUNDEND_VIP_ASSASSINATED						3		// VIP has been assassinated!
#define ROUNDEND_TERRORISTS_ESCAPED						4		// The terrorists have escaped!
#define ROUNDEND_CTS_PREVENTESCAPE						5		// The CT's have prevented most of the terrorists from escaping!
#define ROUNDEND_ESCAPING_TERRORISTS_NEUTRALIZED		6		// Escaping terrorists have all been neutralized!
#define ROUNDEND_BOMB_DEFUSED							7		// The bomb has been defused!
#define ROUNDEND_CTS_WIN								8		// Counter-Terrorists Win!
#define ROUNDEND_TERRORISTS_WIN							9		// Terrorists Win!
#define ROUNDEND_ROUND_DRAW								10		// Round Draw!
#define ROUNDEND_ALL_HOSTAGES_RESCUED					11		// All Hostages have been rescued!
#define ROUNDEND_TARGET_SAVED							12		// Target has been saved!
#define ROUNDEND_HOSTAGES_NOT_RESCUED					13		// Hostages have not been rescued!
#define ROUNDEND_TERRORISTS_NOT_ESCAPED					14		// Terrorists have not escaped!
#define ROUNDEND_VIP_NOT_ESCAPED						15		// VIP has not escaped!
#define ROUNDEND_GAME_COMMENCING						16		// Game Commencing!
/**
 * @endsection
 */

/**
 * Delay between round ending and new round starting. (Normal)
 */
#define ROUNDEND_DELAY_NORMAL 5.0

/**
 * Delay between round ending and new round starting. (Short, for ROUNDEND_GAME_COMMENCING)
 */
#define ROUNDEND_DELAY_SHORT 3.0
 
/**
 * Possible round end outcomes.
 */
enum RoundEndOutcome
{
    HumansWin, /** Humans have killed all zombies. */
    ZombiesWin, /** Zombies have infected all humans. */
    Draw, /** Round has ended in unexpected way. */
}

/**
 * Global variable to store round win timer handle.
 */
new Handle:tRoundEnd = INVALID_HANDLE;

/**
 * Global variable to store a convar query cookie
 */ 
new QueryCookie:mat_dxlevel;

/**
 * Map is starting.
 */ 
RoundEndOnMapStart()
{
    // Reset timer handle.
    tRoundEnd = INVALID_HANDLE;
}

/**
 * Client is joining the server.
 * 
 * @param client    The client index.  
 */ 
RoundEndClientInit(client)
{
    // Get client's DX level.
    RoundEndGetClientDXLevel(client);
}

/**
 * The round is starting.
 */
RoundEndOnRoundStart()
{
    // If round end timer is running, then kill it.
    if (tRoundEnd != INVALID_HANDLE)
    {
        // Kill timer.
        KillTimer(tRoundEnd);
        
        // Reset timer handle.
        tRoundEnd = INVALID_HANDLE;
    }
    
    // Balance teams, and respawn all players.
    RoundEndBalanceTeams(true);
}

/**
 * The freeze time is ending.
 */
RoundEndOnRoundFreezeEnd()
{
    // Calculate round length, in seconds.
    // Get mp_roundtime. (in minutes)
    new Float:roundtime = GetConVarFloat(FindConVar("mp_roundtime"));
    
    // Convert to seconds.
    roundtime *= 60.0;
    
    // Start timer.
    tRoundEnd = CreateTimer(roundtime, RoundEndTimer, _, TIMER_FLAG_NO_MAPCHANGE);
}

/**
 * The round is ending.
 * 
 * @param reason    Reason the round has ended.
 */
RoundEndOnRoundEnd(reason)
{
    // If round end timer is running, then kill it.
    if (tRoundEnd != INVALID_HANDLE)
    {
        // Kill timer.
        KillTimer(tRoundEnd);
        
        // Reset timer handle.
        tRoundEnd = INVALID_HANDLE;
    }
    
    // Get outcome of the round.
    new RoundEndOutcome:outcome = RoundEndReasonToOutcome(reason);
    
    // Display the overlay to all clients.
    RoundEndOverlayStart(ROUNDEND_DELAY_NORMAL, outcome);
    
    // Balance teams.    
    RoundEndBalanceTeams();
}

/**
 * Finds DX level of a client.
 * 
 * @param client    The client index.  
 */
RoundEndGetClientDXLevel(client)
{
    // If client is fake (or bot), then stop.
    if (IsFakeClient(client))
    {
        return;
    }
    
    // Query mat_dxlevel on client.
    mat_dxlevel = QueryClientConVar(client, "mat_dxlevel", RoundEndQueryClientDXLevel);
}

/**
 * Query callback function.
 *
 * @param cookie    Unique cookie of the query.
 * @param client    The client index.
 * @param result    The result of the query (see console.inc enum ConVarQueryResult)
 * @param cvarName  Name of the cvar.
 * @param cvarValue Value of the cvar.
 */       
public RoundEndQueryClientDXLevel(QueryCookie:cookie, client, ConVarQueryResult:result, const String:cvarName[], const String:cvarValue[])
{
    // If query cookie does not match cookie given by mat_dxlevel query, then stop, this isn't our query.
    if (cookie != mat_dxlevel)
    {
        return;
    }
    
    // Reset dxLevel.
    dxLevel[client] = 0;
    
    // If result is any other than ConVarQuery_Okay, then stop.
    if (result != ConVarQuery_Okay)
    {   
        return;
    }
    
    // Copy cvar value to dxLevel array.
    dxLevel[client] = StringToInt(cvarValue);
}

/**
 * Convert a round_end reason, to a round winner, or draw.
 * 
 * @param reason    The round_end reason.
 * @return          The winner of the round. (see enum RoundEndOutcome)
 */ 
RoundEndOutcome:RoundEndReasonToOutcome(reason)
{
    switch(reason)
    {
        // CTs won the round.
        case ROUNDEND_CTS_WIN:
        {
            return HumansWin;
        }
        // Ts won the round.
        case ROUNDEND_TERRORISTS_WIN:
        {
            return ZombiesWin;
        }
        // Unexpected case.
        default:
        {
            return Draw;
        }
    }
    
    // Return draw to satisfy compiler. (code will never reach this point.)
    return Draw;
}

/**
 * Timer callback, called when round time reaches 0.
 *
 * @param timer     The timer handle.
 */
public Action:RoundEndTimer(Handle:timer)
{
    // Terminate the round with a normal delay, and CTs as the winner.
    TerminateRound(ROUNDEND_DELAY_NORMAL, ROUNDEND_CTS_WIN);
}

/**
 * Checks if the round is over.
 * 
 * @param outcome   Set to the outcome of the round, if round is over.
 * @return          True if the round is over, false otherwise.
 */
bool:RoundEndGetRoundStatus(&RoundEndOutcome:outcome)
{
    // If zombie hasn't spawned, then stop.
    if (!g_bZombieSpawned)
    {
        // Round isn't over.
        return false;
    }
    
    // Initialize count variables
    new zombiecount;
    new humancount;
    
    // Count valid clients. (true to only allow living clients)
    ZRCountValidClients(zombiecount, humancount);
    
    // If there are no clients on either teams, then stop.
    if (!zombiecount && !humancount)
    {
        // Round isn't active.
        return false;
    }
    
    // If there are clients on both teams, then stop.
    if (zombiecount && humancount)
    {
        // Round isn't over.
        return false;
    }
    
    // We know here, that either zombiecount or humancount is 0. (not both)
    
    // If there are zombies, then zombies won the round.
    if (zombiecount)
    {
        outcome = ZombiesWin;
    }
    // If there are no zombies, that means there must be humans, they win the round.
    else
    {
        outcome = HumansWin;
    }
    
    // Round is over.
    return true;
}

/**
 * Ends the round with the given outcome.
 * 
 * @param outcome   The outcome of the round.
 */
RoundEndTerminateRound(RoundEndOutcome:outcome)
{
    switch(outcome)
    {
        // Zombies won.
        case ZombiesWin:
        {
            // Terminate the round with a normal delay, and Ts as the winner.
            TerminateRound(ROUNDEND_DELAY_NORMAL, ROUNDEND_TERRORISTS_WIN);
        }
        // Humans won.
        case HumansWin:
        {
            // Terminate the round with a normal delay, and CTs as the winner.
            TerminateRound(ROUNDEND_DELAY_NORMAL, ROUNDEND_CTS_WIN);
        }
    }
}

/**
 * Balances teams
 * 
 * @param spawn     If true, it will respawn player after switching their team.
 */
RoundEndBalanceTeams(bool:spawn = false)
{
    // Create eligible player list.
    new Handle:arrayEligibleClients = INVALID_HANDLE;
    new eligibleclients = CreateEligibleClientList(arrayEligibleClients, true);
    
    // If there are no eligible client's then stop.
    if (!eligibleclients)
    {
        return;
    }
    
    new client;
    
    // Move all clients to T
    
    // x = array index.
    // client = client index.
    for (new x = 0; x < eligibleclients; x++)
    {
        // Get client stored in array index.
        client = GetArrayCell(arrayEligibleClients, x);
        
        // Switch client to T
        CS_SwitchTeam(client, CS_TEAM_T);
    }
    
    // Move every other client back to CT
    
    // x = array index
    // client = client index.
    for (new x = 0; x < eligibleclients; x += 2)
    {
        // Get client stored in array index.
        client = GetArrayCell(arrayEligibleClients, x);
        
        // Switch client to CT
        CS_SwitchTeam(client, CS_TEAM_CT);
    }
    
    // If spawn is false, then stop.
    if (!spawn)
    {
        return;
    }
    
    // x = array index.
    // client = client index.
    for (new x = 0; x < eligibleclients; x++)
    {
        // Get client stored in array index.
        client = GetArrayCell(arrayEligibleClients, x);
        
        // If client is dead, then stop.
        if (!IsPlayerAlive(client))
        {
            continue;
        }
        
        // Respawn client.
        CS_RespawnPlayer(client);
    }
    
    // Destroy handle.
    CloseHandle(arrayEligibleClients);
}

/**
 * Displays overlay to client, or prints unsupported message on client's screen.
 * 
 * @param client    The client index.
 * @param overlay   The overlay path.
 */    
RoundEndDisplayClientOverlay(client, const String:overlay[])
{
    // If dxLevel is 0, then query on client failed, so try again, then stop.
    if (!dxLevel[client])
    {
        // Query dxlevel cvar.
        RoundEndGetClientDXLevel(client);
        return;
    }
    
    // If dxLevel is above or equal to minimum requirement, then display overlay.
    if (dxLevel[client] >= GENERAL_MIN_DXLEVEL)
    {
        ClientCommand(client, "r_screenoverlay \"%s\"", overlay);
    }
    // If client doesn't meet minimum requirement, then print unsupported text.
    else
    {
        ZR_PrintCenterText(client, "DX90 not supported", dxLevel[client], GENERAL_MIN_DXLEVEL);
    }
}

/**
 * Displays overlays to clients, depending on the outcome.
 * 
 * @param time      Time to display overlays.
 * @param outcome   The outcome of the round.
 */
RoundEndOverlayStart(Float:time, RoundEndOutcome:outcome)
{
    // If round end overlays are disabled, then stop.
    new bool:overlay = GetConVarBool(g_hCvarsList[CVAR_ROUNDEND_OVERLAY]);
    if (!overlay)
    {
        return;
    }
    
    decl String:overlaypath[PLATFORM_MAX_PATH];
    
    switch(outcome)
    {
        // Show "zombies win" overlay.
        case ZombiesWin:
        {
            GetConVarString(g_hCvarsList[CVAR_ROUNDEND_OVERLAY_ZOMBIE], overlaypath, sizeof(overlaypath));
        }
        // Show "humans win" overlay.
        case HumansWin:
        {
            GetConVarString(g_hCvarsList[CVAR_ROUNDEND_OVERLAY_HUMAN], overlaypath, sizeof(overlaypath));
        }
        // Show no overlay.
        default:
        {
            strcopy(overlaypath, sizeof(overlaypath), "");
        }
    }
    
    // x = client index.
    for (new x = 1; x <= MaxClients; x++)
    {
        // If client isn't in-game, then stop.
        if (!IsClientInGame(x))
        {
            continue;
        }
        
        RoundEndDisplayClientOverlay(x, overlaypath);
    }
    
    CreateTimer(time, RoundEndOverlayTimer, _, TIMER_FLAG_NO_MAPCHANGE);
}

RoundEndOverlayStop()
{
    // x = client index.
    for (new x = 1; x <= MaxClients; x++)
    {
        // If client isn't in-game, then stop.
        if (!IsClientInGame(x))
        {
            continue;
        }
        
        // Removes overlay from client's screen.
        ClientCommand(x, "r_screenoverlay \"\"");
    }
}

/**
 * Timer callback, stops overlays on all clients.
 * 
 * @param timer     The timer handle.
 */
public Action:RoundEndOverlayTimer(Handle:timer)
{
    // Stop all overlays.
    RoundEndOverlayStop();
}