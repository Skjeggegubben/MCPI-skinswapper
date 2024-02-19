#include <filesystem>
#include <dirent.h>
#include <fstream> //For file reading and writing
#include <vector>
#include <bits/stdc++.h> // For std::find ?

#include <iostream>
#include <cmath>
#include <string>

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>
#include <mods/home/home.h>

#include "custom_packetstuff.h"
#include "headerstuff.h"
#include <SDL/SDL.h>

#define ARROW_RIGHT 262
#define ARROW_LEFT 263

namespace fs = std::filesystem;
using namespace std; //so that we don't need that darn std:: in front of everything!

// Declarations, declare vars and functions blah blah..
std::string skinsPath = "";
std::string overridesPath = "";

std::unordered_map<std::string, struct bgT> bgTs = {};	// Umap holding bgT.
static unsigned char *minecraft;
static b64class b64;                            // Usage: INFO( b64.enc("BMW") );
static bool game_ready = false;                 // Bool var that will be set to true once Minecraft_isLevelGenerated returns true, i.e. when game is up.
static uint64_t tickMarkerTS = 0;               // Marker timestamp, for making interval'ish behaviour instead of running all the code on every tick.
static int sec_counter = 0;                     // Int var for counting seconds between intervals.
static std::vector<unsigned char *> playerList; // Vector to hold current players. Needed for comparison.
static std::string selfNameB64 = "";            // Hold local instance username as b64.

// Stuff only for client instance:
static bool skn_server = false;     // Bool that helps determine if server supports SKN packet.
static std::string currentSkin = "";// Holds the current string skin (if got any files to grab from ofc)
static std::string currentFN = "";  // Holds the current skin File Name 
static int currentN = 0;            // To keep track of which skin file is the current one.
static int totalN = 0;              // To keep track of how many skin files to swap between.


// Stuff only for server instance:
std::unordered_map<std::string, std::string> overridesDict = {};// Client's umap holding username b64encoded + pathToFile for current userskins.
std::unordered_map<std::string, std::string> skinsDict = {};	// Server's umap holding username b64encoded + the skin file as base64 string.



// Function that trigges stuff by keypress
HOOK(SDL_PollEvent, int, (SDL_Event *event)) {
    ensure_SDL_PollEvent();
    int ret = (*real_SDL_PollEvent)(event);
    if (ret != 1 || event == NULL) return ret;
    if (event->type == SDL_USEREVENT && event->user.data1 == SDL_PRESSED) {
        if(!game_ready) return ret; // No need to do anything at start-screen etc..
        if( event->user.data2 == ARROW_LEFT || event->user.data2 == ARROW_RIGHT ){
            if( event->user.data2 == ARROW_LEFT ){
                skinswap(false);    // currentN--;
            } else if( event->user.data2 == ARROW_RIGHT ){
                skinswap(true);     // currentN++; (Can be reversed here if it feels discriminating to left-handed humans!)
            }
            
            if(on_ext_server()) {   // This must be a client instance, so let's send skin if server supports SKN.
                if(skn_server) send_skin_packet("SET " + currentSkin, NULL, NULL );
            } else {
                // If server is also a player (must be for this to be triggere?!)
                if(server_is_player()) {    // Just set it in skinsDict and send to all other users.
                    skinsDict[selfNameB64] = currentSkin; 
                    send_skin_packet(selfNameB64 + " " + currentSkin, NULL, NULL ); // Send server's skin to other users.
                }
            }
            newSkin(selfNameB64, currentSkin);
        }
    }
    return ret;
}



// Clear out old skin files from overrides folder! They pile up :P
static void clearOldFiles(){
    vector<string> all_files = dirList(overridesPath, ".png"); // Vector we'll populate with just old files to delete!
    if( all_files.size() > 0 ){
        for(std::string item : all_files) { 
            // If this file isnt in overridesDict, it can be deleted!
            bool foundIt = false;
            for ( auto& [clientB64, fileName] : overridesDict ){
                if(item == fileName) foundIt = true;
            }
            //std::string fullPath = overridePath + item;
            std::string fullPath = overridesPath + item;
            if(!foundIt) std::remove(fullPath.c_str() ); // delete file
        }
        INFO("[SKINSWAPPER]: OLD SKIN FILE OVERRIDES DELETED!");    
    }
}



