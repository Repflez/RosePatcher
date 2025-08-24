#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <typeinfo>

#include <wups.h>
#include <wups/storage.h>
#include <coreinit/bsp.h>
#include <coreinit/launch.h>
#include <coreinit/mcp.h>
#include <coreinit/memory.h>
#include <coreinit/title.h>
#include <nn/act/client_cpp.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>

#include "token.hpp"
#include "utils.hpp"
#include "logger.h"
#include "Notification.hpp"
#include "obfuscate.hpp"
#include "monocypher.h"
#include "monocypher-ed25519.h"

extern "C" MCPError MCP_GetDeviceId(int handle, char* out);
extern "C" MCPError MCP_GetCompatDeviceId(int handle, char* out);

#define EXPECTED_TOKEN_LENGTH 32
#define TOKEN_LENGTH 32

namespace nn::act {
    static constexpr size_t UrlSize   = 255; // GUESS

    nn::Result GetMiiImageUrlEx(char outUrl[UrlSize], SlotNo slot)
        asm("GetMiiImageUrlEx__Q2_2nn3actFPcUc");
}

namespace token {
    std::string currentToken;
    std::vector<std::string> rverseTokens(12);
    std::vector<std::string> accountNames(12);
    std::string otpHash = "nohash";

    char codeId[8];
    char serialId[12];

#ifndef TOKEN_KEY
    #error TOKEN_KEY is not defined
#else
    _Static_assert((sizeof(TOKEN_KEY) - 1) == EXPECTED_TOKEN_LENGTH, "Invalid TOKEN_KEY length!");
#endif

    void setReplacementToken(std::string token, nn::act::SlotNo slot) {
        rverseTokens[slot - 1] = token;
    }

    std::string bytes_to_hex(const uint8_t* data, size_t len) {
        std::stringstream ss;
        for (size_t i = 0; i < len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        }
        return ss.str();
    }

    std::string stringToHex(const std::string& input) {
        std::ostringstream hexStream;

        for (unsigned char c : input) {
            hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }

        return hexStream.str();
    }

    void getOTPHash() {
        std::string otpPath = "fs:/vol/external01/otp.bin";

        std::ifstream file(otpPath, std::ios::binary);

        if (!file) {
            DEBUG_FUNCTION_LINE("Either the OTP path in %s is dead, or this SD Card may be dying", otpPath.c_str());
            file.close();
            return;
        }

        std::vector<char> wiiUDevicePrivateKey(0x1E);
        std::vector<char> wiiUDeviceUniqueCertificatePrivateKey(0x1E);
        std::vector<char> wiiUDeviceUniqueCertificateSignature(0x3C);
        std::string hash;
        std::string preHash = "";

        file.seekg(0x220, std::ios::beg);
        file.read(wiiUDevicePrivateKey.data(), 0x1E);

        file.seekg(0x240, std::ios::beg);
        file.read(wiiUDeviceUniqueCertificatePrivateKey.data(), 0x1E);

        file.seekg(0x28C, std::ios::beg);
        file.read(wiiUDeviceUniqueCertificateSignature.data(), 0x3C);

        std::string str(wiiUDevicePrivateKey.begin(), wiiUDevicePrivateKey.end());
        preHash.append(stringToHex(str));
        
        str = std::string(wiiUDeviceUniqueCertificatePrivateKey.begin(), wiiUDeviceUniqueCertificatePrivateKey.end());
        preHash.append(stringToHex(str));
        
        str = std::string(wiiUDeviceUniqueCertificateSignature.begin(), wiiUDeviceUniqueCertificateSignature.end());
        preHash.append(stringToHex(str));

        // Hash the OTP now
        size_t key_size = strlen(OBFUSCATE(TOKEN_KEY));
        uint8_t* key = new uint8_t[key_size];
        memcpy(key, OBFUSCATE(TOKEN_KEY), key_size);

        const uint8_t* msg_data = reinterpret_cast<const uint8_t*>(preHash.c_str());
        size_t msg_size = preHash.length();
            
        uint8_t hashedOtp[64];
        crypto_sha512_hmac(hashedOtp, key, key_size, msg_data, msg_size);
            
        otpHash = bytes_to_hex(hashedOtp, 64);
            
        // Clear from memory our stuff
        crypto_wipe(key, key_size);
        crypto_wipe(hashedOtp, 64); // Clear the raw HMAC data from memory
        delete[] key;

        DEBUG_FUNCTION_LINE("%s", otpHash.c_str());
        
        file.close();
    }
    
