//////////////////////////////////////////////////////////////////////////
//                                                                      //
//               NeuroMatrix(r) C runtime library                       //
//                                                                      //
//  sprintf.c                                                           //
//                                                                      //
// Copyright (c) 2004-2008 RC Module                                    //
//                                                                      //
//  Àâòîð: Áèðþêîâ À.À.                                                 //
//  EasyNMC àäàïòàöèÿ - Àíäðèàíî À.Â.                                   //
// $Revision:: 9     $      $Date:: 28.11.11 19:13   $                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <cstdarg>
#include <cassert>
extern "C" {
#include <easynmc/easynmc.h>
}

#include <../crt/auxfd.h>

using namespace std;

#if defined(__NM6403__) || defined(__NM6405__)
#define LLong long long
#else
#if defined(NM6403) || defined(NM6404) || defined(NM6405)
#define LLong long
//#define __old_compiler
#else
	#define LLong long long
#endif  // defined(NM6403) || defined(NM6404) || defined(NM6405)
#endif  // defined(__NM6403__) || defined(__NM6405__)

template <class INT, int BASE> inline void readNumber( char* &p, INT &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % BASE ];
		arg /= BASE;
	}
}

template <class INT, int BASE> inline void readNumberX( char* &p, INT &arg )
{
	const char* latters = "0123456789ABCDEF";
	while ( arg ){
		*p++ = latters[ arg % BASE ];
		arg /= BASE;
	}
}

#ifdef __old_compiler
//	Ðåàëèçóåì øàáëîíû âðó÷íóþ, ...
inline void readNumber<unsigned int, 10>( char* &p, unsigned int &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 10 ];
		arg /= 10;
	}
}

inline void readNumber<unsigned int, 8>( char* &p, unsigned int &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 8 ];
		arg /= 8;
	}
}

inline void readNumber<unsigned int, 16>( char* &p, unsigned int &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 16 ];
		arg /= 16;
	}
}

inline void readNumberX<unsigned int, 16>( char* &p, unsigned int &arg )
{
	const char* latters = "0123456789ABCDEF";
	while ( arg ){
		*p++ = latters[ arg % 16 ];
		arg /= 16;
	}
}

inline void readNumber<unsigned LLong, 10>( char* &p, unsigned LLong &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 10 ];
		arg /= 10;
	}
}

inline void readNumber<unsigned LLong, 8>( char* &p, unsigned LLong &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 8 ];
		arg /= 8;
	}
}

inline void readNumber<unsigned LLong, 16>( char* &p, unsigned LLong &arg )
{
	const char* latters = "0123456789abcdef";
	while ( arg ){
		*p++ = latters[ arg % 16 ];
		arg /= 16;
	}
}

inline void readNumberX<unsigned LLong, 16>( char* &p, unsigned LLong &arg )
{
	const char* latters = "0123456789ABCDEF";
	while ( arg ){
		*p++ = latters[ arg % 16 ];
		arg /= 16;
	}
}

#endif

enum FlagNames{
	LEFT_JUSTIFY = 1,
	PLUS_SIGN = 2,
	INS_SPACE = 4,
	ALT_FORM = 8,
	FILL_ZERO = 16,
	PREC_IS_SET = 32,
	WIDTH_IS_SET = 64
};

static int
writeNum( va_list& args, char ch, char* &pt, int fNum2, int flags, int mLen, bool& isNeg );

