// (C) 2013 CPPGM Foundation www.cppgm.org.  All rights reserved.

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <memory>
#include <cstring>
#include <cstdint>
#include <climits>
#include <map>


using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The following are PA1 code
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////



// Translation features you need to implement:
// - utf8 decoder	
int Utf82Unicode(vector<unsigned char> utf8){
	int result=-1;
	switch(utf8.size()){
		case 1:result = utf8[0];break;
		case 2:result = ((utf8[0] & 0x1f)<<6) + (utf8[1] & 0x3f);break;
		case 3:result = ((utf8[0] & 0x0f)<<12) + ((utf8[1] & 0x3f)<<6) + (utf8[2] & 0x3f);break;
		case 4:result = ((utf8[0] & 0x07)<<18) + ((utf8[1] & 0x3f)<<12) + ((utf8[2] & 0x3f)<<6) + (utf8[3] & 0x3f);break;
		default:throw logic_error("utf8-->unicode failed!");
	}
	return result;
}
// - utf8 encoder
string Unicode2Utf8(int u){
	string result;
	if(u<=0x7f && u>=0){
		result += char(u);
	}
	else if(u<=0x7ff && u>=0x80){
		result += char(0xC0 + ((u & 0x07c0)>>6));
		result += char(0x80 + (u & 0x3f));
	}
	else if(u<=0xffff && u>=0x800){
		result += char(0xe0 + ((u & 0xf000)>>12));
		result += char(0x80 + ((u & 0xfc0)>>6));
		result += char(0x80 + (u & 0x3f));
	}
	else if(u<=0x10ffff && u>=0x10000){
		result += char(0xf0 + ((u & 0x1c0000)>>18));
		result += char(0x80 + ((u & 0x3f000)>>12));
		result += char(0x80 + ((u & 0xfc0)>>6));
		result += char(0x80 + (u & 0x3f));
	}
	else{
		throw logic_error("utf8 segmentation error!\n");
	}
	return result;

}
string Unicode2Utf16(int u){
	string res = "  ";
	if(u < 0x10000){
		res[1] = char((u & 0xff00)>>8);
		res[0] = char(u & 0x00ff);
	}else if(u <= 0x10ffff){
		res = "    ";
		int uu = u - 0x10000;
		res[0] = char((uu & 0x3fc00)>>10);
		res[1] = char(0xd8 + ((uu & 0xc0000)>>18));
		res[2] = char(uu & 0xff);
		res[3] = char(0xdc + ((uu & 0x300)>>8));
	}else throw logic_error("unicode ---> utf16 error!\n");
	return res;
}
vector<int> strUtf82Unicode(string u){
	vector<int> result;
	for(uint i=0;i<u.length();i++){
		if((u[i] & 0xe0) == 0xc0){
			vector<unsigned char> utf8;
			utf8.push_back(u[i]);
			utf8.push_back(u[i+1]);
			i=i+1;
			result.push_back(Utf82Unicode(utf8));
		}
		else if((u[i] & 0xf0) == 0xe0){
			vector<unsigned char> utf8;
			utf8.push_back(u[i]);
			utf8.push_back(u[i+1]);
			utf8.push_back(u[i+2]);
			i=i+2;
			result.push_back(Utf82Unicode(utf8));
		}
		else if((u[i] & 0xf8) == 0xf0){
			vector<unsigned char> utf8;
			utf8.push_back(u[i]);
			utf8.push_back(u[i+1]);
			utf8.push_back(u[i+2]);
			utf8.push_back(u[i+3]);
			i=i+3;
			result.push_back(Utf82Unicode(utf8));
		}else{
			result.push_back(u[i]);
		}
	}
	return result;

}
string strUnicode2Utf16(vector<int> uni){
	string result;
	for(uint i=0;i<uni.size();i++){
		result+=Unicode2Utf16(uni[i]);
	}
	return result;
	
}
string strUnicode2Utf32(vector<int> uni){
	string result;
	for(uint i=0;i<uni.size();i++){
		result += char(uni[i] & 0xff);
		result += char((uni[i] & 0xff00)>>8);
		result += char((uni[i] & 0xff0000)>>16);
		result += char((uni[i] & 0xff000000)>>24);
	}
	return result;
}
// - universal-character-name decoder
// - trigraphs
// - line splicing
// - newline at eof
// - comment striping (can be part of whitespace-sequence)

// EndOfFile: synthetic "character" to represent the end of source file
constexpr int EndOfFile = -1;
constexpr int PartialComment = 0x00110000;

// given hex digit character c, return its value
int HexCharToValue(int c)
{
	switch (c)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A': return 10;
	case 'a': return 10;
	case 'B': return 11;
	case 'b': return 11;
	case 'C': return 12;
	case 'c': return 12;
	case 'D': return 13;
	case 'd': return 13;
	case 'E': return 14;
	case 'e': return 14;
	case 'F': return 15;
	case 'f': return 15;
	default: throw logic_error("HexCharToValue of nonhex char");
	}
}

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> AnnexE1_Allowed_RangesSorted =
{
	{0xA8,0xA8},
	{0xAA,0xAA},
	{0xAD,0xAD},
	{0xAF,0xAF},
	{0xB2,0xB5},
	{0xB7,0xBA},
	{0xBC,0xBE},
	{0xC0,0xD6},
	{0xD8,0xF6},
	{0xF8,0xFF},
	{0x100,0x167F},
	{0x1681,0x180D},
	{0x180F,0x1FFF},
	{0x200B,0x200D},
	{0x202A,0x202E},
	{0x203F,0x2040},
	{0x2054,0x2054},
	{0x2060,0x206F},
	{0x2070,0x218F},
	{0x2460,0x24FF},
	{0x2776,0x2793},
	{0x2C00,0x2DFF},
	{0x2E80,0x2FFF},
	{0x3004,0x3007},
	{0x3021,0x302F},
	{0x3031,0x303F},
	{0x3040,0xD7FF},
	{0xF900,0xFD3D},
	{0xFD40,0xFDCF},
	{0xFDF0,0xFE44},
	{0xFE47,0xFFFD},
	{0x10000,0x1FFFD},
	{0x20000,0x2FFFD},
	{0x30000,0x3FFFD},
	{0x40000,0x4FFFD},
	{0x50000,0x5FFFD},
	{0x60000,0x6FFFD},
	{0x70000,0x7FFFD},
	{0x80000,0x8FFFD},
	{0x90000,0x9FFFD},
	{0xA0000,0xAFFFD},
	{0xB0000,0xBFFFD},
	{0xC0000,0xCFFFD},
	{0xD0000,0xDFFFD},
	{0xE0000,0xEFFFD}
};

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const vector<pair<int, int>> AnnexE2_DisallowedInitially_RangesSorted =
{
	{0x300,0x36F},
	{0x1DC0,0x1DFF},
	{0x20D0,0x20FF},
	{0xFE20,0xFE2F}
};

bool Is_Initial_Banned(int c){
	for(uint i = 0 ; i < AnnexE2_DisallowedInitially_RangesSorted.size() ; i++){
		if(c >= AnnexE2_DisallowedInitially_RangesSorted[i].first && c <= AnnexE2_DisallowedInitially_RangesSorted[i].second){
			return true;
		}
	}
	return false;
}

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> Digraph_IdentifierLike_Operators =
{
	"new", "delete", "and", "and_eq", "bitand",
	"bitor", "compl", "not", "not_eq", "or",
	"or_eq", "xor", "xor_eq"
};