    void initToken() {
        // Before we do anything, we have to load the OTP
        getOTPHash();

        // enable RNG in case we need it.
        srand(time(NULL));

        int handle = MCP_Open();
        WUT_ALIGNAS(0x40) MCPSysProdSettings settings;
        MCPError error = MCP_GetSysProdSettings(handle, &settings);

        if (error) {
            DEBUG_FUNCTION_LINE("MCP_GetSysProdSettings failed");
            ShowNotification("rverse: Failed to get Wii U Settings. You will not be able to launch rverse. Try again via rebooting.");
            return;
        }
        
        memcpy(codeId, settings.code_id, sizeof(settings.code_id));
        memcpy(serialId, settings.serial_id, sizeof(settings.serial_id));

        std::string tokenStoragePath = "fs:/vol/external01/wiiu/rverse";
        try {
            if (std::filesystem::create_directory(tokenStoragePath)) {
                //DEBUG_FUNCTION_LINE("The directory %s was created and it's ready to use", tokenStoragePath.c_str());
            } else {
                //DEBUG_FUNCTION_LINE("The directory %s either exists already or there was an error creating it", tokenStoragePath.c_str());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            DEBUG_FUNCTION_LINE("Error with directory creation: %s", e.what());
            ShowNotification("rverse: Failed to start the plugin. You will not be able to launch rverse. Try again via rebooting.");
            return;
        }
        
        
        for (size_t i = 1; i < 12; i++) {
            if (!nn::act::IsSlotOccupied(i)) {
                // No more accounts
                //DEBUG_FUNCTION_LINE("Slot %d has no account, stopping loading", i);
                return;
            }

            unsigned int pid = 0;

            // Get the PID for the account in the current slot
            nn::act::GetPrincipalIdEx(&pid, i);

            if (pid == 0) {
                DEBUG_FUNCTION_LINE("PID for account %d is 0; account probably not linked to NNID/PNID", i);
                continue;
            }

            //DEBUG_FUNCTION_LINE("Got PID %d for account %d", pid, i);

            char accountName[255];
            nn::act::GetAccountIdEx(accountName, i);
            accountNames[i - 1] = accountName;

            // Create filename for the token filename
            std::string tokenFilename = std::format("{}:{}{}", pid, codeId, serialId);
            
            size_t key_size = strlen(OBFUSCATE(TOKEN_KEY));
            uint8_t* key = new uint8_t[key_size];
            memcpy(key, OBFUSCATE(TOKEN_KEY), key_size);

            const uint8_t* msg_data = reinterpret_cast<const uint8_t*>(tokenFilename.c_str());
            size_t msg_size = tokenFilename.length();
            
            uint8_t hashedFilename[64];
            crypto_sha512_hmac(hashedFilename, key, key_size, msg_data, msg_size);
            
            std::string finalFilename = bytes_to_hex(hashedFilename, 64);
            
            // Clear from memory our stuff
            crypto_wipe(key, key_size);
            crypto_wipe(hashedFilename, 64); // Clear the raw HMAC data from memory
            delete[] key;

            // Account token path
            std::string filePath = tokenStoragePath + "/" + finalFilename;

            // The token
            std::string newToken = "";
            char token[TOKEN_LENGTH + 1];

            // Create or open the current file
            FILE *fp = fopen(filePath.c_str(), "r");

            if (!fp) {
                DEBUG_FUNCTION_LINE("File for account %d not found, generating token now", i);

                fclose(fp);
                fp = fopen(filePath.c_str(), "w");

                // This is Project Rose's generator. It's awful but it works.
                // It was made with AI, though.
                for (int i = 0; i < TOKEN_LENGTH; i++) {
                    int randNum = rand() % 62;
                    
                    if (randNum < 26) {
                        newToken += 'a' + randNum;
                    } else if (randNum < 52) {
                        newToken += 'A' + (randNum - 26);
                    } else {
                        newToken += '0' + (randNum - 52);
                    }
                }

                fputs(newToken.c_str(), fp);
                strcpy(token, newToken.c_str());

                fclose(fp);
            } else {
                fread(token, TOKEN_LENGTH, 1, fp);
                token[TOKEN_LENGTH] = '\0';
                fclose(fp);
            }

            //DEBUG_FUNCTION_LINE("Replacement token: %s", token);
            setReplacementToken(token, i);

            // Close the file
            fclose(fp);
        }
        MCP_Close(handle);
    }

    void updCurrentToken() {
        size_t slotNo = nn::act::GetSlotNo();

        int16_t miiNameTemp[255];
        nn::act::GetMiiNameEx(miiNameTemp, slotNo);

        char miiImageUrl[255];
        nn::act::GetMiiImageUrlEx(miiImageUrl, slotNo);
        
        unsigned int pid = nn::act::GetPrincipalId();

        std::string isNintendoAccount;
        std::string miiName;

        // Check if the Mii URL is from Nintendo Netork or not.
        std::string url(miiImageUrl);

        if (url.find("https://mii-secure.account.nintendo.net/") != std::string::npos) {
            isNintendoAccount = "1";
        } else {
            isNintendoAccount = "0";
        }

        miiName = std::string(miiNameTemp, miiNameTemp + sizeof(miiNameTemp) / sizeof(miiNameTemp[0]));

        // TODO: Encrypt
        currentToken = std::format("{}\\{}\\{}\\{}\\{}{}\\{}\\{}",
            accountNames[slotNo - 1].c_str(),   // NNID/PNID Name
            miiName.c_str(),                    // Mii Name
            pid,                                // PID
            isNintendoAccount.c_str(),          // Is NNID or not
            codeId,                             // Serial beginning
            serialId,                           // Serial Number
            rverseTokens[slotNo - 1].c_str(),   // Token
            otpHash.c_str()                     // OTP Hash
        );

        DEBUG_FUNCTION_LINE("result: %s", currentToken.c_str());
    }
} // namespace token