extern "C" int evsprintf(struct nmc_stdio_channel *chan, const char* format, va_list args )
{
	int numout = 0;
	const char* f = format;
	int* fNum;
	int  fNum1;
	int  fNum2;

	int flags;

	int mLen;
	/*	step:	èñïîëüçóåòñÿ äëÿ êîíòðîëÿ êîððåêòíîñòè ñòðîêè ôîðìàòà.
	Â ðåëèç - âåðñèè ïðè âêëþ÷åííîé îïòèìèçàöèè êîä ïîðîæäàòü íå äîëæåí.
	1 - ôëàãè
	2 - øèðèíà
	3 - òî÷íîñòü
	4 - äëèíà àðãóìåíòà
	5 - ñïåöèôèêàòîð	*/
	int step;

	while (*f){
		if ( *f != '%' ){
			eputc_smart(chan, *f++);
			numout++;
			continue;
		}
		step = 0;

		fNum1 = 0;
		fNum2 = 1;
		fNum = &fNum1;

		flags =0;

		mLen = 0;

		int fill0 = 0;
		int fill1 = 0;
		int fill2 = 0;

		//! Îáðàáîòêà ïîëÿ
		f++;
		bool end =false;
		char mantiss[64];
		char* pt = mantiss;
		bool isNeg = false;
		char sgn;
		int vall;
		int i;
		while(!end){
			char ch = *f++;
			switch( ch ){
				//! Ôëàãè (flag characters)
				case '-':
					assert( step < 2 );
					step = 1;

					flags |= LEFT_JUSTIFY;
					break;
				case '+':
					assert( step < 2 );
					step = 1;

					flags |= PLUS_SIGN;
					break;
				case ' ':
					assert( step < 2 );
					step = 1;

					flags |= INS_SPACE;
					break;
				case '#':
					assert( step < 2 );
					step = 1;

					flags |= ALT_FORM;
					break;
				//! Äëèíà àðãóìåíòà (lenght modifiers)
				case 'h':
					assert( step < 5 && ( mLen==0 || mLen==-1 ) );
					step = 4;

					mLen--;
					break;
				case 'l':
					assert( step < 5 && ( mLen==0 || mLen==1 ) );
					step = 4;

					mLen++;
					break;
				case 'j':
					assert( step < 4 );
					step = 4;

					mLen = 16;
					break;
				case 'z':
					assert( step < 4 );
					step = 4;

					mLen = 17;
					break;
				case 't':
					assert( step < 4 );

					step = 4;

					mLen = 18;
					break;
				case 'L':
					assert( sizeof(double)==sizeof(long double) );
					assert( step < 4 );
					step = 4;

					mLen = 19;
					break;
				//! Ñïåöèôèêàòîðû
				case 'd':
				case 'i':
				case 'u':
				case 'o':
				case 'x':
				case 'X':
				case 'f':
				case 'F':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
				case 'c':
				case 's':
					assert( step < 5 );
					step = 5;

                    if ( ch=='s' ){
                        //  Êîãäà ïå÷àòàåì ñòðîêó, íå èñïîëüçóåì áóôåð,
                        //  èíà÷å bug497 - mantiss ìîæåò ïåðåïîëíèòüñÿ
		                pt = va_arg( args, char* );
                        char* ptt= pt;
                        vall=0;
                        if ( fNum == &fNum2 ){
                            //  precision óêàçàí
                            int i;
                            for (i=0; i<fNum2 && *ptt++; i++){
                                vall++;
                            }
                        }
                        else{
                            while (*ptt++){
                                vall++;
                            }
                        }
                    }
                    else{
					    writeNum( args, ch, pt, fNum2, flags, mLen, isNeg );
					    vall = pt - mantiss;
                    }
					if ( vall > fNum2 ){
						fNum2 = vall;
					}
					else{
						fill1 = fNum2 - vall;
					}

					if ( isNeg ){
						sgn = '-';
					}
					else if ( flags & PLUS_SIGN ){
						sgn = '+';
					}
					else if ( flags & INS_SPACE ){
						sgn = ' ';
					}
					else
						sgn = 0;

					if ( sgn )
						fNum2++;

					if ( fNum1 <= fNum2 );
					else if ( flags & LEFT_JUSTIFY ){
						fill2 = fNum1 - fNum2;
					}
					else if ( (flags & FILL_ZERO) && !(flags & PREC_IS_SET) ){
						fill1 = fNum1 - fNum2;
					}
					else{
						fill0 = fNum1 - fNum2;
					}
					//	Ñîáñòâåííî âûâîä
					while (fill0--) {
						eputc_smart(chan, ' ');
						numout++;
					}
					if (sgn){
						eputc_smart(chan, '-');
						numout++;
					}
					while (fill1--){
						eputc_smart(chan, '0');
						numout++;
					}
					if ( ch!='s' ){
						while (vall--) {	//	Ïåðåâîðà÷èâàåì
							eputc_smart(chan, *--pt);
							numout++;
						}
					}
					else {
						//pt -= vall;
						while (vall--) {	//	Íå ïåðåâîðà÷èâàåì
							eputc_smart(chan, *pt++);
							numout++;
						}
						pt= mantiss;
					}

					while (fill2--) {
						eputc_smart(chan, ' ');
						numout++;						
					}
					end =true;
					break;
				case '*':
					assert( step < 4 && *fNum==0 );
					step = step < 2 ? 2 : step;
					i = va_arg( args, int );
					if ( step >2 )
						flags |= PREC_IS_SET;
					else
						flags |= WIDTH_IS_SET;
					if ( i<0 ){
						if ( fNum==&fNum1 ){
							i = -i;
							flags |= LEFT_JUSTIFY;
						}
						else
							break;
					}
					*fNum = i;
					break;
				case '0':
					if ( step < 2 ){	//	Ýòî ôëàã
						flags |= FILL_ZERO;
						break;
					}
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					assert( step < 4 );
					step = step < 2 ? 2 : step;
					if ( step >2 )
						flags |= PREC_IS_SET;
					else
						flags |= WIDTH_IS_SET;

					*fNum = *fNum *10 + (ch - '0');
					break;
				case '.':
					assert( step < 3 );
					step = 3;

					fNum2 = 0;
					fNum = &fNum2;
					break;
				case '%':
					assert( step == 0 );
					step = 5;

					eputc_smart(chan, '%');
					numout++;
					end =true;
					break;
			default:
				//throw BadFormat();
				return -1;
			}
		}
		assert( step == 5 );
	}
	eputc_smart(chan, 0);
	numout++;
	return numout;
}

