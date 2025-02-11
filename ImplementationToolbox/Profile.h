#include <string>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <vector>
/*
    This class will be used to save and load profiles easily.
    By offering a simple way to build objects with all previously saved data read back into the class.
    This should allow us to switch profiles on the fly, as well as save/edit them as needed.
*/
class Profile
{
private:
    // Contains all aspects of a profile needed to generate an export
    char exportName[64], sftpUser[64], sftpPass[64], sftpIp[128], sftpTargetDir[64], agency[5], filePrefix[64], label[120][100], rmsDb[15], delimiter[1], profileName[128];
    bool isPinReq, includeHousing, exportNow, combineHousing, combineStApt, retainHistory, secureSSN, noQuotes, selected[120];
    int pinType, fileType, recordOptn, fileOptn, createOptn, dateFormat, isSftp, writeDelay, order[130], genexptid;
    const int actcodetyp = 2, cashBalFmt = 0, inactive = 0;
    const char activeField = 'A', inActiveField = 'I';
public:
    // Getter functions for static char arrays
    const char* getExportName() { return exportName; }
    const char* getSftpUser() { return sftpUser; }
    const char* getSftpPass() { return sftpPass; }
    const char* getSftpIp() { return sftpIp; }
    const char* getSftpTargetDir() { return sftpTargetDir; }
    const char* getAgency() { return agency; }
    const char* getFilePrefix() { return filePrefix; }
    const char* getLabel(int index) { return label[index]; }
    const char* getRmsDb() { return rmsDb; }
    const char* getDelimiter() { return delimiter; }
    const char* getProfileName() { return profileName; }

    // Getter functions for static bool variables
    bool getIsPinReq() { return isPinReq; }
    bool getIncludeHousing() { return includeHousing; }
    bool getExportNow() { return exportNow; }
    bool getCombineHousing() { return combineHousing; }
    bool getCombineStApt() { return combineStApt; }
    bool getRetainHistory() { return retainHistory; }
    bool getSecureSSN() { return secureSSN; }
    bool getNoQuotes() { return noQuotes; }
    bool getSelected(int index) { return selected[index]; }

    // Getter functions for static int variables
    int getPinType() { return pinType; }
    int getFileType() { return fileType; }
    int getRecordOptn() { return recordOptn; }
    int getFileOptn() { return fileOptn; }
    int getCreateOptn() { return createOptn; }
    int getDateFormat() { return dateFormat; }
    int getIsSftp() { return isSftp; }
    int getWriteDelay() { return writeDelay; }
    int getOrder(int index) { return order[index]; }
    int getGenexptid() { return genexptid; }

    // Getter functions for const static int variables
    int getActcodetyp() { return actcodetyp; }
    int getCashBalFmt() { return cashBalFmt; }
    int getInactive() { return inactive; }

    // Getter functions for const static char variables
    char getActiveField() { return activeField; }
    char getInActiveField() { return inActiveField; }

    // Setter functions for static char arrays
    void setProfileName(const char* value) { strncpy(profileName, value, sizeof(profileName) - 1); profileName[sizeof(profileName) - 1] = '\0'; }
    void setExportName(const char* value) { strncpy(exportName, value, sizeof(exportName) - 1); exportName[sizeof(exportName) - 1] = '\0'; }
    void setSftpUser(const char* value) { strncpy(sftpUser, value, sizeof(sftpUser) - 1); sftpUser[sizeof(sftpUser) - 1] = '\0'; }
    void setSftpPass(const char* value) { strncpy(sftpPass, value, sizeof(sftpPass) - 1); sftpPass[sizeof(sftpPass) - 1] = '\0'; }
    void setSftpIp(const char* value) { strncpy(sftpIp, value, sizeof(sftpIp) - 1); sftpIp[sizeof(sftpIp) - 1] = '\0'; }
    void setSftpTargetDir(const char* value) { strncpy(sftpTargetDir, value, sizeof(sftpTargetDir) - 1); sftpTargetDir[sizeof(sftpTargetDir) - 1] = '\0'; }
    void setAgency(const char* value) { strncpy(agency, value, sizeof(agency) - 1); agency[sizeof(agency) - 1] = '\0'; }
    void setFilePrefix(const char* value) { strncpy(filePrefix, value, sizeof(filePrefix) - 1); filePrefix[sizeof(filePrefix) - 1] = '\0'; }
    void setLabel(int index, const char* value) { strncpy(label[index], value, sizeof(label[index]) - 1); label[index][sizeof(label[index]) - 1] = '\0'; }
    void setRmsDb(const char* value) { strncpy(rmsDb, value, sizeof(rmsDb) - 1); rmsDb[sizeof(rmsDb) - 1] = '\0'; }
    void setDelimiter(const char* value) { strncpy(delimiter, value, sizeof(delimiter) - 1); delimiter[sizeof(delimiter) - 1] = '\0'; }

