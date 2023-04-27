// Scintilla source code edit control
/** @file LexLua.cxx
 ** Lexer for Lua language.
 **
 ** Written by Paul Winwood.
 ** Folder by Alexey Yutkin.
 ** Modified by Marcos E. Wurzius & Philippe Lhoste
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static void strUpper(char* dstr, const char* sstr)
{
  if (!sstr || sstr[0] == 0) return;
  for (; *sstr; sstr++, dstr++)
    *dstr = (char)toupper(*sstr);
  *dstr = 0;
}

static void ColouriseLedDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
	WordList &keywords8 = *keywordlists[7];

	// Accepts accented characters
	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases. [pP] is for hex floats.
	CharacterSet setNumber(CharacterSet::setDigits, ".-+abcdefpABCDEFP");
	CharacterSet setExponent(CharacterSet::setNone, "eEpP");
	CharacterSet setLedOperator(CharacterSet::setNone, "()=[]");
	CharacterSet setEscapeSkip(CharacterSet::setNone, "\"'\\");

	Sci_Position currentLine = styler.GetLine(startPos);
	// Initialize long string [[ ... ]] or block comment --[[ ... ]] nesting level,
	// if we are inside such a string. Block comment was introduced in Lua 5.0,
	// blocks with separators [=[ ... ]=] in Lua 5.1.
	// Continuation of a string (\z whitespace escaping) is controlled by stringWs.
	int nestLevel = 0;
	int sepCount = 0;
	int stringWs = 0;
	if (initStyle == SCE_LED_STRING) {
		int lineState = styler.GetLineState(currentLine - 1);
		nestLevel = lineState >> 9;
		sepCount = lineState & 0xFF;
		stringWs = lineState & 0x100;
	}

	// Do not leak onto next line
	if (initStyle == SCE_LED_COMMENTLINE) {
		initStyle = SCE_LED_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);
	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			switch (sc.state) {
			case SCE_LED_STRING:
			case SCE_LED_CHARACTER:
				// Inside a literal string, block comment or string, we set the line state
				styler.SetLineState(currentLine, (nestLevel << 9) | stringWs | sepCount);
				break;
			default:
				// Reset the line state
				styler.SetLineState(currentLine, 0);
				break;
			}
		}
		if (sc.atLineStart && (sc.state == SCE_LED_STRING)) {
			// Prevent SCE_LED_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_LED_STRING);
		}

		// Determine if the current state should terminate.
    if (sc.state == SCE_LED_OPERATOR) {
      sc.SetState(SCE_LED_DEFAULT);
    }
    else if (sc.state == SCE_LED_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eEpP non-sign non-hexdigit char
			if (!setNumber.Contains(sc.ch)) {
				sc.SetState(SCE_LED_DEFAULT);
			} else if (sc.ch == '-' || sc.ch == '+') {
				if (!setExponent.Contains(sc.chPrev))
					sc.SetState(SCE_LED_DEFAULT);
			}
		} else if (sc.state == SCE_LED_IDENTIFIER) {
			// IUP  if (!(setWord.Contains(sc.ch) || sc.ch == '.') || sc.Match('.', '.')) {
			if (!(setWord.Contains(sc.ch))) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
        strUpper(s, s);
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_LED_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_LED_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_LED_WORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_LED_WORD4);
				} else if (keywords5.InList(s)) {
					sc.ChangeState(SCE_LED_WORD5);
				} else if (keywords6.InList(s)) {
					sc.ChangeState(SCE_LED_WORD6);
				} else if (keywords7.InList(s)) {
					sc.ChangeState(SCE_LED_WORD7);
				} else if (keywords8.InList(s)) {
					sc.ChangeState(SCE_LED_WORD8);
				}
				sc.SetState(SCE_LED_DEFAULT);
			}
    }
    else if (sc.state == SCE_LED_STRING) {
      if (sc.ch == '\"') {
        sc.ForwardSetState(SCE_LED_DEFAULT);
      }
    }
    else if (sc.state == SCE_LED_COMMENTLINE) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_LED_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_LED_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_LED_NUMBER);
			} else if (setWordStart.Contains(sc.ch)) {
				sc.SetState(SCE_LED_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_LED_STRING);
				stringWs = 0;
      } else if (sc.Match('#')) {
				sc.SetState(SCE_LED_COMMENTLINE);
				sc.Forward();
			} else if (setLedOperator.Contains(sc.ch)) {
				sc.SetState(SCE_LED_OPERATOR);
			}
		}
	}

	if (setWord.Contains(sc.chPrev) || sc.chPrev == '.') {
		char s[100];
		sc.GetCurrent(s, sizeof(s));
		if (keywords.InList(s)) {
			sc.ChangeState(SCE_LED_WORD);
		} else if (keywords2.InList(s)) {
			sc.ChangeState(SCE_LED_WORD2);
		} else if (keywords3.InList(s)) {
			sc.ChangeState(SCE_LED_WORD3);
		} else if (keywords4.InList(s)) {
			sc.ChangeState(SCE_LED_WORD4);
		} else if (keywords5.InList(s)) {
			sc.ChangeState(SCE_LED_WORD5);
		} else if (keywords6.InList(s)) {
			sc.ChangeState(SCE_LED_WORD6);
		} else if (keywords7.InList(s)) {
			sc.ChangeState(SCE_LED_WORD7);
		} else if (keywords8.InList(s)) {
			sc.ChangeState(SCE_LED_WORD8);
		}
	}

	sc.Complete();
}

static void FoldLedDoc(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
	Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);

	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
    if (style == SCE_LED_OPERATOR) {
			if (ch == '[' || ch == '(') {
				levelCurrent++;
			} else if (ch == ']' || ch == ')') {
				levelCurrent--;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const ledWordListDesc[] = {
	"Keywords",
	"Basic functions",
	"String, (table) & math functions",
	"(coroutines), I/O & system facilities",
	"user1",
	"user2",
	"user3",
	"user4",
	0
};

LexerModule lmLed(SCLEX_LED, ColouriseLedDoc, "led", FoldLedDoc, ledWordListDesc);
