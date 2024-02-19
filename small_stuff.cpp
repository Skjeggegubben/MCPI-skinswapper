#include <filesystem>
#include <regex>
#include <fstream> //For file reading and writing
#include <cstdio> // For file remove (delete)
#include <cstdlib>
#include <dirent.h>
#include <algorithm>
#include <random>

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>

#include "headerstuff.h"

namespace fs = std::filesystem;
using namespace std; //so that we don't need that darn std:: in front of everything!

static b64class b64;
//std::unordered_map<std::string, std::string> invKeyDict = {};	// Umap holding server name and client_id key


// Function to make unix timestamp in ms since epoch.
uint64_t get_ms_unixTS(){
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}



// Check if path+dir exists
static bool existsDir(const std::string& thePath) {
    fs::path directory(thePath);
    return fs::exists(directory) && fs::is_directory(directory);
}



// Function for creating the dirs we need for storage
static void createDir(const std::string& thePath) {
    fs::path dir(thePath);
    fs::create_directories(dir);
}



// For saving string to file, named after the php-function
void file_put_contents(std::string thePath, std::string saveStr){
    ofstream outfile;
    outfile.open(thePath, std::ios_base::trunc); // 'app' for append, 'trunc' overwrite
    outfile << saveStr;
    outfile.close();
}



// For reading the contents of a file, named after the php-function
std::string file_get_contents(std::string file){
    std::ifstream input_file( file );
    std::string resultStr = "";
    if ( ( !input_file.good() ) || ( !input_file.is_open() )  ) { 
        ERR("Error with file %s", file.c_str());
    } else { 
        std::string theLine;
        while (std::getline(input_file, theLine)){
            resultStr += theLine + '\n';
        }
    }
    resultStr.pop_back(); // Remove nasty linebreak!
    return resultStr; 
}



// Function to quickly determine if string is found in vector.
bool in_vec(std::vector<std::string> inputVec, std::string strValue){
    if ( std::find(inputVec.begin(), inputVec.end(), strValue ) != inputVec.end() ) return true;
	return false;
}



// Nice split_str function
vector<string> split_str(string str, char theChar) {
    stringstream ss(str);
    vector<string> ret;
    string tmp = "";
    while (getline(ss, tmp, theChar)) {
        ret.push_back(tmp);
    }
    return ret;
}



// Check if a string contains only valid b64 chars
bool isValidBase64(const std::string& input) {
    std::regex base64_regex("[A-Za-z0-9+/]*={0,2}");
    return std::regex_match(input, base64_regex);
}



// For making a string of chars shuffle into a random order. Used by get_randStr
std::string shuffleString(const std::string& str) {
    std::vector<char> chars(str.begin(), str.end());
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(chars.begin(), chars.end(), g);
    return std::string(chars.begin(), chars.end());
}



// Function that returns a random-ish string of given len.
std::string get_randStr(int length) {
    static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result;
    result.resize(length);

    srand(time(NULL));
    for (int i = 0; i < length; i++) result[i] = charset[rand() % charset.length()];
    return shuffleString(result); // It is based on time().. shuffle a bit before returning!
}



// List files in given dir and of given extention (".png" in this case)
std::vector<std::string> dirList(std::string thePath, std::string fileExt){
	vector<string> filesList; // Vector to push the filenames into!
    struct dirent *entry = nullptr;
    DIR *dp = nullptr;
    dp = opendir(thePath.c_str() ); // Var thePath must be full path and entire filename.
    if (dp != nullptr) {
        while ((entry = readdir(dp))){
            std::string filenameStr = entry->d_name;
			if( filenameStr != "." && filenameStr != ".." && ( strEndsWith( lower_cased(filenameStr), fileExt) ) ){
                filesList.push_back(filenameStr); // that's it then!
			}
		}
    }
    closedir(dp);
    
    // Sort the vector alphabetically using lambda expression.
    std::sort(filesList.begin(), filesList.end(), [](const std::string& a, const std::string& b) {
        return a < b; // Use lexicographical comparison.
    });
    
    return filesList;
}



// Check if a string contains only chars in given charset
bool all_allowed_chars(const std::string& str) {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    return str.find_first_not_of(charset) == std::string::npos;
}


// Turn a string to lower, useful for sanitizing input etc.
std::string lower_cased(std::string inputStr){
    std::string retStr = "";
    for(auto c : inputStr){   
          retStr += (char)tolower(c); 
    }
    return retStr;
}


// Get only a set length of the entire contents of a string.
std::string shortened(std::string inputStr, int max){
    std::string retStr = "";
    int counter = 0;
    for(auto c : inputStr){   
          retStr += c;
        counter++;
        if(counter == max) break;
    }
    return retStr + "[...]";
}