// Some background work to be done for both client and server instance.
static void update_dicts_and_files(){
    std::vector<unsigned char *> tmpList = get_all_players(); 
    
    if(tmpList.size() == 0 && playerList.size() == 0) return;
    vector<string> list_b64encoded; // To make a list of the current player names in b64, to cross-check with skinsDict! 
    vector<string> removeables;     // To make a list of old overrides files to delete from disk and from overridesDict! 
    
    if(tmpList != playerList){ // There's been a change, do some updating stuff...
        // Loop through fresh player list, make b64-version of list while at it so we can add new/remove 'dead' in dicts.
        for (uchar *player : tmpList ) { 
            std::string b64name = b64.enc( get_safe_username(player) );
            list_b64encoded.push_back(b64name);
            // Check if this one is in the dict, if not, set it in dict!
            if (skinsDict.find(b64name) == skinsDict.end()) { // Is not in dict, new client
                INFO("[SKINSWAPPER]: NEW CLIENT '%s'", get_safe_username(player).c_str() );
                skinsDict[b64name] = ""; // Set it to empty, if client has SKN he will provide b64 skin data later!
            }
        }
        // Make sure to update the comparing list var.
        playerList = tmpList;
        
        // Now we also check the skinsDict for 'dead' clients to remove! If in skinsDict but no longer in playerList!
        for ( auto& [clientB64, skn_data] : skinsDict ){
            if(!in_vec(list_b64encoded, clientB64) ){
                if( clientB64 != selfNameB64 ) {
                    removeables.push_back(clientB64);
                    INFO("[SKINSWAPPER]: CLIENT DISCONNECTED '%s'", b64.dec( clientB64 ).c_str() );
                }
            }
        }
        for(std::string item : removeables) skinsDict.erase(item);      // Loop through the list and delete from skinsDict.
        for(std::string item : removeables) overridesDict.erase(item);  // Same with overridesDict..
        clearOldFiles(); // Finally, clear out files that are not in overridesDict.
    }
}



// The function that will run on every tick.
void sknTickFunction(unsigned char *mcpi){  
	if (mcpi == NULL) {                     // Ticks start running before we have mcpi, and ofc
        return;                             // without mcpi there is nothing more to do here but return.
    }
    int x = (game_ready) ? 1000 : 500;      // Half sec interval if game isn't actually started yet, then one sec interval.

	if(tickMarkerTS < 1) { 
		tickMarkerTS = get_ms_unixTS();     // Variable could be still NULL, which is also less than 1, then initiate it, make new timestamp.
	}
    uint64_t nowTS = get_ms_unixTS();		// Making a new timestamp on each tick, to compare with marker. Maybe kinda stupid waste of cpu resources?
    
    if( (*Minecraft_isLevelGenerated)(mcpi)  ) update_dicts_and_files(); // Make sure we _always_ got dicts and files in order!
    
    
    if( (nowTS - tickMarkerTS) > x ){	    // If time now, minus time when tickMarkerTS was set, equals more than 1000 millisec ( 1 sec ),
        tickMarkerTS = get_ms_unixTS();		// then just set a new marker timestamp, and below comes the stuff to repeat at ~1 second interval.


        if(!game_ready){    // This code part is to be executed only while game isnt up i.e. level not generated yet. Once it finds that
                            // level is generated, it is time to do important stuff needed at game startup. Afterwards this part gets skipped.
            
            if ((*Minecraft_isLevelGenerated)(mcpi)){   // Got level, minecraft obj, all up and running and all is presumably good to go!
                selfNameB64 = b64.enc( *default_username );
                INFO("[SKINSWAPPER]: GAME IS READY!");
                skn_server = false;         // This turns to 'true' if server responds to SKN request.
                game_ready = true;          // Set bool to true so this wont be repeated!
                sec_counter = 0;            // Counter must be reset to ensure 0!
                
                if(!on_ext_server() ) { // Procedures needed for server at game startup, can be done here:
                    // This instance is server.. but if player also, there's work to do!
                    if(server_is_player() ){
                        // Just insert skin into the skinsDict and set the skin, there's nobody to send it to yet.
                        skinsDict[selfNameB64] = currentSkin;
                        newSkin(selfNameB64, currentSkin);
                    }
                    
                } else { // Procedures needed at game startup as client, can be done here:
                        bgT initLoader1 = bgT();
                        bgTs["initLoader1"] = initLoader1;
                        bgTs["initLoader1"].setTimeout([&](std::string str1, std::string str2) {            
                            send_skin_packet("SKN", NULL, NULL ); // SEND 1ST PACKET, ASK IF SERVER SUPPORTS SKN.
                        }, 1000, "", ""); // This is just to give server a sec. to register that client connected!

                        // In case server doesn't reply, i.e. doesn't have this mod, the fallback is to just inform about it:
                        bgT initLoader2 = bgT();
                        bgTs["initLoader2"] = initLoader2;
                        bgTs["initLoader2"].setTimeout([&](std::string str1, std::string str2) {
                            // If no reply from server, bool skn_server is still false.
                            if(!skn_server) showInfo("Server doesn't support skinswap mod.");
                            // Prevent server trying to fool us later with false skin strings he supposedly don't support!?
                        }, 5000, "", ""); // Will be executed 5 sec from now. Server reply should be instant within 1-2s.
                }
            } else return;
        } else {
            // THIS PART repeats every sec. after game has started.
            if( !(*Minecraft_isLevelGenerated)(mcpi) ){ // Check if game still up once per sec.
                game_ready = false;
                INFO("[SKINSWAPPER]: GAME OVER!");
            } else {
                /*
                // The game is still running, so here's the spot for in-game interval'ed stuff by second counts:
                if(!self_is_server() ){ // Client can do stuff every 30 sec.
                    sec_counter++;
                    if(sec_counter == 30){
                        // If client instance needs to do something in 30 sec. intervals put it here.
                        sec_counter = 0;
                    }
                } else { // Server side can do intervals action too, with other count.
                    sec_counter++;
                    if(sec_counter == 60){
                        // server_function_here();
                        sec_counter = 0;
                    }
                }*/
            }
        }
    }
}


