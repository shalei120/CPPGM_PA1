#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#include "IPPTokenStream.h"
#include "DebugPPTokenStream.h"

// Translation features you need to implement:
// - utf8 decoder	
int Utf82Unicode(vector<unsigned char> utf8){
	int result=-1;
	switch(utf8.size()){
		case 1:result = utf8[0];break;
		case 2:result = ((utf8[0] & 0x1f)<<6) + (utf8[1] & 0x3f);break;
		case 3:result = ((utf8[0] & 0x0f)<<12) + ((utf8[1] & 0x3f)<<6) + (utf8[2] & 0x3f);break;
		case 4:result = ((utf8[0] & 0x07)<<18) + ((utf8[1] & 0x3f)<<12) + ((utf8[2] & 0x3f)<<6) + (utf8[3] & 0x3f);break;
		default:cout<<"utf8-->unicode failed!"<<endl;throw logic_error("utf8-->unicode failed!");
			
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
		cout<<"utf8 segmentation error!\n";
		throw logic_error("utf8 segmentation error!\n");
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
//constexpr int PartialComment = 0x00110000;

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
	default: cout<<"HexCharToValue of nonhex char"<<endl;throw logic_error("HexCharToValue of nonhex char");
		
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

unordered_map<int, string> reversestring;
unordered_set<int> line_splice;
vector<int> iinput;
// Tokenizer
struct PPTokenizer
{
	IPPTokenStream& output;

	string delimiter="",delimiter2="";

	PPTokenizer(IPPTokenStream& output)
		: output(output)
	{}

	bool process(int c, int pos)       //return whether or not the pointer should go back one int
	{
		// TODO:  Your code goes here.

		// 1. do translation features
		// 2. tokenize resulting stream
		// 3. call an output.emit_* function for each token.
		static int state=0;
		static string result;
		static bool header = false, headerstart = true;
		static int escapefrom = -1, blankfrom = -1;
		//cout<<state<<" "<<headerstart<<" "<<header<<endl;
		switch(state){
			case 0: //result = ""; escapefrom = -1;
				if(c == ' ' || c == '\t') state = 1;
				else if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c) && c != 'u' && c != 'U' && c != 'L' && c != 'R') {
					state = 2; 
					result += Unicode2Utf8(c);					
				}
				else if(Digit.find(c) != Digit.end()) {state = 3; result += Unicode2Utf8(c);}
				else if(c == '.') {state = 5; result += Unicode2Utf8(c);}
				else if(c == '\''){state = 7; result += Unicode2Utf8(c);}
				else if(c == 'U' || c == 'L'){state = 10; result += Unicode2Utf8(c);}
				else if(c == 'u'){state = 11; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 13; result += Unicode2Utf8(c);}
				else if(c == 'R'){state = 16; result += Unicode2Utf8(c);}
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
				else if(c == '\n'){output.emit_new_line(); headerstart = true;header=false;}
				else if(c == '#'){state = 17;result += Unicode2Utf8(c);}
				else if(c == '<'){state = 18;result += Unicode2Utf8(c);}
				else if(c == ':'){state = 22;result += Unicode2Utf8(c);}
				else if(c == '%'){state = 23;result += Unicode2Utf8(c);}
				else if(c == '+'){state = 26;result += Unicode2Utf8(c);}
				else if(c == '-'){state = 27;result += Unicode2Utf8(c);}
				else if(c == '=' || c == '!' || c == '*' || c == '^'){state = 29;result += Unicode2Utf8(c);}
				else if(c == '/'){state = 46;result += Unicode2Utf8(c);blankfrom = 0;}
				else if(c == '&'){state = 30;result += Unicode2Utf8(c);}
				else if(c == '|'){state = 31;result += Unicode2Utf8(c);}
				else if(c == '>'){state = 32;result += Unicode2Utf8(c);}
				else if(c == EndOfFile)break;
				//else if(c == PartialComment) {cout<<"partial comment"<<endl;throw logic_error("partial comment");}
				else{result += Unicode2Utf8(c); output.emit_non_whitespace_char(result); result = ""; headerstart = false;}
				break;
			case 1:if(c == ' ' || c == 0x0d || c == '\t') state = 1;
				else if(c == '/'){state = 51;blankfrom = 0;}
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
				else if(c == 'e' || c == 'E') {state = 4; result += Unicode2Utf8(c);}
				else{ state = 0; output.emit_pp_number(result); result = ""; headerstart = false; return 1;}
				break;
			case 4:if(Digit.find(c) != Digit.end() || Is_identifier_nondigit(c) || c == '.' || c == '+' || c == '-')
					{state = 3; result += Unicode2Utf8(c);}
				else{ state = 0; output.emit_pp_number(result); result = ""; headerstart = false; return 1;}
				break; 
			case 5:if(Digit.find(c) != Digit.end()){state = 3; result += Unicode2Utf8(c);}
				else if(c == '.'){state = 6; result += Unicode2Utf8(c);}
				else if(c == '*'){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result="";}
				else{state = 0; output.emit_preprocessing_op_or_punc(result); result=""; headerstart = false; return 1;}
				break;
			case 6:if(c == '.'){state = 0; result += '.'; output.emit_preprocessing_op_or_punc(result); result = "";}
				else if(Digit.find(c) != Digit.end()){state = 3; output.emit_preprocessing_op_or_punc("."); result = "."; result += Unicode2Utf8(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("."); output.emit_preprocessing_op_or_punc("."); headerstart = false;return 1;}
				break;
			case 7:if(Is_cchar(c)){state = 7; result +=  Unicode2Utf8(c);}
				else if(c == '\\'){state = 34; escapefrom = 7; result += '\\';}
				else if(c == '\''){state = 8; result += '\'';}
				else {cout<<"state 7:unterminated character literal"<<endl;throw logic_error("state 7:unterminated character literal");}
				break;
			case 8:if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c)){state = 9; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_character_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 9:if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 9; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_user_defined_character_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 10:if(c == '\''){state = 7; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 13; result += Unicode2Utf8(c);}
				else if(c == 'R'){state = 16; result += Unicode2Utf8(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 11:if(c == '\''){state = 7; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 13; result += Unicode2Utf8(c);}
				else if(c == 'R'){state = 16; result += Unicode2Utf8(c);}
				else if(c == '8'){state = 12; result += Unicode2Utf8(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}				
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 12:if(c == '\"'){state = 13; result += Unicode2Utf8(c);}
				else if(c == 'R'){state = 16; result += Unicode2Utf8(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 13:if(Is_schar(c)){state = 13; result +=  Unicode2Utf8(c);}
				else if(c == '\\'){state = 34; escapefrom = 13; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 14; result += Unicode2Utf8(c);}
				else {cout<<"state 13:unterminated string literal"<<endl;throw logic_error("state 13:unterminated string literal");}
				break;
			case 14:if(Is_identifier_nondigit(c) && !Is_Initial_Banned(c)){state = 15; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_string_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 15:if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 15; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_user_defined_string_literal(result); result = ""; headerstart = false; return 1;}
				break;
			case 16:if(c == '\"'){state = 47;result += Unicode2Utf8(c);}
				else if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 2; result += Unicode2Utf8(c);}
				else{state = 0; output.emit_identifier(result); result = ""; headerstart = false; return 1;}
				break;
			case 17:if(c == '#'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("##"); headerstart = false;}
				else{if(headerstart == true)header = true; state = 0; result = ""; output.emit_preprocessing_op_or_punc("#"); headerstart = false; return 1;}
				break;
			case 18:if(c == '<'){state = 19; result += Unicode2Utf8(c);}
				else if(c == ':'){state = 20; result += Unicode2Utf8(c);}
				else if(c == '%' || c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("<"); result = ""; headerstart = false; return 1;}
				break;
			case 19:if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc("<<="); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("<<"); headerstart = false; return 1;}
				break;
			case 20:if(c == ':'){state = 21; result += Unicode2Utf8(c);}
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
			case 23:if(c == ':'){state = 24; result += Unicode2Utf8(c);}
				else if(c == '>' || c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%"); headerstart = false; return 1;}
				break;
			case 24:if(c == '%'){state = 25; result += Unicode2Utf8(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:"); if( headerstart == true) header = true; headerstart = false; return 1;}
				break;
			case 25:if(c == ':'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:%:"); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("%:"); output.emit_preprocessing_op_or_punc("%"); headerstart = false; return 1;}
				break;
			case 26:if(c == '+' || c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result);result = ""; headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("+"); headerstart = false;return 1;}
				break;
			case 27:if(c == '-' || c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result);result = ""; headerstart = false;}
				else if(c == '>'){state = 28; result += Unicode2Utf8(c);}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("-"); headerstart = false;return 1;}
				break;
			case 28:if(c == '*'){state = 0; result = ""; output.emit_preprocessing_op_or_punc("->*"); headerstart = false;}
				else{state = 0; result = ""; output.emit_preprocessing_op_or_punc("->"); headerstart = false;return 1;}
				break;
			case 29:if(c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false; return 1;}
				break;
			case 30:if(c == '=' || c == '&'){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("&"); result = ""; headerstart = false; return 1;}
				break;
			case 31:if(c == '=' || c == '|'){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc("|"); result = ""; headerstart = false; return 1;}
				break;
			case 32:if(c == '>'){state = 33; result += Unicode2Utf8(c); }
				else if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc(">="); headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(">"); result = ""; headerstart = false; return 1;}
				break;
			case 33:if(c == '='){state = 0; result = ""; output.emit_preprocessing_op_or_punc(">>="); headerstart = false;}
				else{state = 0; output.emit_preprocessing_op_or_punc(">>"); result = ""; headerstart = false; return 1;}
				break;
			case 34:if(SimpleEscapeSequence_CodePoints.find(c) != SimpleEscapeSequence_CodePoints.end()){					
					result += Unicode2Utf8(c);
					state = escapefrom;
					escapefrom = -1;
				}
				else if(Octal_Digit.find(c) != Octal_Digit.end()){state = 35; result += Unicode2Utf8(c);}
				else if(c == 'x'){state = 38; result += Unicode2Utf8(c);}
				else {cout<<"invalid escape sequence\n";throw logic_error("invalid escape sequence\n");}
				break;
			case 35:if(Octal_Digit.find(c) != Octal_Digit.end()){state = 36; result += Unicode2Utf8(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 36:if(Octal_Digit.find(c) != Octal_Digit.end()){state = escapefrom; escapefrom = -1; result += Unicode2Utf8(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 37:if(c==0x0a || c==-1){
					if(blankfrom == 0)state=0;
					else if(blankfrom == 41)state = 41;
					else throw logic_error("invalid blankfrom");
					blankfrom = -1; 
					output.emit_whitespace_sequence();
					output.emit_new_line();
					result = "";
				}
				else {state = 37;}
				break;
			case 38:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 39; result += Unicode2Utf8(c);}
				else {cout<<"invalid escape sequence\n";throw logic_error("invalid escape sequence\n");}
				break;
			case 39:if(Hexadecimal_Digit.find(c) != Hexadecimal_Digit.end()){state = 39; result += Unicode2Utf8(c);}
				else{state = escapefrom; escapefrom = -1; return 1;}
				break;
			case 40:if(c == ' '){state = 41;}
				else if(c == '<'){state = 42; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 43; result += Unicode2Utf8(c);}
				else if(c == '/'){state = 51; blankfrom = 41;}
				else{state = 0; return 1;}
				break;
			case 41:if(c == ' '){state = 41;}
				else if(c == '<'){state = 42; result += Unicode2Utf8(c); output.emit_whitespace_sequence();}
				else if(c == '\"'){state = 43; result += Unicode2Utf8(c); output.emit_whitespace_sequence();}
				else if(c == '/'){state = 51; blankfrom = 41;}
				else{state = 0; output.emit_whitespace_sequence(); return 1;}
				break;
			case 42:if(Is_hchar(c)){state = 42; result += Unicode2Utf8(c);}
				else if(c == '>'){state = 0; result += Unicode2Utf8(c); output.emit_header_name(result); result = ""; headerstart = false;}
				else{state = 0; throw logic_error("unterminated header name");return 1;}
				break;
			case 43:if(Is_qchar(c)){state = 43; result += Unicode2Utf8(c);}
				else if(c == '\"'){state = 0; result += Unicode2Utf8(c); output.emit_header_name(result); result = ""; headerstart = false;}
				else{state = 0; return 1;}
				break;
			case 44:if(c == '*'){state = 45; }
				else if(c == EndOfFile){cout<<"partial comment"<<endl;throw logic_error("partial comment");}
				break;
			case 45:if(c == '/'){
					if(blankfrom == 0)state=0;
					else if(blankfrom == 41)state = 41;
					else throw logic_error("invalid blankfrom");
					blankfrom = -1; 
					iinput[pos]=' ';
					result = "";
					return 1;
				}
				else if(c == '*')state=45;
				else if(c == EndOfFile){cout<<"partial comment"<<endl;throw logic_error("partial comment");}
				else state = 44;
				break;
			case 46:if(c == '='){state = 0; result += Unicode2Utf8(c); output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false;}
				else if(c == '/'){state = 37;result += Unicode2Utf8(c);}
				else if(c == '*'){state = 44;result += Unicode2Utf8(c);}
				else {state = 0; output.emit_preprocessing_op_or_punc(result); result = ""; headerstart = false; return 1;}
				break;

			case 47:if(Is_dchar(c)){state=47; 
					if(reversestring.find(pos) == reversestring.end()){
						result += Unicode2Utf8(c); 
						delimiter += char(c);
					}else{
						string rev = reversestring.at(pos);
						result += rev.substr(0,rev.size()-1); 
						delimiter += rev.substr(0,rev.size()-1);
						iinput[pos]=rev[rev.size()-1];
						reversestring.erase(pos);
						return 1;
					}
				}
				else if(c == '('){state=48; result += '(';}
				else {
					throw logic_error("error raw string!\n");
				}
				break;
			case 48:if(c == ')'){state=49; result += ')';}
				else if(c == -1){           //not a raw string			
					throw logic_error("not a raw string\n");
				}
				else{ state = 48;
					if(line_splice.find(pos) != line_splice.end()){//a line splicing before
						result += '\\';result += char(0x0a);line_splice.erase(pos);
					}
					if(reversestring.find(pos) == reversestring.end()){
						result += Unicode2Utf8(c); 
					}else{
						
						string rev = reversestring.at(pos);
						result += rev.substr(0,rev.size()-1); 
						iinput[pos]=rev[rev.size()-1];
						reversestring.erase(pos);
						return 1;
					}
				}
				break;
			case 49:if(line_splice.find(pos) != line_splice.end()){
					result += '\\';result += char(0x0a);
					line_splice.erase(pos);
					state = 48;
					return 1;
				}
				if(c == '\"'){
					result += Unicode2Utf8(c);
					state = 50;
				}else if(Is_dchar(c)){state=49;
					if(reversestring.find(pos) == reversestring.end()){
						result += Unicode2Utf8(c); 
						delimiter2 += char(c);
					}else{
						string rev = reversestring.at(pos);
						result += rev.substr(0,rev.size()-1); 
						delimiter2 += rev.substr(0,rev.size()-1);
						iinput[pos]=rev[rev.size()-1];
						reversestring.erase(pos);
						return 1;
					}
				}else if(c == ')'){
					result += ')';
					delimiter2 = "";
				}else{state = 48;result += Unicode2Utf8(c);}
				break;
			case 50:if(delimiter == delimiter2 && delimiter.length() <= 16){					
					delimiter = "";
					delimiter2 = "";
					if(Is_identifier_nondigit(c) || Digit.find(c) != Digit.end()){state = 15;result += Unicode2Utf8(c);}
					else {state = 0;output.emit_string_literal(result); result = ""; headerstart = false; return 1; }
				}else{
					//cout<<delimiter<<" "<<delimiter2<<endl;
					state = 48;
					delimiter2 = "";
					return 1;
				}
				break;
			case 51:if(c == '*' || c == '/'){state = 46;return 1;}
				else if(c == '='){state = 0;output.emit_whitespace_sequence();output.emit_preprocessing_op_or_punc("/=");}
				else {state = 0;output.emit_whitespace_sequence();output.emit_preprocessing_op_or_punc("/");return 1;}
				break;
			default:cout<<"Big DFA failed, what the fuck!"<<state<<endl;throw logic_error("Big DFA failed, what the fuck!");
				
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

class PPTranslator{
public:
	PPTranslator(){}

	static vector<int> Translate(string s){
		vector<int> results;
		int state=0;
		int result=0;
		string replace;
		for(uint i=0;i<s.length();i++){
			switch(state){
				case 0:if(s[i]=='\\'){state=1; replace += s[i];}
					else if(s[i]=='?'){state=10;replace += s[i];}
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
					else if((s[i] & 0xf8) == 0xf8){cout<<"utf8 invalid unit (111111xx)\n"; throw logic_error("utf8 invalid unit (111111xx)\n");}
					//else if(s[i]=='/')state=16;				
					else results.push_back(s[i]);
					break;
				case 1:if(s[i]=='u'){state=2; replace += s[i];}
					else if(s[i]=='U'){state=6; replace += s[i];}
					else if(s[i]==0x0a) {state = 0;  replace = "";line_splice.insert(results.size());}
					else {i=i-1;state=0;results.push_back('\\'); replace = "";}
					break;
				case 2:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 3; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-2; results.push_back('\\'); replace = "";}
					break;
				case 3:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 4; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-3; results.push_back('\\');replace = "";}
					break;
				case 4:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 5; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-4; results.push_back('\\');replace = "";}
					break;
				case 5:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 0; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);
						results.push_back(result); result = 0;
						reversestring[results.size()-1] = replace;
						replace = "";
					}
					else {state=0; i=i-5; results.push_back('\\');}
					break;

				case 6:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 7; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-2; results.push_back('\\');replace = "";}
					break;
				case 7:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 8; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-3; results.push_back('\\');replace = "";}
					break;
				case 8:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 9; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-4; results.push_back('\\');replace = "";}
					break;
				case 9:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 12; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-5; results.push_back('\\');replace = "";}
					break;

				case 10:if( s[i]=='?' ){state=11;replace += s[i]; }
					else{ state=0; i=i-1; results.push_back('?');replace = "";}
					break;
				case 11:if( s[i]=='=' ){state=0; replace += s[i]; s[i]='#'; i = i - 1;reversestring[results.size()] = replace;replace = "";} 
					else if( s[i]=='/' ){state=0; replace += s[i]; s[i]='\\'; i = i - 1;reversestring[results.size()] = replace;replace = "";} 
					else if( s[i]=='\'' ){state=0; replace += s[i]; s[i]='^'; i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]=='(' ){state=0; replace += s[i]; s[i]='[';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]==')' ){state=0; replace += s[i]; s[i]=']';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]=='!' ){state=0; replace += s[i]; s[i]='|';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]=='<' ){state=0; replace += s[i]; s[i]='{';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]=='>' ){state=0; replace += s[i]; s[i]='}';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else if( s[i]=='-' ){state=0; replace += s[i]; s[i]='~';  i = i - 1;reversestring[results.size()] = replace;replace = "";}
					else{ state=0; i=i-2; results.push_back('?');replace = "";}break;

				case 12:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 13; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-6; results.push_back('\\');replace = "";}
					break;
				case 13:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 14; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-7; results.push_back('\\');replace = ""; }
					break;
				case 14:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 15; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);}
					else {state=0; i=i-8; results.push_back('\\');replace = "";}
					break;
				case 15:if( Hexadecimal_Digit.find(s[i]) != Hexadecimal_Digit.end() ){state = 0; replace += s[i]; result = (result << 4) + HexCharToValue(s[i]);
						results.push_back(result); result = 0;
						reversestring[results.size()-1] = replace;
						replace = "";
					} 
					else {state=0; i=i-9; results.push_back('\\');replace = "";}
					break;
				
				
				default:cout<<"little DFA failed? What the hell is it?\n";throw new logic_error("little DFA failed? What the hell is it?\n");
					
			}
		}
		if(!results.empty() && results.back()!=0x0a)results.push_back(0x0a);
		return results;
	}
};

int main()
{
	try
	{
		//ifstream cin("aaa.txt");
		//ofstream out("bbb.txt");

		ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();
	
		input += char(-1);
		
		iinput = PPTranslator::Translate(input);		
	
		iinput.push_back(-1);

		DebugPPTokenStream output;

		PPTokenizer tokenizer(output);

		for(uint i=0;i<iinput.size();i++)
		{	
			//if(iinput[i] < PPTranslator::RawStringBase)
			//	out<<iinput[i]<<"  "<<char2bin(iinput[i])<<"    "<<char2hex(iinput[i])<<endl;
			//else	out<<PPTranslator::RawStrings[iinput[i] - PPTranslator::RawStringBase]<<endl;
			int code_unit = iinput[i];
			if(tokenizer.process(code_unit,i)){
				i=i-1;
			}
		}

		//tokenizer.process(EndOfFile);
	}
	catch (exception& e)
	{
		cout << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