// Function to do like endswith in other languages.
bool strEndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.empty()) {
        return true; // Any string ends with an empty suffix
    } else if (text.size() < suffix.size()) {
        return false; // Suffix is longer than the string
    }
    return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}


// Function to do pretty much same as startswith function in other languages.
bool strStartsWith(std::string inputStr, std::string prefixStr){			
	if(inputStr.rfind(prefixStr, 0) == 0) return true;
	return false;
}



// Check if a number is power of 2 (for bit depth checks)
static bool isPowerOf2(int n) {
  return n > 0 && (n & (n - 1)) == 0;
}

// Function for sanitizing received skin data 
bool isSkinString(std::string skinB64){
    /* First 8 bytes are always the PNG header, which is:
    // "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A" --> "iVBORw0KGgo=" // PNG header
    // '137' / x89  - '80' / x50 / '78' - x4E / '71' - x47 / '13' - x0D / '10' - x0A / '26' - x1A / '10' - x0A
                        P           N           G !
    */
    if( !strStartsWith(skinB64, "iVBORw0KGgo") ) return false;
    
    std::string skinPNG = b64.dec(skinB64); // Get file data as string, check bytes
    
    // Check for the width and height 4-byte integers ( 0, 0, 0, 64/32)
    for (int i = 16; i < 19; ++i) if( skinPNG[i] != 0 ) return false; // Width 4-byte integer ERROR!
    if( skinPNG[19] != 64 ) return false; // Not width 64 pixels!
    for (int i = 20; i < 23; ++i) if( skinPNG[i] != 0 ) return false; // Height 4-byte integer ERROR!
    if( skinPNG[23] != 32 ) return false; // Not height 32 pixels!
    
    // Read color type and bit depth
    int color_type, bit_depth;
    bit_depth = (int)skinPNG[24]; // Supposedly at byte 24 is the bit depth value :S
    ///INFO("bit_depth = '%d'", bit_depth); // Usually 8 for these skin png's..
    color_type = (int)skinPNG[25];  // Color type is supposedly at byte 25 then..
    //INFO("color_type = '%d'", color_type); // Usually 6 for these skin png's..

    // Check color type and bit depth validity
    if (color_type < 0 || color_type > 6 || !isPowerOf2(bit_depth)) {
        INFO("Error: Unsupported color type or bit depth.");
        return false;
    }
    
    // TODO: Add more sanitation to check verify actual pixel count and pixel data!
    return true;
}



// Function to check if a string is digits or not
static bool isNumeric(const std::string& str) {
    const std::regex pattern("^[0-9]+$");
    return std::regex_match(str, pattern);
}



// Function for printing some debug info directly to game screen
void showInfo(string text){
    if( get_minecraft() == NULL){ 
        INFO("%s", text.c_str() );
    } else { 
        unsigned char *gui = get_minecraft() + Minecraft_gui_property_offset;
        misc_add_message(gui, text.c_str() ); 
    }
}


// Get Player's Username
std::string get_safe_username(unsigned char *player) {
    std::string *username = (std::string *) (player + Player_username_property_offset);
    char *safe_username_c = from_cp437(username->c_str());
    std::string safe_username = safe_username_c;
    free(safe_username_c);
    return safe_username;
}



// Function for getting player username that sent a custom packet.
std::string guid2un(RakNet_RakNetGUID *guid){
    std::string retStr = "";
    unsigned char *server_side_network_handler = *(unsigned char **) (get_minecraft() + Minecraft_network_handler_property_offset);    
    unsigned char *player = (*ServerSideNetworkHandler_getPlayer)(server_side_network_handler, guid);
    if (player == NULL) {
        INFO("SRY error, no player!"); // Maybe suddenly already disconnected!?
    }else {
        retStr = get_safe_username(player);
    }
    return retStr;
}


// Returns the player obj from a given b64 username.
uchar *b64un2player(std::string unB64){
    for (uchar *player : get_all_players()) {
        if ( b64.enc( get_safe_username(player) ) == unB64) return player;
    }
    return NULL;
}


// Get b64 list of player usernames, except self's username
std::vector<std::string> get_b64playerNames(){
    vector<string> ret;
    for (uchar *player : get_all_players()) {	
        std::string playerName = get_safe_username(player); //*player_username;
        if(playerName != *default_username) ret.push_back( b64.enc(playerName) );
    }
    return ret;
}