// Parse the string from inventory packet and do different stuff as result.
void parse_skin_packet(std::string packetStr, RakNet_RakNetGUID *guid, uchar *callback){ 
    // Called from friendly custompackets.cpp each time there's some mail <3
    if(self_is_server() ){
        std::string unStr = guid2un(guid);
        if(unStr == "" ) return; // Might have disconnected?
        std::string clientUNb64 = b64.enc(unStr); // Use this safe b64-string in the dict. 
        //INFO("Packet from: '%s'", clientUN.c_str() );
        // Commands: 'SKN', 'GET [userb64]' or 'SET [skinB64]'
        
        std::vector<std::string> data = split_str(packetStr, ' ');
        if( (data.size() != 1) && (data.size() != 2) ) return;                      // Malformed shiet! 
        if( (data[0] != "SKN") && (data[0] != "GET") && (data[0] != "SET") ) return;// Malformed packet or hackery attempt!
        // The arg data[1] must be username with GET, or skin string with SET.
        

        if(  data[0] == "SKN" ){ // Client connected and wants to know if server supports SKN packets!
            if( data.size() != 1 ) return;              // Hakcry mfukker! SKN command has no args!
            send_skin_packet( "SKN", guid, callback );  // Let client know SKN is supported!
            
        } else if( data[0] == "SET" ){ // Client wants server to do the favor of saving his skin data.. Is it really skin data?
            if( data.size() != 2) return;               // Malformed piece of sheeit! 
            if(!isValidBase64(data[1]) ) return;        // Not a base64 string! Wtf is this hacker trying to do here?
            if( data[1].length() > 3000 ) return;       // This is NOT new free unlimited Dropbox storage for dumb hackrs!
            if(!isSkinString( data[1] ) ) return;       // No thanks, only skin data pls, gtfo!
            skinsDict[clientUNb64] = data[1];           // Save it to dict and reply 'OK'
            send_skin_packet( "OK", guid, callback );   // Send reensuring reply to client
            redistribute_skin_packet( clientUNb64 +" "+ data[1] , guid, callback); // Redistribute to the other connected clients!
            if( server_is_player() ) newSkin( clientUNb64, data[1]); // So a server-player also see the skinswaps of others.
            

        } else if( data[0] == "GET" ) { // must be GET then.. Client asks for a user's skin string via arg nr.2 / data[1]
            if( data.size() != 2 ) return; // Malformed hecketry!
            if( data[1].length() > 50 ||  !isValidBase64(data[1]) ) return; // Not base64-string of the player username, hacker basterd gtfo!!
            std::string targetB64 = data[1];
            if (skinsDict.find(targetB64) == skinsDict.end()) { // Is not in dict, no such user?
                INFO("[SKINSWAPPER]: *** WARNING: request for NON-EXISTING USER SKIN?");
            } else {
                std::string skinData = skinsDict[ data[1] ];
                std::string replyStr = data[1] + " " + skinData;
                if(skinData != "") send_skin_packet( replyStr, guid, callback ); // There's data to serve, so serve it!
            }
        } // else eh.. nothing to do, client wanna send invalid maneure then just ignore..

    } else { // WHEN THIS INSTANCE IS A CLIENT - Expect either 'SKN', 'OK', or a username + base64 string from server.
        
        if(packetStr == "SKN"){ // Reply on 1st packet. Server supports SKN!
            // NOW IS THE TIME TO SET LOCAL SKIN, SEND LOCAL SKIN, AND ASK FOR SKINS FOR ALL PLAYERS!
            skn_server = true;
            //INFO("Server supports SKN!"); // For debugging.
            send_skin_packet("SET " + currentSkin, NULL, NULL );
            newSkin( selfNameB64 , currentSkin);
            showInfo("Skin: " + currentFN);
            for(std::string playerNameB64 : get_b64playerNames() ){
                send_skin_packet("GET " + playerNameB64 , NULL, NULL ); //send_skin_packet("GET " + b64.enc("Skjeggegubben"),NULL,NULL);
            }

        } else if(packetStr == "OK" && skn_server){ // Server has accepted SET packet, no problems, just be happy!
            //INFO("'OK' WAS RECEIVED!!!"); // For debugging.
            
        
        } else { // Most probably another player has SET his skin and server redistributed to everybody!
            if(!skn_server) return;                                     // Server supposedly not SKN, but sends crap?!
            //INFO(" *** GOT OTHER THAN 'SKN'/'OK'!");                  // Expecting b64username and base64 skin only!
            std::vector<std::string> data = split_str(packetStr, ' ');
            if( (data.size() != 2) ) return;                            // Malformed shiet!
            std::string b64name = data[0];
            std::string b64skin = data[1];
            if( !isValidBase64( b64name ) || !isValidBase64( b64skin ) ) return;    // Not base64 string! Heckery server fukcer hm?
            if(!in_vec(get_b64playerNames(), b64name) ) return;                     // This is not recognized as a logged on user!
            if( b64skin.length() > 3000 || !isSkinString( b64skin ) ) return;       // No thanks gtfo! Sending invalid data maliciously?!
            newSkin(b64name, b64skin); // Skin data seems valid :s Load it up and we'll see :3
        }
    }
}