static int
writeNum( va_list& args, char ch, char* &pt, int fNum2, int flags, int mLen, bool& isNeg )
{
	char* pt_b=pt;
	if ( ch == 'd' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'x' || ch == 'X' ){
		int i;
		unsigned int ui;
		unsigned LLong ull;
		LLong ll;
		int ii;
		if( ch=='d' || ch=='i' ){
			if ( mLen > 0 ){
				if (mLen == 1)
					ll = va_arg( args, long );
				else
					ll = va_arg( args, LLong );
				if ( ll<0 ){
					isNeg = true;
					ull = -ll;
				}
				else{
					isNeg = false;
					ull = ll;
				}
			}
			else{
				if (mLen == 0)
					i = va_arg( args, int );
				else if (mLen == -2)
					i = va_arg( args, char );
				else
					i = va_arg( args, short );
				if ( i<0 ){
					isNeg = true;
					ui = -i;
				}
				else{
					isNeg = false;
					ui = i;
				}

			}
		}
		else {
			isNeg = false;
			if ( mLen==0)
				ui = va_arg( args, unsigned int );
			else if ( mLen==1)	 	//	'l'
				ull = va_arg( args, unsigned long );
			else if ( mLen==-2) 	//	'hh'
				ui = va_arg( args, unsigned char );
			else if ( mLen==-1) 	//	'h'
				ui = va_arg( args, unsigned short );
			else                	//	'll', 'j', 'z', 't'
				ull = va_arg( args, unsigned LLong );
		}
		if ( mLen <=0 ){
			if ( ch == 'd' || ch == 'i' || ch == 'u' ){
				readNumber <unsigned int, 10> ( pt, ui );
			}
			else if ( ch == 'o' ){
				readNumber <unsigned int, 8> ( pt, ui );
			}
			else if ( ch == 'x' ){
				readNumber <unsigned int, 16> ( pt, ui );
			}
			else if ( ch == 'X' ){
				readNumberX <unsigned int, 16> ( pt, ui );
			}
		}
		else{
			if ( ch == 'd' || ch == 'i' || ch == 'u' ){
				readNumber <unsigned LLong, 10> ( pt, ull );
			}
			else if ( ch == 'o' ){
				readNumber <unsigned LLong, 8> ( pt, ull );
			}
			else if ( ch == 'x' ){
				readNumber <unsigned LLong, 16> ( pt, ull );
			}
			else if ( ch == 'X' ){
				readNumberX <unsigned LLong, 16> ( pt, ull );
			}
		}
	}
	else if ( ch == 'f' || ch == 'e' || ch == 'g' || ch == 'a' ){
		char cas=0;
		if ( ch>='A' && ch<='Z' ){
			cas = 'A'-'a';
			ch -= cas;
		}
		double f = va_arg( args, double );
		LONG Mant;
		unsigned LLong Mantiss;
		unsigned LLong MantissLow;
		int Exp;
		int Sign;
		bool noDecimalPoint = ( fNum2==0 && !(flags & ALT_FORM) );
		DstrDouble( f, Mant, Exp, Sign);
		Mantiss = Mant;
		isNeg = Sign;
		if ( Mantiss==0 && Exp==-0x3ff )
			Exp = 0;
		if ( Exp==0x400 ){	//	NaN, infinity
			if (Mantiss==0){	//	inf
				*pt++ = 'i'+cas;
				*pt++ = 'n'+cas;
				*pt++ = 'f'+cas;
			}
			else{
				*pt++ = 'n'+cas;
				*pt++ = 'a'+cas;
				*pt++ = 'n'+cas;
			}
		}
		else if ( ch=='a' ){
			if ( !(flags & PREC_IS_SET) )
				fNum2 = 13;
#ifdef NIBBLE_ALIGN	//	Èíà÷å, ïåðâàÿ öèôðà âñåãäà 1
			Mantiss <<= (Exp%4);
			Exponent -= Exp%4;
#endif
			//	Âûâîäèì ýêñïîíåòó
			char e_sign;
			unsigned int Exponent;
			if ( Exp<0 ){
				Exponent = -Exp;
				e_sign = '-';
			}
			else{
				Exponent = Exp;
				e_sign = '+';
			}
			readNumber <unsigned int, 10> ( pt, Exponent );
			*pt++ = e_sign;
			*pt++ = 'p'+cas;
			//	Âûâîäèì ìàíòèññó, äðîáíàÿ ÷àñòü
			while ( fNum2-- > 13 ){
				*pt++ = '0';
			}
			MantissLow = (Mantiss << 12)>>(63-fNum2*4);
//			MantissLow = (MantissLow >> 1) + (MantissLow & 1);
			Mantiss >>= 52;
			assert(Mantiss<16);
			if (cas)
				readNumberX <unsigned LLong, 16> ( pt, MantissLow  );
			else
				readNumber <unsigned LLong, 16> ( pt, MantissLow );
			//	Âûâîäèì ìàíòèññó, ñòàðøàÿ öèôðà
			if ( !noDecimalPoint )
				*pt++ = '.';
			if (cas)
				readNumberX <unsigned LLong, 16> ( pt, Mantiss );
			else
				readNumber <unsigned LLong, 16> ( pt, Mantiss );
			*pt++ = 'x'+cas;
			*pt++ = '0';
		}
		else if ( ch=='e' || (ch=='g' && (Exp<=-4 || Exp>=fNum2 ) ) ){
		}
		else if ( ch=='f' || ch=='g' ){
			int surplus = 0;
//			bool bPoint = fNum2!=0 || (flags & ALT_FORM);
			if ( Exp > 52 ){
				surplus = Exp - 52;
				Exp = 52;
			}
			//  Âûâîäèì äðîáíóþ ÷àñòü
			if ( !(flags & PREC_IS_SET) )
				fNum2 = 6;
			char* ptx = pt;
			if ( Exp < 0 ){
				MantissLow = Mantiss << 7;
				Mantiss = 0;
				Exp++;
				while ( Exp < -3 && fNum2-- >0 ){
					//	Âûâîäèì îäèí íóëü
					MantissLow *=10;
					*ptx++ = '0';
					if ( MantissLow & 0xF000000000000000L ){
						Exp +=4;
						MantissLow >>=4;
					}
				}
				MantissLow >>= -Exp;
				Exp = 0;
			}
			else{
				MantissLow = Mantiss << (12+Exp);
				MantissLow >>=4;
				Mantiss = Mantiss >> (52-Exp);
			}
			while ( fNum2-- >0 ){
				//	Âûâîäèì îäèí öèôð
				MantissLow *=10;
				//	assert(((MantissLow & 0xF000000000000000) >> 60 )<10);	!!!!!!!!!!!!!!!!!
				if( ( ((MantissLow & 0xF000000000000000L) >> 60 )>=10 ) )
					assert(0);
				*ptx++ = '0' + ((MantissLow & 0xF000000000000000L) >> 60 );
				MantissLow &= 0x0FFFFFFFFFFFFFFFL;
			}
			//  Ïåðåâîðà÷èâàåì äðîáíóþ ÷àñòü
			{
				int i=0;
				ptx--;
				while( pt+i < ptx-i ){
					//	swap
					char c = *(pt+i);
					*(pt+i) = *(ptx-i);
					*(ptx-i) = c;
					i++;
				}
				ptx++;
			}
			pt = ptx;
			//  .
			if ( !noDecimalPoint )
				*pt++ = '.';
			//  Âûâîäèì öåëóþ ÷àñòü
			ptx = pt;
			readNumber <unsigned LLong, 10> ( pt, Mantiss );
			if ( ptx == pt )	//	0
				*pt++ = '0';
			while (surplus-- >0){	//	äëÿ ñëèøêîì áîëüøèõ ÷èñåë. ìåäëåííî.
				char* ptxx = ptx;
				int carry =0;
				while ( ptxx != pt ){	//  óìíîæàåì íà 2
					int dg = *ptxx - '0';
					assert( dg>=0 && dg <=9 );
					dg = dg*2 + carry;
					carry = dg / 10;
					dg %= 10;
					*ptxx = dg + '0';
					ptxx++;
				}
				if (carry)
					*pt++ = '1';
			}
		}
		else
			assert(0);
		//if ( ch=='g' ){
		//		if ( !(flags & ALT_FORM) && MantissLow ){
		//			while( !(MantissLow%16) )
		//				MantissLow >>= 4;
		//		}
		//}
	}
	else if ( ch == 'c' ){
		int i;
		i = va_arg( args, int );
		*pt++ = i;
	}
	else {
        assert(0);
	}
	return pt-pt_b;
}

extern "C" int eprintf(struct nmc_stdio_channel *ch, const char* format,...)
{
    va_list argptr;
    va_start(argptr, format);
    int r= evsprintf( ch, format, argptr);
    va_end( argptr );
    easynmc_send_LPINT();
    return r;
}