// Function to make sure all needed dirs and files for the mod are there.
static void sknDirCheck(){
    std::string homePath(home_get()); // Cause home can be somewhere other than ./minecraft-pi/ 
    
    skinsPath = homePath; // A dir where user puts his PNG skin files.
    skinsPath.append("/SKINS_DIR/");
    // Make sure the dir exists
    if(!existsDir(skinsPath) ) { 
        createDir(skinsPath); 
        // Add two example skin files there..
        std::string stevePiSkin = "iVBORw0KGgoAAAANSUhEUgAAAEAAAAAgCAYAAACinX6EAAAABGdBTUEAALGPC/xhBQAAABh0RVh0U29mdHdhcmUAUGFpbnQuTkVUIHYzLjM2qefiJQAABONJREFUaEPll99rVEcUx6+IUk1ijbQYosYfG+OaRtb4g2JEY42/EYuRipqioqIxMQiLVlDE+INqKlSfAkq0UChIUXzwFyL2sU956X9TmofjfM/u9/ZknLs30cUN8cJh5s6cuTvfz5w5MzslSnmaF9QIXP4bGYmmT5um3qjjGfl3arT/28aSX7j9+p8pab8xofsBoLHuC8kt/FKN72iDnd+ek55NK+XnfWviEnW0wya0uLFMDoIpvql+ZiKATCYj1gBlUgDg6nPlURIKSkbApAVgxSICYHY7fBYAAMEXz3yQBKAz2zQ5tgDFQ/Dy+dUKYuHsqjghEgD2vG+TIgcAAIUzCpgMmQOY8UPlWBJtRX2y82YKDKKWzatSsasys2XFooLlFtXINw2FkyA7v0qPPviiDQDohzEYi2+gn99l0sQ4W8c7Iqmi4vHjFgAm3tIwS4VA4OrGWi1peN/cPEfam79WPwChD8agjVFiAZS6R1QcgE66rrBqBeFOmFvFxjq+18jLgTPy9te8/D3YL29v5139nDz+6ZCsWTpHx8AXYzA23i7um/h22j2i4gB0pepdKC+YVVhV944wRj23pFb+vHxSnt/o0/JJf58M5X+UNwN5GTq9Ww0+8MUYjEUd38I38Z52j5gYAOI8UC2ZuTPk5oE2GTq+VdqyX6l4PJ0bBuTFpVMqHnU8gAIf+GIMxjbVV8f7HwDS7hETB4BbMWyFloZaudHZKoNHvpNbh9Y5cdvk2aVj8upaPj7m/rp1Rn7v2S797s4PH/hiDMbiG1x9Aih1j6g4AIY9wvZYR04e9Hwv949v0T83ONbuHu5wAjfJhc1rR9mdwxvl6r516gNfjMFYfEO3QHE7pN0jKg4Ak6cAlEOnduiK/nJwvZzbkZOzW1ukr6NZxcIu7lkt139Yr4Y6fOD7W/dOHXv/xK5CWYRIkPYOgTrBVRyAP4H9O4fFWjabFWupEx4ejv8K+xejyPVFR4+WtrQfePRI9DswV8ftc2/r0rhMG57aXw4AoWsx2soCgOKL5ecNwEWAXX2F/LFPOSIg6b9BWSLA2wIfDWBlNi+wpsVdWm5r+2OU2T6/v611QKIrV/63ri6J7t2T6OFDTZg4Kpk80aZm/VFHG0TRnj6VCMb3JH/6+P0czzItIijwQwAAlgrq7i4Y6px4EQIAqEhOyPpjDPz9SVsA/Da/zz6UBGp//0MBMBLGEwEKIDRBThITRERYkdYf9ZB4GwUhwHaM3//JASDsKQp1OwECsG1J/gBFcLbuC7TR9KkiwOYB7HtrKh6iaJg8BUAQ62y3vgSGCAkBcONxKaPddJcv/P+whjbr4/enpYAolOQoECHu54j3AFhBgGEBhIRZYNwCoTHFbRMCMHi2K4aAfguhLABsHih1CsQ5gBAIgCFv9z9hEAC3jQ175goDxIpjBBAAyhAAbS9CGlMEMAGO9xgMngJ++FMUAfinQAhWAgCItSvsAwhtkbIAKAXovWPQJjz/BMB7Wtb2sjgjgGUIQKkckQqgvb1dYKWOQfTRr7e3V6zFgngh4er54c+E6AOw/vQxCdGK8yMAMGyOYL/dImMGkCSQ7Un9NkfgMuXfI/CuN0YDAH724sXEqveFIhC0YWwSAIoMRUjFAIRyCIEoBCcu9WrtfCxEf4VHJUCX6ELH4HgAvAMPTt9mhQSK5wAAAABJRU5ErkJggg==";
        std::string spidermanSkin = "iVBORw0KGgoAAAANSUhEUgAAAEAAAAAgCAYAAACinX6EAAAEjElEQVR4nN1ZP2sjORT/vdjFFYYhsEgBp4q/gQemMGxzfXzF9VdOyjNcHRd79YLb8XfY4ux+m8AUBvsbJFUGPGYhGFyk8WqLkWSNRvNnQ7J27gdhJnpPmvee3j/JhBpMOBcAEAiRG18QAQBm2y3OOx18bLVy9Lv9Hk+7Hb4+P1PdN46JdhOmQAhwosKYMsLTboe7TidHf9rtXknEt0WtAUzlqdsFAIgkASdCIARmku+9KGzjrCmjUt5+f+9obID/KxobQCSJ8/29o9YACyKksgKIJNHKp0YSVDjvdHBuJcNTR6MqsJAJzx5TKJRBz9Nl8NRBf374ID62Wk4F7/Z7AICiq2qgdt+m18130b98+3bUPqFtK6cQCAHIXVV0lf15kjjpdfNd9C9volZzUMyYsOs8kMV7KgQGmw0AIGYs5wH2uD1X8QFw9hHGOkf1AJ0Eq2p7FEUYbDaYA5gDGGw2iKKolN+11qn2EY3KYBiGiKIIgRAIhEAURQjD8K1l+yXQBiir7TFjICLc3NyAE4HLdyJCzJhzTtlaZhk9FbRVieNEOeHMOm/H+b1FD4QAtxSroqv8YvcRx0D7br8HasqYonOrCbLpdfNL6UeEYwsiAaj4nuL33/7OUevO9+r+oAyjdG38N5VPM59Uu8WEc6GO4rPtFkPPwzWy5HwNoJemP+VWb3oYmm23mG23r7pmIAQuLi9zHqVC8yXVxWGA08/u68dH/T7sdjFLEv38WTQ6C7wUQ8979TXNc8nQ83Tifml1aQOiGLO+fC6Br89/2UTJf8gTeYwBlN8h2vwTPgYwNq7YDjFeBUXvpX8AAEZ6vf/0HeaCCKOanFDuAcuqadVhEshW+aHfBwBcrVaIGStVapR+ytY0DA9MMeHjgiEB6Ou4weZfS1gfo9RHzG41Tx2o1AOkEPkPKKJpANsDgJjd4uLyElerFQDgod/H+vHRENhGiJhx66yRApjino8LZwgA6KVrh3y+Xu+eX0i+ag840/N8Y7R09/3Dwy/jCbEg0soDmQdku+/ynGznB5sU1O2Cut1MeT+jzXHoIOdyRi9dV3w/k62XK7flyAxQUHgq/2zLSlSFh5+5tNkmx4xlbl4hdMy4VjRmXI8HQuhDWLVLV1mkHG33XFeCa2iE5RSZF4y1EfTuL4vhotYa+KlzfTOWc3cK8jvFEAwNWj3aTmV8e6zEEwDc83FhrJdmHZ+Kw5GMVxevqhokurhaZd956PsQ/BDv9mVKBlNBW67mvQxVta6uez3Xz2AK9k6ZySs13NdVDVxXbkDW3trrmPPt84QpW9DgwqVtf0AJLpIEaLX0Lz/mB64Nnl76qbDoRO70tVxvDgBEsty5MTG9w+AN2C0ucFBehQRXt9XWZpg0t+dYBlCKv9otjR9itJSxKZXK1flCHsjG9RxjDEuA08EwpmJlCto8daCYsVIudeIyMfS8XDZW9VoLDjhyiD3m4AeKqWaJXEVQMqkuLxACn79/z9H/OTvL8dR2glUtp+u8ru4P8rCSjm6ijGqyDEv4TZo0jPG/ks+8YldjLtmraC78AGbofTSndyx3AAAAAElFTkSuQmCC";
        file_put_contents( skinsPath + "Default-StevePi.png", b64.dec(stevePiSkin) );
        file_put_contents( skinsPath + "Spiderman.png", b64.dec(spidermanSkin) );
    }
    
    // Skin trickery works by actually setting a real file in overrides/images/skins/ and linking it in the game.
    overridesPath = homePath;
    overridesPath.append("/overrides/images/skins/");
    // Make sure the dir exists
    if(!existsDir(overridesPath) ) { 
        createDir(overridesPath); 
    } else {
        // Clear out old skin files there!
        //vector<string> removeables; // Vector we'll populate with filenames to be deleted!
        vector<string> removeables = dirList(overridesPath, ".png");
        if( removeables.size() > 0 ){
            for(std::string item : removeables) {
                std::string removeFile = overridesPath + item;
                std::remove(removeFile.c_str() ); // delete file 
            }
            INFO("[SKINSWAPPER]: OLD SKIN OVERRIDES DELETED!");    
        }
    }
}







//The "main" function that starts when you run game
__attribute__((constructor)) static void init() {
    sknDirCheck();    // Check / create the necessary dir's for the mod!
    skinswap(true);   // Load skins b64 from files if there are any.
}