// Function to set the local player HUD, e.g. the arm poking out when not holding anything.
static void setLocalClientHUD(std::string fileName){ 			
	std::string tmpStr = "skins/" + fileName + ".png";	    // Should change the texture instantly.
	char *local_skin = new char[tmpStr.size() + 1]; 		// Entire array of 0 for each letter in string, and one additional 0.
	strcpy ( local_skin, tmpStr.c_str() );					// Copy string into the char * variable.
	patch_address((void *) 0x4c294, (void *) local_skin ); 	// Patch addr with char * variable that has the proper standard 0 end byte.
}



// Function that links override skin to a player obj. Actually just setting the path to an override fileName.
static void setPlayerSkin(unsigned char *player, std::string fileName){
	*(std::string *)(player + 0xb54) = "skins/" + fileName + ".png"; 	// Should change player entity texture instantly.
}



// Save skin to a random filename in overrides dir, link it in client to a player.
static void newSkin(std::string playerNameB64, std::string skinB64){
    if(skinB64 == "") return;                       // Error occurred, maybe stupid user actually deleted all files in his SKINS_DIR?
	uchar *player = b64un2player(playerNameB64);    // Get the player obj.
	if(player != NULL){
        std::string fileName = get_randStr(25);		// Making long random filename, to not confuse with any existing skin files.
        std::string thePath = overridesPath;
        thePath.append(fileName+".png");
        // Set values in dict
        overridesDict[playerNameB64] = fileName + ".png";   // So that we can delete overrides files that arent in dict later!
        file_put_contents(thePath, b64.dec( skinB64 ) );
        setPlayerSkin(player, fileName);				    // Sets the skin texture on the player once the file is in place.
                                                            // If this is the local player, also set the HUD, i.e. player arm.
        if( playerNameB64 == selfNameB64 ) setLocalClientHUD(fileName);
        clearOldFiles(); // Finally, clear out files that are not in overridesDict.
	}
}