// See 'simple-escape-sequence' grammar
const unordered_set<int> SimpleEscapeSequence_CodePoints =
{
	'\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

const unordered_set<int> Octal_Digit =
{
    '0','1','2','3','4','5','6','7'
};

const unordered_set<int> Hexadecimal_Digit =
{
	'0','1','2','3','4','5','6','7','8','9','a',
	'b','c','d','e','f','A','B','C','D','E','F'
};

const unordered_set<int> Nondigit=
{
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
    	'n','o','p','q','r','s','t','u','v','w','x','y','z',
    	'A','B','C','D','E','F','G','H','I','J','K','L','M',
    	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z','_'
};
 
const unordered_set<int> Digit=
{
	'0','1','2','3','4','5','6','7','8','9'
};

bool Is_identifier_nondigit(int c){
	for(uint i = 0 ; i < AnnexE1_Allowed_RangesSorted.size() ; i++){
		if(c >= AnnexE1_Allowed_RangesSorted[i].first && c <= AnnexE1_Allowed_RangesSorted[i].second){
			return true;
		}
	}
	if(Nondigit.find(c) != Nondigit.end())
		return true;
	return false;	
}

bool Is_dchar(int c){
	if(c != ' ' && c != '(' && c != ')' && c != '\\' && c != '\t' && c != '\v' && c != '\f' && c != '\n'){
		return true;
	}
	return false;
}

bool Is_cchar(int c){
	if(c != '\'' && c != '\\' && c !='\n') return true;
	return false;
}

bool Is_schar(int c){
	if(c != '\"' && c != '\\' && c !='\n') return true;
	return false;
}

bool Is_hchar(int c){
	if(c != '>' && c !='\n') return true;
	return false;
}

bool Is_qchar(int c){
	if(c != '\"' && c !='\n') return true;
	return false;
}
bool Is_RawStringPointer(int c);
string GetRawString(int c);

class PPTranslator{
public:
	const static int RawStringBase = 0x00120000;
	static vector<string> RawStrings;
	static string RawString,delimiter;
	
	PPTranslator(){}

	static vector<int> Translate(string s){
		vector<int> results;
		int state=0;
		int result=0;
		bool exitsuccess=true;//used by special state 23
		for(uint i=0;i<s.length();i++){
			switch(state){
				case 0:if(s[i]=='\\')state=1;
					else if(s[i]=='?')state=10;
					else if((s[i] & 0xe0) == 0xc0){
						vector<unsigned char> utf8;
						utf8.push_back(s[i]);
						utf8.push_back(s[i+1]);
						i=i+1;
						results.push_back(Utf82Unicode(utf8));
					}
					else if((s[i] & 0xf0) == 0xe0){
						vector<unsigned char> utf8;
						utf8.push_back(s[i]);
						utf8.push_back(s[i+1]);
						utf8.push_back(s[i+2]);
						i=i+2;
						results.push_back(Utf82Unicode(utf8));
					}
					else if((s[i] & 0xf8) == 0xf0){
						vector<unsigned char> utf8;
						utf8.push_back(s[i]);
						utf8.push_back(s[i+1]);
						utf8.push_back(s[i+2]);
						utf8.push_back(s[i+3]);
						i=i+3;
						results.push_back(Utf82Unicode(utf8));
					}
					else if(s[i]==-1 && i == s.length()-1){}
					else if((s[i] & 0xf8) == 0xf8) throw logic_error("utf8 invalid unit (111111xx)\n");
					else if(s[i]=='/')state=16;
					else if(s[i]=='R'){state=20;RawString+='R';}					
					else results.push_back(s[i]);
					break;
				case 1:if(s[i]=='u')state=2;
					else if(s[i]=='U')state=6;
					else {i=i-1;state=0;results.push_back('\\');}
					break;
				case 2:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 3; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-2; results.push_back('\\');}
					break;
				case 3:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 4; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-3; results.push_back('\\');}
					break;
				case 4:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 5; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-4; results.push_back('\\');}
					break;
				case 5:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 0; result = (result << 4) + HexCharToValue(s[i]);
						results.push_back(result); result = 0;
					}
					else {state=0; i=i-5; results.push_back('\\');}
					break;

				case 6:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 7; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-2; results.push_back('\\');}
					break;
				case 7:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 8; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-3; results.push_back('\\');}
					break;
				case 8:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 9; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-4; results.push_back('\\');}
					break;
				case 9:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 12; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-5; results.push_back('\\');}
					break;

				case 10:if( s[i]=='?' )state=11;
					else{ state=0; i=i-1; results.push_back('?');}
					break;
				case 11:if( s[i]=='=' ){state=0; s[i]='#'; i = i - 1;} 
					else if( s[i]=='/' ){state=0; s[i]='\\'; i = i - 1;} 
					else if( s[i]=='\'' ){state=0; s[i]='^'; i = i - 1;}
					else if( s[i]=='(' ){state=0; s[i]='[';  i = i - 1;}
					else if( s[i]==')' ){state=0; s[i]=']';  i = i - 1;}
					else if( s[i]=='!' ){state=0; s[i]='|';  i = i - 1;}
					else if( s[i]=='<' ){state=0; s[i]='{';  i = i - 1;}
					else if( s[i]=='>' ){state=0; s[i]='}';  i = i - 1;}
					else if( s[i]=='-' ){state=0; s[i]='~';  i = i - 1;}
					else{ state=0; i=i-2; results.push_back('?');}break;

				case 12:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 13; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-6; results.push_back('\\');}
					break;
				case 13:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 14; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-7; results.push_back('\\'); }
					break;
				case 14:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 15; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-8; results.push_back('\\');}
					break;
				case 15:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 0; result = (result << 4) + HexCharToValue(s[i]);
						results.push_back(result); result = 0;
					} 
					else {state=0; i=i-9; results.push_back('\\');}
					break;
				case 16:if(s[i]=='*')state=17;
					else if(s[i]=='/'){state=19;}
					else{state=0; results.push_back('/'); i=i-1;}
					break;
				case 17:if(s[i]=='*')state=18;
					else if(s[i] == EndOfFile){results.push_back(PartialComment);}
					else state=17;
					break;
				case 18:if(s[i]=='/'){state=0;results.push_back(' ');}
					else if(s[i]=='*')state=18;
					else if(s[i] == EndOfFile){results.push_back(PartialComment);}
					else state=17;
					break;
				case 19:if(s[i]==0x0a || s[i]==-1){state=0;results.push_back(' ');results.push_back(0x0a);}
					else{state = 19;}
					
					break;
				case 20:if(s[i]=='\"'){state=21; RawString += '\"';}
					else {state=0; i=i-1; results.push_back('R'); RawString = "";}
					break;
				case 21:if(Is_dchar(s[i])){state=21; RawString += s[i]; delimiter += s[i];}
					else if(s[i] == '('){state=22; RawString += '(';}
					else throw new logic_error("invalid characters in raw string delimiter");
					break;
				case 22:if(s[i] == ')'){state=23; RawString += ')';}
					else{ state = 22; RawString += s[i]; }
					break;
				case 23:
					for(uint j=i,k=0;k<delimiter.length();j++,k++){
						if(s[j] != delimiter[k]){
							state = 22;
							i=j-1;
							exitsuccess=false;
							break;
						}
						else{
							RawString += s[j];
						}
					}
					if(exitsuccess){
						i = i + delimiter.length();
						if(s[i] == '\"'){
							state = 0;
							RawString += s[i];
							delimiter = "";						
							RawStrings.push_back(RawString);
							RawString = "";
							results.push_back(RawStringBase + RawStrings.size() - 1);
						}
						else{
							state = 22;
							i = i-1;
						}
					}
					exitsuccess = true;
					break;
				default:throw new logic_error("little DFA failed? What the hell is it?\n");
			}
		}
		if(!results.empty() && results.back()!=0x0a)results.push_back(0x0a);
		return results;
	}
};

vector<string> PPTranslator::RawStrings;
string PPTranslator::RawString = "";
string PPTranslator::delimiter = "";

bool Is_RawStringPointer(int c){
	return c >= PPTranslator::RawStringBase;
}

