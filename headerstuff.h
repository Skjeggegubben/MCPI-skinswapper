#ifdef __cplusplus
	#include <unordered_map>
	#include <thread> //For the timers/bgTs

	typedef unsigned char uchar;
	//Stuff for the manipulating of inventory data
	typedef unsigned char *(*Inventory_t)(unsigned char *inventory, unsigned char *player, uint32_t is_creative);
	static Inventory_t Inventory = (Inventory_t) 0x8e768;

	uint64_t get_ms_unixTS();
	uchar *get_minecraft();
	uchar *b64un2player(std::string unB64);

	void parse_skin_packet(std::string packetStr, RakNet_RakNetGUID *guid, uchar *callback);
	void skinswap(bool increment);
	void showInfo(std::string text);
	void file_put_contents(std::string thePath, std::string saveStr); // mimics the php function :)

	bool all_allowed_chars(const std::string& str);
	bool isValidBase64(const std::string& input);
	bool isSkinString(std::string skinB64);
	bool strEndsWith(const std::string& text, const std::string& suffix);
	bool in_vec(std::vector<std::string> inputVec, std::string strValue);

	std::string shuffleString(const std::string& str);
	std::string lower_cased(std::string inputStr);
	std::string guid2un(RakNet_RakNetGUID *guid);
	std::string get_safe_username(unsigned char *player);
	std::string shortened(std::string inputStr, int max);
	std::string get_randStr(int length);
	std::string file_get_contents(std::string fileName); // mimics the php function :)

	std::vector<std::string> split_str(std::string str, char theChar);
	std::vector<std::string> dirList(std::string thePath, std::string fileExt);
	std::vector<std::string> get_b64playerNames();
	std::vector<uchar *> get_all_players();

	static void sknDirCheck();
	static void callback_for_mcpiskinswaps(unsigned char *mcpi);
	static void sknTickFunction(unsigned char *mcpi);
	static void update_dicts_and_files();
	static void createDir(const std::string& thePath);
	static void newSkin(std::string playerNameB64, std::string skinB64);

	static bool existsDir(const std::string& thePath);
	static bool self_is_server();
	static bool server_is_player();
	static bool got_raknetInstance();
	static bool on_ext_server();
	static bool isNumeric(const std::string& str);
	static bool isPowerOf2(int n);


	extern std::string skinsPath; //= "";
	extern std::string overridesPath; //= "";

	extern std::unordered_map<std::string, std::string> skinsDict; 		// The server's umap of received inventories as b64
	extern std::unordered_map<std::string, std::string> skinsDict_old; 	// Duplicate umap of received inventories as b64, for comparison

	extern std::unordered_map<std::string, struct bgT> bgTs;
	class bgT {
	public:
		template<typename Function>
		void setTimeout(Function function, int delay, std::string inputStr1, std::string inputStr2) {
			std::thread t([=]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				function(inputStr1, inputStr2);
				bgTs.erase(inputStr1); // Yes, deletes itself from the umap once done!
			});
			t.detach();
		}
	};




	class b64class {
	  public:
		// encode and decode copies from https://stackoverflow.com/a/34571089/18254316
		std::string dec(std::string inputStr){    
			std::string ret;
			std::vector<int> T(256,-1);
			for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

			int val=0, valb=-8;
			for (char c : inputStr) {
				if (T[c] == -1) break;
				val = (val << 6) + T[c];
				valb += 6;
				if (valb >= 0) {
					ret.push_back(char((val>>valb)&0xFF));
					valb -= 8;
				}
			}
			return ret;
		}

		std::string enc(std::string inputStr){
			std::string ret;
			int val = 0, valb = -6;
			for (char c : inputStr) {
				val = (val << 8) + c;
				valb += 8;
				while (valb >= 0) {
					ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
					valb -= 6;
				}
			}
			if (valb>-6) ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
			while (ret.size()%4) ret.push_back('=');
			return ret;
		}
	};
	/*
	  // EXAMPLE USE:
	  b64class b64; // Make the class object first, then use:
	  INFO( b64.enc("BMW") );
	  INFO( b64.dec("dGVzdA==") );
	*/

	extern "C" {
		char *home_get();
		
		
		//void Gui_addMessage_injection(unsigned char *gui, std::string const &text);
		//std::string get_server_name();
		//bool in_local_world();
		
		//std::vector<unsigned char *> get_players();
	}

#endif