    // Setter functions for static bool variables
    void setIsPinReq(bool value) { isPinReq = value; }
    void setIncludeHousing(bool value) { includeHousing = value; }
    void setExportNow(bool value) { exportNow = value; }
    void setCombineHousing(bool value) { combineHousing = value; }
    void setCombineStApt(bool value) { combineStApt = value; }
    void setRetainHistory(bool value) { retainHistory = value; }
    void setSecureSSN(bool value) { secureSSN = value; }
    void setNoQuotes(bool value) { noQuotes = value; }
    void setSelected(int index, bool value) { selected[index] = value; }

    // Setter functions for static int variables
    void setPinType(int value) { pinType = value; }
    void setFileType(int value) { fileType = value; }
    void setRecordOptn(int value) { recordOptn = value; }
    void setFileOptn(int value) { fileOptn = value; }
    void setCreateOptn(int value) { createOptn = value; }
    void setDateFormat(int value) { dateFormat = value; }
    void setIsSftp(int value) { isSftp = value; }
    void setWriteDelay(int value) { writeDelay = value; }
    void setOrder(int index, int value) { order[index] = value; }
    void setGenexptid(int value) { genexptid = value; }


    // Constructor used to build a blank profile
    Profile() : profileName(""), exportName(""), sftpUser(""), sftpPass(""), sftpIp(""), sftpTargetDir(""), agency(""), filePrefix(""), label(), rmsDb(""), delimiter(""),
        isPinReq(false), includeHousing(false), exportNow(false), combineHousing(false), combineStApt(false), retainHistory(false), secureSSN(false), noQuotes(false), selected(),
        pinType(), fileType(), recordOptn(), fileOptn(), createOptn(), dateFormat(), isSftp(2), writeDelay(0), order(), genexptid(0),
        activeField('A'), inActiveField('I') {}

    // Constructor to build a profile using the csv data
    Profile(char* pName, static char* exName, char* sfUser, char* sfPass, char* sfIp, char* sfTarget, char* agncy, char* filePref, char* label, char* rmsData, char* delim,
        bool pinReq, bool inclHousing, bool exNow, bool cmbnHouse, bool cmbnApt, bool retain, bool secSSN, bool noQuot, bool select[120],
        int pnType, int fileT, int recOptn, int fileOp, int creatOp, int dateFrmt, int sftp, int wrteDlay, int ordr, int gnexptid, int labelIndex, int selectIndex, int orderIndex) {
        // Init all variables with csv file data
        // Init char strings
        setProfileName(pName);
        setExportName(exName);
        setSftpUser(sfUser);
        setSftpPass(sfPass);
        setSftpIp(sfIp);
        setSftpTargetDir(sfTarget);
        setAgency(agncy);
        setFilePrefix(filePref);
        setLabel(labelIndex, label);
        setRmsDb(rmsData);
        setDelimiter(delim);
        // Init bools
        setIsPinReq(pinReq);
        setIncludeHousing(inclHousing);
        setExportNow(exNow);
        setCombineHousing(cmbnHouse);
        setCombineStApt(cmbnApt);
        setRetainHistory(retain);
        setSecureSSN(secSSN);
        setNoQuotes(noQuot);
        setSelected(selectIndex, select);
        // Init ints
        setPinType(pnType);
        setFileType(fileT);
        setFileOptn(fileOp);
        setCreateOptn(creatOp);
        setDateFormat(dateFrmt);
        setIsSftp(sftp);
        setWriteDelay(wrteDlay);
        setOrder(orderIndex, ordr);
        setGenexptid(gnexptid);
    }