string GetRawString(int c){
	return PPTranslator::RawStrings[c - PPTranslator::RawStringBase];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The above are all PA1 code
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The following are all PA2 code
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// See 3.9.1: Fundamental Types
enum EFundamentalType
{
	// 3.9.1.2
	FT_SIGNED_CHAR,
	FT_SHORT_INT,
	FT_INT,
	FT_LONG_INT,
	FT_LONG_LONG_INT,

	// 3.9.1.3
	FT_UNSIGNED_CHAR,
	FT_UNSIGNED_SHORT_INT,
	FT_UNSIGNED_INT,
	FT_UNSIGNED_LONG_INT,
	FT_UNSIGNED_LONG_LONG_INT,

	// 3.9.1.1 / 3.9.1.5
	FT_WCHAR_T,
	FT_CHAR,
	FT_CHAR16_T,
	FT_CHAR32_T,

	// 3.9.1.6
	FT_BOOL,

	// 3.9.1.8
	FT_FLOAT,
	FT_DOUBLE,
	FT_LONG_DOUBLE,

	// 3.9.1.9
	FT_VOID,

	// 3.9.1.10
	FT_NULLPTR_T,

	FT_INVALID,
	FT_DEFAULT
};

// FundamentalTypeOf: convert fundamental type T to EFundamentalType
// for example: `FundamentalTypeOf<long int>()` will return `FT_LONG_INT`
template<typename T> constexpr EFundamentalType FundamentalTypeOf();
template<> constexpr EFundamentalType FundamentalTypeOf<signed char>() { return FT_SIGNED_CHAR; }
template<> constexpr EFundamentalType FundamentalTypeOf<short int>() { return FT_SHORT_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<int>() { return FT_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<long int>() { return FT_LONG_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<long long int>() { return FT_LONG_LONG_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<unsigned char>() { return FT_UNSIGNED_CHAR; }
template<> constexpr EFundamentalType FundamentalTypeOf<unsigned short int>() { return FT_UNSIGNED_SHORT_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<unsigned int>() { return FT_UNSIGNED_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<unsigned long int>() { return FT_UNSIGNED_LONG_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<unsigned long long int>() { return FT_UNSIGNED_LONG_LONG_INT; }
template<> constexpr EFundamentalType FundamentalTypeOf<wchar_t>() { return FT_WCHAR_T; }
template<> constexpr EFundamentalType FundamentalTypeOf<char>() { return FT_CHAR; }
template<> constexpr EFundamentalType FundamentalTypeOf<char16_t>() { return FT_CHAR16_T; }
template<> constexpr EFundamentalType FundamentalTypeOf<char32_t>() { return FT_CHAR32_T; }
template<> constexpr EFundamentalType FundamentalTypeOf<bool>() { return FT_BOOL; }
template<> constexpr EFundamentalType FundamentalTypeOf<float>() { return FT_FLOAT; }
template<> constexpr EFundamentalType FundamentalTypeOf<double>() { return FT_DOUBLE; }
template<> constexpr EFundamentalType FundamentalTypeOf<long double>() { return FT_LONG_DOUBLE; }
template<> constexpr EFundamentalType FundamentalTypeOf<void>() { return FT_VOID; }
template<> constexpr EFundamentalType FundamentalTypeOf<nullptr_t>() { return FT_NULLPTR_T; }

// convert EFundamentalType to a source code
const map<EFundamentalType, string> FundamentalTypeToStringMap
{
	{FT_SIGNED_CHAR, "signed char"},
	{FT_SHORT_INT, "short int"},
	{FT_INT, "int"},
	{FT_LONG_INT, "long int"},
	{FT_LONG_LONG_INT, "long long int"},
	{FT_UNSIGNED_CHAR, "unsigned char"},
	{FT_UNSIGNED_SHORT_INT, "unsigned short int"},
	{FT_UNSIGNED_INT, "unsigned int"},
	{FT_UNSIGNED_LONG_INT, "unsigned long int"},
	{FT_UNSIGNED_LONG_LONG_INT, "unsigned long long int"},
	{FT_WCHAR_T, "wchar_t"},
	{FT_CHAR, "char"},
	{FT_CHAR16_T, "char16_t"},
	{FT_CHAR32_T, "char32_t"},
	{FT_BOOL, "bool"},
	{FT_FLOAT, "float"},
	{FT_DOUBLE, "double"},
	{FT_LONG_DOUBLE, "long double"},
	{FT_VOID, "void"},
	{FT_NULLPTR_T, "nullptr_t"}
};

// token type enum for `simples`
enum ETokenType
{
	// keywords
	KW_ALIGNAS,
	KW_ALIGNOF,
	KW_ASM,
	KW_AUTO,
	KW_BOOL,
	KW_BREAK,
	KW_CASE,
	KW_CATCH,
	KW_CHAR,
	KW_CHAR16_T,
	KW_CHAR32_T,
	KW_CLASS,
	KW_CONST,
	KW_CONSTEXPR,
	KW_CONST_CAST,
	KW_CONTINUE,
	KW_DECLTYPE,
	KW_DEFAULT,
	KW_DELETE,
	KW_DO,
	KW_DOUBLE,
	KW_DYNAMIC_CAST,
	KW_ELSE,
	KW_ENUM,
	KW_EXPLICIT,
	KW_EXPORT,
	KW_EXTERN,
	KW_FALSE,
	KW_FLOAT,
	KW_FOR,
	KW_FRIEND,
	KW_GOTO,
	KW_IF,
	KW_INLINE,
	KW_INT,
	KW_LONG,
	KW_MUTABLE,
	KW_NAMESPACE,
	KW_NEW,
	KW_NOEXCEPT,
	KW_NULLPTR,
	KW_OPERATOR,
	KW_PRIVATE,
	KW_PROTECTED,
	KW_PUBLIC,
	KW_REGISTER,
	KW_REINTERPET_CAST,
	KW_RETURN,
	KW_SHORT,
	KW_SIGNED,
	KW_SIZEOF,
	KW_STATIC,
	KW_STATIC_ASSERT,
	KW_STATIC_CAST,
	KW_STRUCT,
	KW_SWITCH,
	KW_TEMPLATE,
	KW_THIS,
	KW_THREAD_LOCAL,
	KW_THROW,
	KW_TRUE,
	KW_TRY,
	KW_TYPEDEF,
	KW_TYPEID,
	KW_TYPENAME,
	KW_UNION,
	KW_UNSIGNED,
	KW_USING,
	KW_VIRTUAL,
	KW_VOID,
	KW_VOLATILE,
	KW_WCHAR_T,
	KW_WHILE,

	// operators/punctuation
	OP_LBRACE,
	OP_RBRACE,
	OP_LSQUARE,
	OP_RSQUARE,
	OP_LPAREN,
	OP_RPAREN,
	OP_BOR,
	OP_XOR,
	OP_COMPL,
	OP_AMP,
	OP_LNOT,
	OP_SEMICOLON,
	OP_COLON,
	OP_DOTS,
	OP_QMARK,
	OP_COLON2,
	OP_DOT,
	OP_DOTSTAR,
	OP_PLUS,
	OP_MINUS,
	OP_STAR,
	OP_DIV,
	OP_MOD,
	OP_ASS,
	OP_LT,
	OP_GT,
	OP_PLUSASS,
	OP_MINUSASS,
	OP_STARASS,
	OP_DIVASS,
	OP_MODASS,
	OP_XORASS,
	OP_BANDASS,
	OP_BORASS,
	OP_LSHIFT,
	OP_RSHIFT,
	OP_RSHIFTASS,
	OP_LSHIFTASS,
	OP_EQ,
	OP_NE,
	OP_LE,
	OP_GE,
	OP_LAND,
	OP_LOR,
	OP_INC,
	OP_DEC,
	OP_COMMA,
	OP_ARROWSTAR,
	OP_ARROW,
};

// StringToETokenTypeMap map of `simple` `preprocessing-tokens` to ETokenType
const unordered_map<string, ETokenType> StringToTokenTypeMap =
{
	// keywords
	{"alignas", KW_ALIGNAS},
	{"alignof", KW_ALIGNOF},
	{"asm", KW_ASM},
	{"auto", KW_AUTO},
	{"bool", KW_BOOL},
	{"break", KW_BREAK},
	{"case", KW_CASE},
	{"catch", KW_CATCH},
	{"char", KW_CHAR},
	{"char16_t", KW_CHAR16_T},
	{"char32_t", KW_CHAR32_T},
	{"class", KW_CLASS},
	{"const", KW_CONST},
	{"constexpr", KW_CONSTEXPR},
	{"const_cast", KW_CONST_CAST},
	{"continue", KW_CONTINUE},
	{"decltype", KW_DECLTYPE},
	{"default", KW_DEFAULT},
	{"delete", KW_DELETE},
	{"do", KW_DO},
	{"double", KW_DOUBLE},
	{"dynamic_cast", KW_DYNAMIC_CAST},
	{"else", KW_ELSE},
	{"enum", KW_ENUM},
	{"explicit", KW_EXPLICIT},
	{"export", KW_EXPORT},
	{"extern", KW_EXTERN},
	{"false", KW_FALSE},
	{"float", KW_FLOAT},
	{"for", KW_FOR},
	{"friend", KW_FRIEND},
	{"goto", KW_GOTO},
	{"if", KW_IF},
	{"inline", KW_INLINE},
	{"int", KW_INT},
	{"long", KW_LONG},
	{"mutable", KW_MUTABLE},
	{"namespace", KW_NAMESPACE},
	{"new", KW_NEW},
	{"noexcept", KW_NOEXCEPT},
	{"nullptr", KW_NULLPTR},
	{"operator", KW_OPERATOR},
	{"private", KW_PRIVATE},
	{"protected", KW_PROTECTED},
	{"public", KW_PUBLIC},
	{"register", KW_REGISTER},
	{"reinterpret_cast", KW_REINTERPET_CAST},
	{"return", KW_RETURN},
	{"short", KW_SHORT},
	{"signed", KW_SIGNED},
	{"sizeof", KW_SIZEOF},
	{"static", KW_STATIC},
	{"static_assert", KW_STATIC_ASSERT},
	{"static_cast", KW_STATIC_CAST},
	{"struct", KW_STRUCT},
	{"switch", KW_SWITCH},
	{"template", KW_TEMPLATE},
	{"this", KW_THIS},
	{"thread_local", KW_THREAD_LOCAL},
	{"throw", KW_THROW},
	{"true", KW_TRUE},
	{"try", KW_TRY},
	{"typedef", KW_TYPEDEF},
	{"typeid", KW_TYPEID},
	{"typename", KW_TYPENAME},
	{"union", KW_UNION},
	{"unsigned", KW_UNSIGNED},
	{"using", KW_USING},
	{"virtual", KW_VIRTUAL},
	{"void", KW_VOID},
	{"volatile", KW_VOLATILE},
	{"wchar_t", KW_WCHAR_T},
	{"while", KW_WHILE},

	// operators/punctuation
	{"{", OP_LBRACE},
	{"<%", OP_LBRACE},
	{"}", OP_RBRACE},
	{"%>", OP_RBRACE},
	{"[", OP_LSQUARE},
	{"<:", OP_LSQUARE},
	{"]", OP_RSQUARE},
	{":>", OP_RSQUARE},
	{"(", OP_LPAREN},
	{")", OP_RPAREN},
	{"|", OP_BOR},
	{"bitor", OP_BOR},
	{"^", OP_XOR},
	{"xor", OP_XOR},
	{"~", OP_COMPL},
	{"compl", OP_COMPL},
	{"&", OP_AMP},
	{"bitand", OP_AMP},
	{"!", OP_LNOT},
	{"not", OP_LNOT},
	{";", OP_SEMICOLON},
	{":", OP_COLON},
	{"...", OP_DOTS},
	{"?", OP_QMARK},
	{"::", OP_COLON2},
	{".", OP_DOT},
	{".*", OP_DOTSTAR},
	{"+", OP_PLUS},
	{"-", OP_MINUS},
	{"*", OP_STAR},
	{"/", OP_DIV},
	{"%", OP_MOD},
	{"=", OP_ASS},
	{"<", OP_LT},
	{">", OP_GT},
	{"+=", OP_PLUSASS},
	{"-=", OP_MINUSASS},
	{"*=", OP_STARASS},
	{"/=", OP_DIVASS},
	{"%=", OP_MODASS},
	{"^=", OP_XORASS},
	{"xor_eq", OP_XORASS},
	{"&=", OP_BANDASS},
	{"and_eq", OP_BANDASS},
	{"|=", OP_BORASS},
	{"or_eq", OP_BORASS},
	{"<<", OP_LSHIFT},
	{">>", OP_RSHIFT},
	{">>=", OP_RSHIFTASS},
	{"<<=", OP_LSHIFTASS},
	{"==", OP_EQ},
	{"!=", OP_NE},
	{"not_eq", OP_NE},
	{"<=", OP_LE},
	{">=", OP_GE},
	{"&&", OP_LAND},
	{"and", OP_LAND},
	{"||", OP_LOR},
	{"or", OP_LOR},
	{"++", OP_INC},
	{"--", OP_DEC},
	{",", OP_COMMA},
	{"->*", OP_ARROWSTAR},
	{"->", OP_ARROW}
};

// map of enum to string
const map<ETokenType, string> TokenTypeToStringMap =
{
	{KW_ALIGNAS, "KW_ALIGNAS"},
	{KW_ALIGNOF, "KW_ALIGNOF"},
	{KW_ASM, "KW_ASM"},
	{KW_AUTO, "KW_AUTO"},
	{KW_BOOL, "KW_BOOL"},
	{KW_BREAK, "KW_BREAK"},
	{KW_CASE, "KW_CASE"},
	{KW_CATCH, "KW_CATCH"},
	{KW_CHAR, "KW_CHAR"},
	{KW_CHAR16_T, "KW_CHAR16_T"},
	{KW_CHAR32_T, "KW_CHAR32_T"},
	{KW_CLASS, "KW_CLASS"},
	{KW_CONST, "KW_CONST"},
	{KW_CONSTEXPR, "KW_CONSTEXPR"},
	{KW_CONST_CAST, "KW_CONST_CAST"},
	{KW_CONTINUE, "KW_CONTINUE"},
	{KW_DECLTYPE, "KW_DECLTYPE"},
	{KW_DEFAULT, "KW_DEFAULT"},
	{KW_DELETE, "KW_DELETE"},
	{KW_DO, "KW_DO"},
	{KW_DOUBLE, "KW_DOUBLE"},
	{KW_DYNAMIC_CAST, "KW_DYNAMIC_CAST"},
	{KW_ELSE, "KW_ELSE"},
	{KW_ENUM, "KW_ENUM"},
	{KW_EXPLICIT, "KW_EXPLICIT"},
	{KW_EXPORT, "KW_EXPORT"},
	{KW_EXTERN, "KW_EXTERN"},
	{KW_FALSE, "KW_FALSE"},
	{KW_FLOAT, "KW_FLOAT"},
	{KW_FOR, "KW_FOR"},
	{KW_FRIEND, "KW_FRIEND"},
	{KW_GOTO, "KW_GOTO"},
	{KW_IF, "KW_IF"},
	{KW_INLINE, "KW_INLINE"},
	{KW_INT, "KW_INT"},
	{KW_LONG, "KW_LONG"},
	{KW_MUTABLE, "KW_MUTABLE"},
	{KW_NAMESPACE, "KW_NAMESPACE"},
	{KW_NEW, "KW_NEW"},
	{KW_NOEXCEPT, "KW_NOEXCEPT"},
	{KW_NULLPTR, "KW_NULLPTR"},
	{KW_OPERATOR, "KW_OPERATOR"},
	{KW_PRIVATE, "KW_PRIVATE"},
	{KW_PROTECTED, "KW_PROTECTED"},
	{KW_PUBLIC, "KW_PUBLIC"},
	{KW_REGISTER, "KW_REGISTER"},
	{KW_REINTERPET_CAST, "KW_REINTERPET_CAST"},
	{KW_RETURN, "KW_RETURN"},
	{KW_SHORT, "KW_SHORT"},
	{KW_SIGNED, "KW_SIGNED"},
	{KW_SIZEOF, "KW_SIZEOF"},
	{KW_STATIC, "KW_STATIC"},
	{KW_STATIC_ASSERT, "KW_STATIC_ASSERT"},
	{KW_STATIC_CAST, "KW_STATIC_CAST"},
	{KW_STRUCT, "KW_STRUCT"},
	{KW_SWITCH, "KW_SWITCH"},
	{KW_TEMPLATE, "KW_TEMPLATE"},
	{KW_THIS, "KW_THIS"},
	{KW_THREAD_LOCAL, "KW_THREAD_LOCAL"},
	{KW_THROW, "KW_THROW"},
	{KW_TRUE, "KW_TRUE"},
	{KW_TRY, "KW_TRY"},
	{KW_TYPEDEF, "KW_TYPEDEF"},
	{KW_TYPEID, "KW_TYPEID"},
	{KW_TYPENAME, "KW_TYPENAME"},
	{KW_UNION, "KW_UNION"},
	{KW_UNSIGNED, "KW_UNSIGNED"},
	{KW_USING, "KW_USING"},
	{KW_VIRTUAL, "KW_VIRTUAL"},
	{KW_VOID, "KW_VOID"},
	{KW_VOLATILE, "KW_VOLATILE"},
	{KW_WCHAR_T, "KW_WCHAR_T"},
	{KW_WHILE, "KW_WHILE"},
	{OP_LBRACE, "OP_LBRACE"},
	{OP_RBRACE, "OP_RBRACE"},
	{OP_LSQUARE, "OP_LSQUARE"},
	{OP_RSQUARE, "OP_RSQUARE"},
	{OP_LPAREN, "OP_LPAREN"},
	{OP_RPAREN, "OP_RPAREN"},
	{OP_BOR, "OP_BOR"},
	{OP_XOR, "OP_XOR"},
	{OP_COMPL, "OP_COMPL"},
	{OP_AMP, "OP_AMP"},
	{OP_LNOT, "OP_LNOT"},
	{OP_SEMICOLON, "OP_SEMICOLON"},
	{OP_COLON, "OP_COLON"},
	{OP_DOTS, "OP_DOTS"},
	{OP_QMARK, "OP_QMARK"},
	{OP_COLON2, "OP_COLON2"},
	{OP_DOT, "OP_DOT"},
	{OP_DOTSTAR, "OP_DOTSTAR"},
	{OP_PLUS, "OP_PLUS"},
	{OP_MINUS, "OP_MINUS"},
	{OP_STAR, "OP_STAR"},
	{OP_DIV, "OP_DIV"},
	{OP_MOD, "OP_MOD"},
	{OP_ASS, "OP_ASS"},
	{OP_LT, "OP_LT"},
	{OP_GT, "OP_GT"},
	{OP_PLUSASS, "OP_PLUSASS"},
	{OP_MINUSASS, "OP_MINUSASS"},
	{OP_STARASS, "OP_STARASS"},
	{OP_DIVASS, "OP_DIVASS"},
	{OP_MODASS, "OP_MODASS"},
	{OP_XORASS, "OP_XORASS"},
	{OP_BANDASS, "OP_BANDASS"},
	{OP_BORASS, "OP_BORASS"},
	{OP_LSHIFT, "OP_LSHIFT"},
	{OP_RSHIFT, "OP_RSHIFT"},
	{OP_RSHIFTASS, "OP_RSHIFTASS"},
	{OP_LSHIFTASS, "OP_LSHIFTASS"},
	{OP_EQ, "OP_EQ"},
	{OP_NE, "OP_NE"},
	{OP_LE, "OP_LE"},
	{OP_GE, "OP_GE"},
	{OP_LAND, "OP_LAND"},
	{OP_LOR, "OP_LOR"},
	{OP_INC, "OP_INC"},
	{OP_DEC, "OP_DEC"},
	{OP_COMMA, "OP_COMMA"},
	{OP_ARROWSTAR, "OP_ARROWSTAR"},
	{OP_ARROW, "OP_ARROW"}
};

// convert integer [0,15] to hexadecimal digit
char ValueToHexChar(int c)
{
	switch (c)
	{
	case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	case 8: return '8';
	case 9: return '9';
	case 10: return 'A';
	case 11: return 'B';
	case 12: return 'C';
	case 13: return 'D';
	case 14: return 'E';
	case 15: return 'F';
	default: throw logic_error("ValueToHexChar of nonhex value");
	}
}

// hex dump memory range
string HexDump(const void* pdata, size_t nbytes)
{
	unsigned char* p = (unsigned char*) pdata;

	string s(nbytes*2, '?');

	for (size_t i = 0; i < nbytes; i++)
	{
		s[2*i+0] = ValueToHexChar((p[i] & 0xF0) >> 4);
		s[2*i+1] = ValueToHexChar((p[i] & 0x0F) >> 0);
	}

	return s;
}

// DebugPostTokenOutputStream: helper class to produce PA2 output format
struct DebugPostTokenOutputStream
{
	// output: invalid <source>
	void emit_invalid(const string& source)
	{
		cout << "invalid " << source << endl;
	}

	// output: simple <source> <token_type>
	void emit_simple(const string& source, ETokenType token_type)
	{
		cout << "simple " << source << " " << TokenTypeToStringMap.at(token_type) << endl;
	}

	// output: identifier <source>
	void emit_identifier(const string& source)
	{
		cout << "identifier " << source << endl;
	}

	// output: literal <source> <type> <hexdump(data,nbytes)>
	void emit_literal(const string& source, EFundamentalType type, const void* data, size_t nbytes)
	{
		cout << "literal " << source << " " << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
	}

	// output: literal <source> array of <num_elements> <type> <hexdump(data,nbytes)>
	void emit_literal_array(const string& source, size_t num_elements, EFundamentalType type, const void* data, size_t nbytes)
	{
		cout << "literal " << source << " array of " << num_elements << " " << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
	}

	// output: user-defined-literal <source> <ud_suffix> character <type> <hexdump(data,nbytes)>
	void emit_user_defined_literal_character(const string& source, const string& ud_suffix, EFundamentalType type, const void* data, size_t nbytes)
	{
		cout << "user-defined-literal " << source << " " << ud_suffix << " character " << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
	}

	// output: user-defined-literal <source> <ud_suffix> string array of <num_elements> <type> <hexdump(data, nbytes)>
	void emit_user_defined_literal_string_array(const string& source, const string& ud_suffix, size_t num_elements, EFundamentalType type, const void* data, size_t nbytes)
	{
		cout << "user-defined-literal " << source << " " << ud_suffix << " string array of " << num_elements << " " << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
	}

	// output: user-defined-literal <source> <ud_suffix> <prefix>
	void emit_user_defined_literal_integer(const string& source, const string& ud_suffix, const string& prefix)
	{
		cout << "user-defined-literal " << source << " " << ud_suffix << " integer " << prefix << endl;
	}

	// output: user-defined-literal <source> <ud_suffix> <prefix>
	void emit_user_defined_literal_floating(const string& source, const string& ud_suffix, const string& prefix)
	{
		cout << "user-defined-literal " << source << " " << ud_suffix << " floating " << prefix << endl;
	}

	// output : eof
	void emit_eof()
	{
		cout << "eof" << endl;
	}
};


// use these 3 functions to scan `floating-literals` (see PA2)
// for example PA2Decode_float("12.34") returns "12.34" as a `float` type

float PA2Decode_float(const string& s)
{
	istringstream iss(s);
	float x;
	iss >> x;
	return x;
}

double PA2Decode_double(const string& s)
{
	istringstream iss(s);
	double x;
	iss >> x;
	return x;
}

long double PA2Decode_long_double(const string& s)
{
	istringstream iss(s);
	long double x;
	iss >> x;
	return x;
}

EFundamentalType GetTypeOfInteger(string ppnumber,bool isunsigned){
	string num16 = "0";
	int w = 10,start = 0;
	if(ppnumber[0] == '0'){
		if(ppnumber[1] == 'x' || ppnumber[1] == 'X'){start = 2; w = 16;}
		else {start = 1; w = 8;}
	}
	int nbytes = 0;
	if(w == 16) {
		num16 = ppnumber.substr(2);
		reverse(num16.begin(),num16.end());
		int pos = num16.length()-1,zeros = 0;
		while(num16[pos]==0){
			pos--; zeros++;
		}
		num16 = num16.substr(0,num16.length()-zeros);
	}
	else{
		for(uint i = start;i < ppnumber.length();i++){
			int c = 0;
			for(uint j = 0;j < num16.length();j++){		// mutiply w
				int curr = HexCharToValue(num16[j]) ;
				num16[j] = ValueToHexChar((curr * w + c) % 16);	
				c = (curr * w + c) / 16;
			}
			if(c > 0) num16 += ValueToHexChar(c);
			
			c = ppnumber[i] - '0'; 
			for(uint j = 0;j < num16.length();j++){		// add current number 
				int curr = HexCharToValue(num16[j]) ;
				num16[j] = ValueToHexChar((curr + c) % 16);	
				c = (curr + c) / 16;
			}
			if(c > 0) num16 += ValueToHexChar(c);
		}
		
	}
	nbytes = (num16.length()+1)/2;
	if(nbytes <= 4){
		if(nbytes == 4 && ((HexCharToValue(num16[num16.length()-1]) & 0x8) == 0x8)){
			if(w == 10 && isunsigned == false) return  FundamentalTypeOf<long int>();
			else return  FundamentalTypeOf<unsigned int>();
		}
		else if(isunsigned == false) return FundamentalTypeOf<int>();
		else return FundamentalTypeOf<unsigned int>();
	}
	else if(nbytes <= 8){
		if(nbytes == 8 && ((HexCharToValue(num16[num16.length()-1]) & 0x8) == 0x8)) {
			if(w == 10 && isunsigned == false) return  FT_INVALID;
			else return  FundamentalTypeOf<unsigned long int>();
		}
		else if(isunsigned == false) return FundamentalTypeOf<long int>();
		else return FundamentalTypeOf<unsigned long int>();
	}
	else return FT_INVALID;
}

struct IPPTokenStream
{
	vector<string> string_literals;
	DebugPostTokenOutputStream output;

	bool differentiate_string(const string& data,EFundamentalType& type,string& idata,string& udsuffix){
		int state = 0;
		bool invalid=false;
		idata = "";
		int escape = 0;
		uint pos=0;
		type = FT_DEFAULT;
		for(uint i=0;i<data.length();i++){
			char c = data[i];
			switch(state){
				case 0:if(c == 'U'){state = 2; type = FundamentalTypeOf<char32_t>();}
					else if(c == 'L') {state = 2; type = FundamentalTypeOf<wchar_t>();}
					else if(c == 'u') {state = 1; type = FundamentalTypeOf<char16_t>();}
					else if(c == '\"')state = 3;
					else if(c == 'R')state = 12;
					else cerr<<"Is that possible"<<endl;
					break;
				case 1:if(c == '8') {state = 2; type = FundamentalTypeOf<char>();}
					else if(c == '\"') state = 3;
					else if(c == 'R')state = 12;
					else cerr<<"Is that possible"<<endl;
					break;
				case 2:if(c == '\"') state = 3;
					else if(c == 'R')state = 12;
					else cerr<<"Is that possible"<<endl;
					break;
				case 3:if(c == '\"') state = 4;
					else if(c == '\\') state = 5;
					else {idata += c; }
					break;
				case 4:if(c == '_'){state = 11;pos = i;}
					else invalid = true;
					break;
				case 5:state = 3;
					if(c=='\\'){idata += '\\';}
					else if(c=='\"'){idata += '\"';}
					else if(c=='?'){idata += '?';}
					else if(c=='\''){idata += '\'';}
					else if(c=='a'){idata += '\a';}
					else if(c=='b'){idata += '\b';}
					else if(c=='f'){idata += '\f';}
					else if(c=='n'){idata += '\n';}
					else if(c=='r'){idata += '\r';}
					else if(c=='t'){idata += '\t';}
					else if(c=='v'){idata += '\v';}
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 6; escape = c-'0';}
					else if(c == 'x')state = 9; 
					else cerr<<"is that possible?"<<endl;
					break;
				case 6:if(c=='\"'){state=4;idata += Unicode2Utf8(escape);escape = 0;}
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 7; escape = escape * 8 + c - '0';}
					else if(c=='\\'){state = 5; idata += Unicode2Utf8(escape);escape = 0;}
					else {state = 3; idata += Unicode2Utf8(escape);escape = 0; idata += c;}
					break;
				case 7:if(c=='\"'){state=4;idata += Unicode2Utf8(escape);escape = 0;}
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 8; escape = escape * 8 + c - '0';}
					else if(c=='\\'){state = 5; idata += Unicode2Utf8(escape);escape = 0;}
					else {state = 3; idata += Unicode2Utf8(escape);escape = 0; idata += c;}
					break;
				case 8:if(c=='\"'){state=4;idata += Unicode2Utf8(escape);escape = 0;}
					else if(c=='\\'){state = 5; idata += Unicode2Utf8(escape);escape = 0;}
					else {state = 3; idata += Unicode2Utf8(escape);escape = 0; idata += c;}
					break;
				case 9:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 10; escape = HexCharToValue(c);}
					else cerr<<"is that possible?"<<endl;
					break;
				case 10:if(c=='\"'){state=4;idata += Unicode2Utf8(escape);escape = 0;}
					else if(c=='\\'){state = 5; idata += Unicode2Utf8(escape);escape = 0;}
					else if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 10; escape = 16 * escape + HexCharToValue(c);}
					else {state = 3; idata += Unicode2Utf8(escape);escape = 0; idata += c;}
					break;
				case 11:pos = i < pos ? i:pos;	break;
				case 12:pos = i; i = data.length()+1; break;
			}
		}
		if(state == 11) udsuffix = data.substr(pos);
		else if(state == 12) {	
			uint istart = pos,iend = data.length()-1, iud = -1;
			for(;data[iend] != '\"';iend--);          // now the istart and iend are all point to "" 
			iud = iend;
			for(int i = 1;;i++){
				if(data[istart + i] == '(' && data[iend - i] == ')'){
					if(data.substr(istart+1,i-1) == data.substr(iend-i+1,i-1)){
						istart = istart + i;
						iend = iend - i;
						break;
					}					
				}
			}
			idata = data.substr(istart+1,iend - istart - 1);
			if(iud == data.length() - 1) udsuffix = "";
			else udsuffix = data.substr(iud+1);
		}
		else udsuffix = "";
		return invalid;
		
	}

	void clear_string_literals(){
		string total, totaldata;// totaldata are in the origin code
		string udsuffix;
		int num_elements = 0;
		bool invalid = false;
		EFundamentalType type = FT_DEFAULT;
		for(uint i=0;i<string_literals.size();i++){
			EFundamentalType currtype;
			string idata,currsuffix;
			invalid |= differentiate_string(string_literals[i],currtype,idata,currsuffix);
//cout<<"invalid |= differentiate "<< invalid<<endl; 
			if(udsuffix=="") udsuffix = currsuffix;
			else if(currsuffix!="" && udsuffix != currsuffix)	invalid = true;
//cout<<"if(udsuffix=="") u       "<<invalid<<endl;
			if(type == FT_DEFAULT) type = currtype;
			else if(currtype != FT_DEFAULT && type != currtype) invalid = true;
//cout<<"if(type == FT_DEFAULT)   "<<invalid<<endl;

			total += string_literals[i];
			if(i!=string_literals.size()-1)total+=" ";
			totaldata += idata; 
		}
		totaldata += '\0';
		string_literals.clear();
		
		int nbytes = 1;
		if(invalid){
			output.emit_invalid(total);
		}else{			
			if(type == FundamentalTypeOf<char16_t>()){
				vector<int> Unicode = strUtf82Unicode(totaldata);
				nbytes = 2;
				totaldata = strUnicode2Utf16(Unicode);
			}else if(type == FundamentalTypeOf<char32_t>()|| type == FundamentalTypeOf<wchar_t>()){
				vector<int> Unicode = strUtf82Unicode(totaldata);
				nbytes = 4;
				totaldata = strUnicode2Utf32(Unicode);
			}else type = FundamentalTypeOf<char>();

			num_elements = totaldata.length()/nbytes;
			if(udsuffix == ""){
				output.emit_literal_array(total, num_elements, type, totaldata.data(), totaldata.length());
			}else{
				output.emit_user_defined_literal_string_array(total,udsuffix, num_elements, type,totaldata.data(),totaldata.length());
			}
		}
	}

	void emit_whitespace_sequence(){}
	void emit_new_line(){}
	void emit_header_name(const string& data){
		if(!string_literals.empty()) clear_string_literals();
		output.emit_invalid(data);
	}
	void emit_identifier(const string& data){
		if(!string_literals.empty()) clear_string_literals();
		if(StringToTokenTypeMap.find(data) != StringToTokenTypeMap.end()){
			output.emit_simple(data, StringToTokenTypeMap.at(data));
		}else{
			output.emit_identifier(data);
		}
	}
	void emit_pp_number(const string& ppnumber){
		if(!string_literals.empty()) clear_string_literals();
		int state = 0;
		int ERROR = 22;
		long value = 0;
		string udsuffix="_";
		char c=0;
		for(uint i=0;i<ppnumber.length();i++){
			c = ppnumber[i];
			switch(state){
				case 0:if(c<='9' && c>='1') {state=1;value = c - '0';}
					else if(c=='0') state=7;
					else if(c=='.') state=15;
					else state=ERROR;
					break;
				case 1:if(Digit.find(c)!=Digit.end()) {state=1;value = value * 10 + c - '0';}
					else if(c == 'u' || c == 'U') state=2;
					else if(c == 'l' || c == 'L') state=5;
					else if(c == '_') state = 14;
					else if(c == '.') state = 15;
					else if(c == 'e' || c == 'E') state = 16;
					else state = ERROR;
					break;
				case 2:if(c == 'l' || c == 'L') state=3; 
					else state = ERROR;
					break;
				case 3:if(c == 'l' || c == 'L') state=4; 
					else state = ERROR;
					break;	 
				case 4:state = ERROR; break;
				case 5:if(c == 'u' || c == 'U') state=3;
					else if(c == 'l' || c == 'L') state=6;
					else state = ERROR;
					break; 	
				case 6:if(c == 'u' || c == 'U') state=4; 
					else state = ERROR;
					break;		
				case 7:if(Octal_Digit.find(c)!=Octal_Digit.end()) {state=7; value = value * 8 + c - '0';}
					else if(c == 'u' || c == 'U') state=8;
					else if(c == 'l' || c == 'L') state=12;
					else if(c == 'x' || c == 'X') state=23;
					else if(c == '_') state = 14;
					else if(c == '.') state = 15;
					else if(c == 'e' || c == 'E') state = 16;
					else state = ERROR;
					break;
				case 8:if(c == 'l' || c == 'L') state=9; 
					else state = ERROR;
					break;
				case 9:if(c == 'l' || c == 'L') state=10; 
					else state = ERROR;
					break;
				case 10:state = ERROR; break;
				case 11:if(Hexadecimal_Digit.find(c)!=Hexadecimal_Digit.end()) {state=11; value = value * 16 + HexCharToValue(c);}
					else if(c == 'u' || c == 'U') state=8;
					else if(c == 'l' || c == 'L') state=12;
					else if(c == '_') state = 14;
					else state = ERROR;
					break;
				case 12:if(c == 'u' || c == 'U') state=9;
					else if(c == 'l' || c == 'L') state=13;
					else state = ERROR;
					break; 	
				case 13:if(c == 'u' || c == 'U') state=10; 
					else state = ERROR;
					break;	
				case 14:if(Is_identifier_nondigit(c) || Digit.find(c)!=Digit.end()){state=14; udsuffix += Unicode2Utf8(c);}
					else state = ERROR;
					break;
				case 15:if(Digit.find(c)!=Digit.end()) state=15;
					else if(c == 'e' || c == 'E') state=16;
					else if(c == 'l' || c == 'L') state=19;
					else if(c == 'f' || c == 'F') state=20;
					else if(c == '_') state = 21;
					else state = ERROR;
					break;
				case 16:if(c == '+' || c == '-') state=18;
					else if(Digit.find(c)!=Digit.end()) state=17;
					else state = ERROR;
					break; 	
				case 17:if(c == '_') state=21;
					else if(Digit.find(c)!=Digit.end()) state=17;
					else if(c == 'f' || c == 'F') state=20;
					else state = ERROR;
					break; 	
				case 18:if(Digit.find(c)!=Digit.end()) state=17;
					else state = ERROR;
					break; 	
				case 19:
				case 20:if(c == '_') state=21;
					else state = ERROR;
					break; 	
				case 21:if(Is_identifier_nondigit(c) || Digit.find(c)!=Digit.end()){state=21;udsuffix += Unicode2Utf8(c);}
					else state = ERROR;
					break;
				case 23:if(Hexadecimal_Digit.find(c)!=Hexadecimal_Digit.end()) {state=11; value = value * 16 + HexCharToValue(c);}
					else state = ERROR;
					break;
			}
		}
		if(state == 1){
			EFundamentalType type = GetTypeOfInteger(ppnumber,false);
			if(type == FundamentalTypeOf<int>()) output.emit_literal(ppnumber, FundamentalTypeOf<int>(), &value, 4);
			else if(type == FundamentalTypeOf<long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 2){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-1),true);
			if(type == FundamentalTypeOf<unsigned int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned int>(), &value, 4);
			else if(type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 3){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-2),true);
			if(type == FundamentalTypeOf<unsigned int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else if(type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 4){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-3),true);
			if(ppnumber[ppnumber.length()-1]=='u' || ppnumber[ppnumber.length()-1]=='U'){
				if(ppnumber[ppnumber.length()-2] != ppnumber[ppnumber.length()-3]){
					type = FT_INVALID;
				}
			}else if (ppnumber[ppnumber.length()-2]=='u' || ppnumber[ppnumber.length()-2]=='U'){
				type = FT_INVALID;
			}else{
				if(ppnumber[ppnumber.length()-1] != ppnumber[ppnumber.length()-2]){
					type = FT_INVALID;
				}
			}
			if(type != FT_INVALID) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 5){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-1),false);
			if(type == FundamentalTypeOf<int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 6){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-2),false);
			if(ppnumber[ppnumber.length()-1] != ppnumber[ppnumber.length()-2])
				type = FT_INVALID;
			if(type != FT_INVALID) output.emit_literal(ppnumber, FundamentalTypeOf<long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 7){
			EFundamentalType type = GetTypeOfInteger(ppnumber,false);
			if(type == FundamentalTypeOf<int>() || type == FundamentalTypeOf<unsigned int>())output.emit_literal(ppnumber, type, &value, 4);
			else if(type == FundamentalTypeOf<long int>() || type == FundamentalTypeOf<unsigned long int>())output.emit_literal(ppnumber, type, &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 8){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-1),true);
			if(type == FundamentalTypeOf<int>() || type == FundamentalTypeOf<unsigned int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned int>(), &value, 4);
			else if(type == FundamentalTypeOf<long int>() || type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 9){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-2),true);
			if(type == FundamentalTypeOf<int>() || type == FundamentalTypeOf<unsigned int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long int>() || type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 10){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-3),true);
			if(ppnumber[ppnumber.length()-1]=='u' || ppnumber[ppnumber.length()-1]=='U'){
				if(ppnumber[ppnumber.length()-2] != ppnumber[ppnumber.length()-3]){
					type = FT_INVALID;
				}
			}else if (ppnumber[ppnumber.length()-2]=='u' || ppnumber[ppnumber.length()-2]=='U'){
				type = FT_INVALID;
			}else{
				if(ppnumber[ppnumber.length()-1] != ppnumber[ppnumber.length()-2]){
					type = FT_INVALID;
				}
			}
			if(type != FT_INVALID) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 11){
			EFundamentalType type = GetTypeOfInteger(ppnumber,false);
			if(type == FundamentalTypeOf<int>() || type == FundamentalTypeOf<unsigned int>())output.emit_literal(ppnumber, type, &value, 4);
			else if(type == FundamentalTypeOf<long int>() || type == FundamentalTypeOf<unsigned long int>())output.emit_literal(ppnumber, type, &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 12){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-1),false);
			if(type == FundamentalTypeOf<int>() || type == FundamentalTypeOf<unsigned int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long int>(), &value, 8);
			else if(type == FundamentalTypeOf<long int>() || type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, type, &value, 8);
			else if(type == FundamentalTypeOf<long long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<long long int>(), &value,8);
			else output.emit_invalid(ppnumber);
		}else if(state == 13){
			EFundamentalType type = GetTypeOfInteger(ppnumber.substr(0,ppnumber.length()-2),false);
			if(ppnumber[ppnumber.length()-1] != ppnumber[ppnumber.length()-2])
				type = FT_INVALID;
			if(type == FundamentalTypeOf<unsigned long int>()) output.emit_literal(ppnumber, FundamentalTypeOf<unsigned long long int>(), &value, 8);
			else if(type != FT_INVALID) output.emit_literal(ppnumber, FundamentalTypeOf<long long int>(), &value, 8);
			else output.emit_invalid(ppnumber);
		}else if(state == 14){
			output.emit_user_defined_literal_integer(ppnumber,udsuffix,ppnumber.substr(0,ppnumber.length()-udsuffix.length()));	
		}else if(state == 15 || state == 17){
			double dvalue = PA2Decode_double(ppnumber);
			output.emit_literal(ppnumber, FundamentalTypeOf<double>(), &dvalue, 8);
		}else if(state == 19){
			long double dvalue = PA2Decode_long_double(ppnumber);
			output.emit_literal(ppnumber, FundamentalTypeOf<long double>(), &dvalue, 16);
		}else if(state == 20){
			float dvalue = PA2Decode_float(ppnumber);
			output.emit_literal(ppnumber, FundamentalTypeOf<float>(), &dvalue, 4);
		}else if(state == 21){
			output.emit_user_defined_literal_floating(ppnumber,udsuffix,ppnumber.substr(0,ppnumber.length()-udsuffix.length()));	
		}else{
			output.emit_invalid(ppnumber);
		}

	}
	int differentiate_character(vector<int> data,EFundamentalType& type,int& idata,size_t& nbytes){
		type = FundamentalTypeOf<char>();
		int state = 0;
		uint pos = 0;
		idata = 0 ;
		for(uint i=0;i<data.size();i++){
			int c = data[i];
			switch(state){
				case 0:state = 1;	
					if(c=='U')type = FundamentalTypeOf<char32_t>();
					else if(c=='u')type = FundamentalTypeOf<char16_t>();
					else if(c=='L')type = FundamentalTypeOf<wchar_t>();
					else if(c=='\'')state=2;
					else type = FT_INVALID;
					break;
				case 1:if(c=='\'')state=2;
					else type = FT_INVALID;
					break;
				case 2:if(c=='\\')state=4;
					else {state = 3;idata = (int) c;}
					break;
				case 3:if(c=='\'')state=5;
					else type = FT_INVALID;
					break;
				case 4:state = 3;
					if(c=='\\')idata = '\\';
					else if(c=='\"')idata = '\"';
					else if(c=='?')idata = '?';
					else if(c=='\'')idata = '\'';
					else if(c=='a')idata = '\a';
					else if(c=='b')idata = '\b';
					else if(c=='f')idata = '\f';
					else if(c=='n')idata = '\n';
					else if(c=='r')idata = '\r';
					else if(c=='t')idata = '\t';
					else if(c=='v')idata = '\v';
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 6; idata = c-'0';}
					else if(c == 'x')state = 9; 
					else type = FT_INVALID;
					break;
				case 5: if(c=='_'){state = 11;pos = i;}
					else type = FT_INVALID;
					break;
				case 6:if(c=='\'')state=5;
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 7; idata = idata * 8 + c - '0';}
					else type = FT_INVALID;
					break;
				case 7:if(c=='\'')state=5;
					else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 8; idata = idata * 8 + c - '0';}
					else type = FT_INVALID;
					break;
				case 8:if(c=='\'')state=5;
					else type = FT_INVALID;
					break;
				case 9:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 10; idata = HexCharToValue(c);}
					else type = FT_INVALID;
					break;
				case 10:if(c=='\'')state=5;
					else if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 10; idata = 16 * idata + HexCharToValue(c);}
					else type = FT_INVALID;
					break;
				case 11:pos = i < pos ? i:pos;	break;
			}
		}
		nbytes = 1;
		if(type == FundamentalTypeOf<char16_t>()) {
			nbytes = 2;
			if(idata > 0xffff){
				type = FT_INVALID;
			}
		}
		else if(type == FundamentalTypeOf<char32_t>()) nbytes = 4;
		else if(type == FundamentalTypeOf<wchar_t>()) nbytes = 4;
		else if(idata > 127) {type = FundamentalTypeOf<int>(); nbytes = 4;}
		return pos;
		
	};
	void emit_character_literal(const string& data){
		if(!string_literals.empty()) clear_string_literals();
		vector<int> unidata = strUtf82Unicode(data);
		EFundamentalType type;
		int idata;
		size_t nbytes;
		differentiate_character(unidata,type,idata,nbytes);
		if(type == FT_INVALID) output.emit_invalid(data);
		else output.emit_literal(data, type, &idata, nbytes);
	}
	void emit_user_defined_character_literal(const string& data){
		if(!string_literals.empty()) clear_string_literals();
		vector<int> unidata = strUtf82Unicode(data);
		EFundamentalType type;
		int idata;
		size_t nbytes;
		int pos = differentiate_character(unidata,type,idata,nbytes);
		if(type == FT_INVALID) output.emit_invalid(data);
		else output.emit_user_defined_literal_character(data, data.substr(pos), type, &idata, nbytes);
	}
	void emit_string_literal(const string& data){
		string_literals.push_back(data);
	}
	void emit_user_defined_string_literal(const string& data){
		string_literals.push_back(data);
	}
	void emit_preprocessing_op_or_punc(const string& data) {
		if(!string_literals.empty()) clear_string_literals();
		if(data == "#" || data =="##" || data == "%:" || data == "%:%:" ){
			output.emit_invalid(data);
		}
		else if(StringToTokenTypeMap.find(data) != StringToTokenTypeMap.end()){
			output.emit_simple(data, StringToTokenTypeMap.at(data));
		}else{
			output.emit_invalid(data);
		}
		
	}
	void emit_non_whitespace_char(const string& data) {
		if(!string_literals.empty()) clear_string_literals();
		output.emit_invalid(data);
	}
	void emit_eof() {
		if(!string_literals.empty()) clear_string_literals();
		output.emit_eof();
	}

	~IPPTokenStream() {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The above are all PA2 code
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Tokenizer
struct PPTokenizer
{
	IPPTokenStream& output;

	PPTokenizer(IPPTokenStream& output)
		: output(output)
	{}

	bool process(int c)       //return whether or not the pointer should go back one int
	{
		// TODO:  Your code goes here.

		// 1. do translation features
		// 2. tokenize resulting stream
		// 3. call an output.emit_* function for each token.
		static int state=0;
		static string result;
		static bool header = false, headerstart = true;
		static int escapefrom = -1;
		switch(state){
			case 0: //result = ""; escapefrom = -1;
				if(c == ' ' || c == '\t') state = 1;
				else if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c) && c != 'u' && c != 'U' && c != 'L') {
					state = 2; 
					result += Unicode2Utf8(c);					
				}
				else if(Digit.find(c) != Digit.end()) {state = 3; result += char(c);}
				else if(c == '.') {state = 5; result += char(c);}
				else if(c == '\''){state = 7; result += char(c);}
				else if(c == 'U' || c == 'L'){state = 10; result += char(c);}
				else if(c == 'u'){state = 11; result += char(c);}
				else if(c == '\"'){state = 13; result += char(c);}
				else if(Is_RawStringPointer(c)){state = 16; result = GetRawString(c);}
				else if(c == '{'){output.emit_preprocessing_op_or_punc("{"); headerstart = false;}
				else if(c == '}'){output.emit_preprocessing_op_or_punc("}"); headerstart = false;}
				else if(c == '['){output.emit_preprocessing_op_or_punc("["); headerstart = false;}
				else if(c == ']'){output.emit_preprocessing_op_or_punc("]"); headerstart = false;}
				else if(c == '('){output.emit_preprocessing_op_or_punc("("); headerstart = false;}
				else if(c == ')'){output.emit_preprocessing_op_or_punc(")"); headerstart = false;}
				else if(c == ';'){output.emit_preprocessing_op_or_punc(";"); headerstart = false;}
				else if(c == '?'){output.emit_preprocessing_op_or_punc("?"); headerstart = false;}
				else if(c == '~'){output.emit_preprocessing_op_or_punc("~"); headerstart = false;}
				else if(c == ','){output.emit_preprocessing_op_or_punc(","); headerstart = false;}
				else if(c == '\n'){output.emit_new_line(); headerstart = true;}
				else if(c == '#'){state = 17;result += char(c);}
				else if(c == '<'){state = 18;result += char(c);}
				else if(c == ':'){state = 22;result += char(c);}
				else if(c == '%'){state = 23;result += char(c);}
				else if(c == '+'){state = 26;result += char(c);}
				else if(c == '-'){state = 27;result += char(c);}
				else if(c == '=' || c == '!' || c == '*' || c == '/' || c == '^'){state = 29;result += char(c);}
				else if(c == '&'){state = 30;result += char(c);}
				else if(c == '|'){state = 31;result += char(c);}
				else if(c == '>'){state = 32;result += char(c);}
				else if(c == EndOfFile)break;
				else if(c == PartialComment) throw logic_error("partial comment");
				else{result += Unicode2Utf8(c); output.emit_non_whitespace_char(result); result = ""; headerstart = false;}
				break;
			case 1:if(c == ' ' || c == 0x0d || c == '\t') state = 1;
				else{state = 0; output.emit_whitespace_sequence(); return 1;}
				break;
			case 2:if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()) {state = 2; result += Unicode2Utf8(c);}
				else{ 
					state = 0; 
					if(Digraph_IdentifierLike_Operators.find(result) != Digraph_IdentifierLike_Operators.end()){
						output.emit_preprocessing_op_or_punc(result);
					}
					else{
						output.emit_identifier(result); 
						if(result == "include"){
							if(header == true){
								state = 40;
								header = false;
							}
						}else{
							header = false;
						}
					}
					result = ""; 
					headerstart = false;
					return 1;
				}
				break;
			case 3:if((Digit.find(c) != Digit.end() || Is_identifier_nondigit(c) || c == '.') && c != 'e' && c != 'E') {state = 3; result += Unicode2Utf8(c);}
				else if(c == 'e' || c == 'E') {state = 4; result += char(c);}
				else{ state = 0; output.emit_pp_number(result); result = ""; headerstart = false; return 1;}
				break;
			case 4:if(Digit.find(c) != Digit.end() || Is_identifier_nondigit(c) || c == '.' || c == '+' || c == '-')
					{state = 3; result += Unicode2Utf8(c);}
				else{ state = 0; output.emit_pp_number(result); result = ""; headerstart = false; return 1;}
				break; 
			case 5:if(Digit.find(c) != Digit.end()){state = 3; result += char(c);}
				else if(c == '.'){state = 6; result += char(c);}
				else if(c == '*'){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result="";}
				else{state = 0; output.emit_preprocessing_op_or_punc(result); result=""; headerstart = false; return 1;}
				break;
			case 6:if(c == '.'){state = 0; result += '.'; output.emit_preprocessing_op_or_punc(result); result = "";}
				else if(Digit.find(c) != Digit.end()){state = 3; output.emit_preprocessing_op_or_punc("."); result = "."; result += char(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("."); output.emit_preprocessing_op_or_punc("."); headerstart = false;return 1;}
				break;
			case 7:if(Is_cchar(c)){state = 7; result += Unicode2Utf8(c);}
				else if(c == '\\'){state = 34; escapefrom = 7; result += '\\';}
				else if(c == '\''){state = 8; result += '\'';}
				else throw logic_error("state 7:unterminated character literal");
				break;
			case 8:if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c)){state = 9; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_character_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 9:if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 9; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_user_defined_character_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 10:if(c == '\''){state = 7; result += char(c);}
				else if(c == '\"'){state = 13; result += char(c);}
				else if(Is_RawStringPointer(c)){state = 16; result += GetRawString(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 11:if(c == '\''){state = 7; result += char(c);}
				else if(c == '\"'){state = 13; result += char(c);}
				else if(Is_RawStringPointer(c)){state = 16; result += GetRawString(c);}
				else if(c == '8'){state = 12; result += char(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}				
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 12:if(c == '\"'){state = 13; result += char(c);}
				else if(Is_RawStringPointer(c)){state = 16; result += GetRawString(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 13:if(Is_schar(c)){state = 13; result += Unicode2Utf8(c);}
				else if(c == '\\'){state = 34; escapefrom = 13; result += char(c);}
				else if(c == '\"'){state = 14; result += char(c);}
				else throw logic_error("state 13:unterminated string literal");
				break;
			case 14:if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c)){state = 15; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_string_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 15:if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 15; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_user_defined_string_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 16:if(Is_identifier_nondigit(c)){state = 15; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_string_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 17:if(c == '#'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("##"); headerstart = false;}
				else{if(headerstart == true)header = true; state = 0; result = ""; output.emit_preprocessing_op_or_punc("#"); headerstart = false; return 1;}
				break;
			case 18:if(c == '<'){state = 19; result += char(c);}
				else if(c == ':'){state = 20; result += char(c);}
				else if(c == '%' || c == '='){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("<"); result = ""; headerstart = false; return 1;}
				break;
			case 19:if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc("<<="); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("<<"); headerstart = false; return 1;}
				break;
			case 20:if(c == ':'){state = 21; result += char(c);}
				else{state = 0; output.emit_preprocessing_op_or_punc("<:"); result = ""; headerstart = false; return 1;}
				break;
			case 21:if(c == '>'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("<:"); output.emit_preprocessing_op_or_punc(":>"); headerstart = false;}
				else if(c == ':'){state = 0;  result = ""; output.emit_preprocessing_op_or_punc("<:"); output.emit_preprocessing_op_or_punc("::"); headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("<"); output.emit_preprocessing_op_or_punc("::"); result = ""; headerstart = false; return 1;}
				break;
			case 22:if(c == ':'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("::"); headerstart = false;}
				else if(c == '>'){state = 0; result = ""; output.emit_preprocessing_op_or_punc(":>"); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc(":"); headerstart = false; return 1;}
				break;
			case 23:if(c == ':'){state = 24; result += char(c);}
				else if(c == '>' || c == '='){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%"); headerstart = false; return 1;}
				break;
			case 24:if(c == '%'){state = 25; result += char(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:"); if( headerstart == true) header = true; headerstart = false; return 1;}
				break;
			case 25:if(c == ':'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:%:"); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:"); output.emit_preprocessing_op_or_punc("%"); headerstart = false; return 1;}
				break;
			case 26:if(c == '+' || c == '='){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result);result = ""; headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("+"); headerstart = false;return 1;}
				break;
			case 27:if(c == '-' || c == '='){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result);result = ""; headerstart = false;}
				else if(c == '>'){state = 28; result += char(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("-"); headerstart = false;return 1;}
				break;
			case 28:if(c == '*'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("->*"); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("->"); headerstart = false;return 1;}
				break;
			case 29:if(c == '='){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false; return 1;}
				break;
			case 30:if(c == '=' || c == '&'){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("&"); result = ""; headerstart = false; return 1;}
				break;
			case 31:if(c == '=' || c == '|'){state = 0; result += char(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("|"); result = ""; headerstart = false; return 1;}
				break;
			case 32:if(c == '>'){state = 33; result += char(c); }
				else if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc(">="); headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(">"); result = ""; headerstart = false; return 1;}
				break;
			case 33:if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc(">>="); headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(">>"); result = ""; headerstart = false; return 1;}
				break;
			case 34:if(SimpleEscapeSequence_CodePoints.find(c) != SimpleEscapeSequence_CodePoints.end()){					
					result += char(c);
					state = escapefrom;
					escapefrom = -1;
				}
				else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 35; result += char(c);}
				else if(c == 'x'){state = 38; result += char(c);}
				else throw logic_error("invalid escape sequence\n");
				break;
			case 35:if(Octal_Digit.find(c) != Octal_Digit.end()){state = 36; result += char(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 36:if(Octal_Digit.find(c) != Octal_Digit.end()){state = escapefrom; escapefrom = -1; result += char(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 38:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 39; result += char(c);}
				else throw logic_error("invalid escape sequence\n");
				break;
			case 39:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 39; result += char(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 40:if(c == ' '){state = 41;}
				else if(c == '<'){state = 42; result += char(c);}
				else if(c == '\"'){state = 43; result += char(c);}
				else{state = 0; return 1;}
				break;
			case 41:if(c == ' '){state = 41;}
				else if(c == '<'){state = 42; result += char(c); output.emit_whitespace_sequence();}
				else if(c == '\"'){state = 43; result += char(c); output.emit_whitespace_sequence();}
				else{state = 0; output.emit_whitespace_sequence(); return 1;}
				break;
			case 42:if(Is_hchar(c)){state = 42; result += char(c);}
				else if(c == '>'){state = 0; result += char(c); output.emit_header_name(result); result = ""; headerstart = false;}
				else{state = 0; return 1;}
				break;
			case 43:if(Is_qchar(c)){state = 43; result += char(c);}
				else if(c == '\"'){state = 0; result += char(c); output.emit_header_name(result); result = ""; headerstart = false;}
				else{state = 0; return 1;}
				break;
			default:throw logic_error("Big DFA failed, what the fuck!");
				
		}		
		if (c == EndOfFile)
		{
			//output.emit_identifier("not_yet_implemented");
			output.emit_eof();
		}
		return 0;
		// TIP: Reference implementation is about 1000 lines of code.
		// It is a state machine with about 50 states, most of which
		// are simple transitions of the operators.
	}
};
int main()
{
	try
	{
		ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();
	
		input += char(-1);
		
		vector<int> iinput = PPTranslator::Translate(input);		
	
		iinput.push_back(-1);

		IPPTokenStream output;

		PPTokenizer tokenizer(output);

		for(uint i=0;i<iinput.size();i++)
		{	
			int code_unit = iinput[i];
			if(tokenizer.process(code_unit)){
				i=i-1;
			}
		}

	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}