// Function loads next/previous skin to b64 string from files 
void skinswap(bool increment){ // Either increment (currentN++;), or opposite (currentN--;)
    // On init-run, this is called before game is ready, when currentSkin == "" and currentN == 0.
    std::string thePath = skinsPath;  // SKINS_DIR
    
    vector<string> filesList = dirList(thePath, ".png");
    totalN = filesList.size();  // Now we know the total number of skin files is filesList.size()
    if(totalN == 0) {           // Not much to do with zero skin files..
        showInfo("Empty SKINS_DIR! Make MCPI skins: glim.re/skinny/");
        currentFN = "";         // Maybe reset some values..
        currentN = 0;
        currentSkin = "";
        return;
    } else { // Got skin files here! We were at currentN before, check if that file is still in the list and set currentN
        if( in_vec(filesList, currentFN) ){ // Old skin file still there, so get which number it is in the list now!
            for (int i = 0; i < totalN; i++){
                if( filesList[i] == currentFN ) currentN = i;
                break;
            }
        }
    }
    // Handle the incr/decr. of the currentN..
    if(totalN == 1 || currentSkin == ""){ // But.. At init-run, or if there's only 1 skin file, there's no reason to incr/decr!
        currentN = 0;
    } else {
        if(increment){
            currentN++;
        } else currentN--;
        // At this point, currentN might be equal to or more than totalN, or less than 0, that must be fixed!
        if(currentN >= totalN) currentN = 0; // Making sure it just goes round and round..
        if(currentN < 0) currentN = (totalN - 1); // the last file is at totalN - 1
    }
    // At this point, we have a valid currentN and can load the right file into currentSkin, set currentFN..
    currentFN = filesList[currentN];
    showInfo("Skin: " + currentFN);
    std::string pathToFile = thePath + currentFN;
    std::string fileData = file_get_contents(pathToFile);
    currentSkin = b64.enc(fileData);
}

// Returns a vector of all players
std::vector<uchar *> get_all_players() {
    uchar *level = *(uchar **) ( get_minecraft() + Minecraft_level_property_offset);
    return *(std::vector<uchar *> *) ( level + Level_players_property_offset);
}


// The functions that catches the minecraft obj for us
static void callback_for_mcpiskinswaps(unsigned char *mcpi) { // Runs on every tick, sets the minecraft var.
    minecraft = mcpi; 
    sknTickFunction(mcpi); // From here we can call a custom function to run on every tick
}



// Helper function. Returns the minecraft obj. 
unsigned char *get_minecraft() {
    return minecraft;
}



// Function that determines if on external server or not
static bool on_ext_server() {
    if( self_is_server() ) return false;
    bool on_ext_srv = *(bool *) (*(unsigned char **) (get_minecraft() + Minecraft_level_property_offset) + 0x11);
    return on_ext_srv;
}



// Function that determines if server is also player, if not then it is dedi server..
static bool server_is_player(){
    // If this is a dedi server, *default_username wont be in the player list!
    bool self_in_playerslist = false;
    for (uchar *player : get_all_players()) {
        if ( get_safe_username(player) == *default_username ) { 
            self_in_playerslist = true;
            break;
        }
    }
    return ( ( self_is_server() ) && ( self_in_playerslist ) ) ? true : false;
}


// Function to check if game instance is currently acting as server
static bool self_is_server(){
    unsigned char *rak_net_instance = *(unsigned char **) (get_minecraft() + Minecraft_rak_net_instance_property_offset);
    if( rak_net_instance == NULL ) return false;
    unsigned char *rak_net_instance_vtable = *(unsigned char**) rak_net_instance;
    RakNetInstance_isServer_t RakNetInstance_isServer = *(RakNetInstance_isServer_t *) (rak_net_instance_vtable + RakNetInstance_isServer_vtable_offset);
    return ((*RakNetInstance_isServer)(rak_net_instance)) ? true : false;
}



//The "main" function that starts when you run game
__attribute__((constructor)) static void init() {
    misc_run_on_update(callback_for_mcpiskinswaps);
}