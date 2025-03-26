#pragma once
#include <string>

constexpr int CODE_KEY_LEN = 5;

// Only need code_agcy, description, and code_key for beats
// code_key is always LWBT on initial import
// May need to separate out geotab1 importing to be it's own module for map importing.
// 
class Beat
{
private:
	char
		code_fbi[CODE_KEY_LEN],
		code_sbi[CODE_KEY_LEN],
		code_agcy[CODE_KEY_LEN],
		code_key[CODE_KEY_LEN],
		descriptn[55],
		sys_msg[40],
		sys_use[30],
		notes[1000];
	int
		internal;
public:
	// Default constructor
	Beat() = default;

	// Constructor
	Beat(char* code_fbi, char* code_sbi, char* code_agcy, char* code_key, char* descriptn, char* sys_msg, char* sys_use, char* notes, int internal)
	{
		strncpy_s(this->code_fbi, code_fbi, CODE_KEY_LEN);
		strncpy_s(this->code_sbi, code_sbi, CODE_KEY_LEN);
		strncpy_s(this->code_agcy, code_agcy, CODE_KEY_LEN);
		strncpy_s(this->code_key, code_key, CODE_KEY_LEN);
		strncpy_s(this->descriptn, descriptn, sizeof(descriptn));
		strncpy_s(this->sys_msg, sys_msg, sizeof(sys_msg));
		strncpy_s(this->sys_use, sys_use, sizeof(sys_use));
		strncpy_s(this->notes, notes, sizeof(notes));
		internal = this->internal;
	}

	// Getters
	const char* getCodeFbi() { return code_fbi; }
	const char* getCodeSbi() { return code_sbi; }
	const char* getCodeAgcy() { return code_agcy; }
	const char* getCodeKey() { return code_key; }
	const char* getDescriptn() { return descriptn; }
	const char* getSysMsg() { return sys_msg; }
	const char* getSysUse() { return sys_use; }
	const char* getNotes() { return notes; }
	int getInternal() { return internal; }

	// Setters
	void setCodeFbi(const char* value) { strncpy(code_fbi, value, CODE_KEY_LEN); }
	void setCodeSbi(const char* value) { strncpy(code_sbi, value, CODE_KEY_LEN); }
	void setCodeAgcy(const char* value) { strncpy(code_agcy, value, CODE_KEY_LEN); }
	void setCodeKey(const char* value) { strncpy(code_key, value, CODE_KEY_LEN); }
	void setDescriptn(const char* value) { strncpy(descriptn, value, sizeof(descriptn)); }
	void setSysMsg(const char* value) { strncpy(sys_msg, value, sizeof(sys_msg)); }
	void setSysUse(const char* value) { strncpy(sys_use, value, sizeof(sys_use)); }
	void setNotes(const char* value) { strncpy(notes, value, sizeof(notes)); }
	void setInternal(int value) { internal = value; }
};