    // Read in profile data that is saved in the profiles.txt file
    bool importProfiles(Profile* p, std::string profile_name) {
        std::ifstream importProfile;

        importProfile.open("C:\\ImplementationToolbox\\" + profile_name);

        if (!importProfile)
            return false;
        else {
            std::string line;
            if (std::getline(importProfile, line))
                p->setExportName(line.c_str());
            if (std::getline(importProfile, line))
                p->setSftpUser(line.c_str());
            if (std::getline(importProfile, line))
                p->setSftpPass(line.c_str());
            if (std::getline(importProfile, line))
                p->setSftpIp(line.c_str());
            if (std::getline(importProfile, line))
                p->setSftpTargetDir(line.c_str());
            if (std::getline(importProfile, line))
                p->setAgency(line.c_str());
            if (std::getline(importProfile, line))
                p->setFilePrefix(line.c_str());
            // Input integers into (bool)selected array
            if (std::getline(importProfile, line)) {
                std::stringstream ss(line);
                std::string token;
                int i = 0;
                // Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
                while (std::getline(ss, token, ',')) {
                    p->setSelected(std::stoi(token), true);
                    p->setOrder(i, std::stoi(token));
                    i++;
                }
            }

            if (std::getline(importProfile, line))
                p->setRmsDb(line.c_str());
            if (std::getline(importProfile, line))
                p->setDelimiter(line.c_str());
            if (std::getline(importProfile, line))
                p->setProfileName(line.c_str());
            if (std::getline(importProfile, line))
                p->setIsPinReq(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setIncludeHousing(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setExportNow(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setCombineHousing(std::stoi(line));
            if (std::getline(importProfile, line))
                p->setCombineStApt(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setRetainHistory(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setSecureSSN(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setNoQuotes(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setPinType(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setFileType(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setRecordOptn(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setFileOptn(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setCreateOptn(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setDateFormat(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setIsSftp(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setWriteDelay(std::stoi(line.c_str()));
            if (std::getline(importProfile, line))
                p->setGenexptid(std::stoi(line.c_str()));

            return true;
        }

        return false;
    }

    // Create profiles.txt if it doesn't exist, then write all profile data to it
    // Append the profile details if it already exists
    void exportProfiles(Profile* p) {
        // static int orderSize = IM_ARRAYSIZE(order);
        //std::vector<int> selectedLabels;
        //for (int i = 0; i < sizeof(order); i++)
        //{
        //    // Populate vector with order that fields were selected in
        //    if(order[i+1] != 0)
        //        selectedLabels.push_back(getOrder(i));
        //}

        // Open profile.txt
        std::ofstream exportProfile;

        try {
            // Build file path using profile names which will be changeable in the future. Also will allow custom file paths in the future
            const char *profile_name = { getProfileName() };
            std::string fileName = profile_name;
            std::string save_path = "C:\\ImplementationToolbox\\";       // Hard coded...for now
            std::string full_filepath = save_path + fileName + "_profiles.txt";
            exportProfile.open(full_filepath);
        }
        catch(std::exception const& e){
            std::cout << e.what();
        }

        if (!exportProfile) {
            return;
        }
        else
        {
            exportProfile << getExportName() << std::endl;
            exportProfile << getSftpUser() << std::endl;
            exportProfile << getSftpPass() << std::endl;
            exportProfile << getSftpIp() << std::endl;
            exportProfile << getSftpTargetDir() << std::endl;
            exportProfile << getAgency() << std::endl;
            exportProfile << getFilePrefix() << std::endl;
            // Used order index to get list of fields in order
            for (int i = 0; i < sizeof(order); i++)
            {
                // Don't add comma on end of list
                if (getOrder(i + 1) != 0) {
                        exportProfile << getOrder(i) << ",";
                }
                else if (getOrder(i + 1) == 0) {
                    exportProfile << getOrder(i);
                    break;
                }
            }
            exportProfile << std::endl;
            exportProfile << getRmsDb() << std::endl;
            exportProfile << getDelimiter() << std::endl;
            exportProfile << getProfileName() << std::endl;
            exportProfile << getIsPinReq() << std::endl;
            exportProfile << getIncludeHousing() << std::endl;
            exportProfile << getExportNow() << std::endl;
            exportProfile << getCombineHousing() << std::endl;
            exportProfile << getCombineStApt() << std::endl;
            exportProfile << getRetainHistory() << std::endl;
            exportProfile << getSecureSSN() << std::endl;
            exportProfile << getNoQuotes() << std::endl;
            exportProfile << getPinType() << std::endl;
            exportProfile << getFileType() << std::endl;
            exportProfile << getRecordOptn() << std::endl;
            exportProfile << getFileOptn() << std::endl;
            exportProfile << getCreateOptn() << std::endl;
            exportProfile << getDateFormat() << std::endl;
            exportProfile << getIsSftp() << std::endl;
            exportProfile << getWriteDelay() << std::endl;
            // Not actually needed. We always set the order based on the order the fields are selected in
            // even when it's customizable they will be in the correct order top to bottom
            //// Print out matching list of field orders for selected labels
            //for (int i = 0; i < selectedLabels.size(); i++)
            //{
            //    // Don't add a comma at the end of the list
            //    if (i + 1 == selectedLabels.size())
            //        exportProfile << getOrder(selectedLabels[i]);
            //    else
            //        exportProfile << getOrder(selectedLabels[i]) << ",";
            //}
            //exportProfile << std::endl;
            exportProfile << getGenexptid() << std::endl;
            exportProfile << getActcodetyp() << std::endl;
            exportProfile << getCashBalFmt() << std::endl;
            exportProfile << getInactive() << std::endl;
            exportProfile << getActiveField() << std::endl;
            exportProfile << getInActiveField() << std::endl;
        }

        // Close the file when we're done writing to it
        exportProfile.close();
    }
    
};

