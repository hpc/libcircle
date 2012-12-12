%{
/* Lexer for C and a bit of C++ (what I need).  For example, "class" is
 * recognized as a STRUCT token, unless PURE_C is defined in options.h.
 * Most of this is from http://www.lysator.liu.se/c/ANSI-C-grammar-l.html
 */

#include <stdio.h>
#include "options.h"
#include "tokens.h"

int column = 0, line = 1;

int yywrap (void);
void count();
%}

D                       [0-9]
L                       [a-zA-Z_]
H                       [a-fA-F0-9]
E                       [Ee][+-]?{D}+
FS                      (f|F|l|L)
IS                      (u|U|l|L)*

%%

^"#".*                  { count(); }

"/*"([^*]*\*)"/"        { count(); }

"auto"                  { count(); return(AUTO); }
"break"                 { count(); return(BREAK); }
"case"                  { count(); return(CASE); }
"char"                  { count(); return(CHAR); }
"const"                 { count(); return(CONST); }
"continue"              { count(); return(CONTINUE); }
"default"               { count(); return(DEFAULT); }
"do"                    { count(); return(DO); }
"double"                { count(); return(DOUBLE); }
"else"                  { count(); return(ELSE); }
"enum"                  { count(); return(ENUM); }
"extern"                { count(); return(EXTERN); }
"float"                 { count(); return(FLOAT); }
"for"                   { count(); return(FOR); }
"goto"                  { count(); return(GOTO); }
"if"                    { count(); return(IF); }
"int"                   { count(); return(INT); }
"long"                  { count(); return(LONG); }
"register"              { count(); return(REGISTER); }
"return"                { count(); return(RETURN); }
"short"                 { count(); return(SHORT); }
"signed"                { count(); return(SIGNED); }
"sizeof"                { count(); return(SIZEOF); }
"static"                { count(); return(STATIC); }
"struct"                { count(); return(STRUCT); }
"switch"                { count(); return(SWITCH); }
"typedef"               { count(); return(TYPEDEF); }
"union"                 { count(); return(UNION); }
"unsigned"              { count(); return(UNSIGNED); }
"void"                  { count(); return(VOID); }
"volatile"              { count(); return(VOLATILE); }
"while"                 { count(); return(WHILE); }

"class"                 { count();
#                         ifndef PURE_C
                              return STRUCT;
#                         else
                              return IDENTIFIER;
#                         endif
                        }

{L}({L}|{D})*           { count(); return(IDENTIFIER); }

0[xX]{H}+{IS}?          { count(); return(CONSTANT); }
0{D}+{IS}?              { count(); return(CONSTANT); }
{D}+{IS}?               { count(); return(CONSTANT); }
'(\\.|[^\\'])+'         { count(); return(CONSTANT); }

{D}+{E}{FS}?            { count(); return(CONSTANT); }
{D}*"."{D}+({E})?{FS}?  { count(); return(CONSTANT); }
{D}+"."{D}*({E})?{FS}?  { count(); return(CONSTANT); }

\"(\\.|[^\\"])*\"       { count(); return(STRING_LITERAL); }

">>="                   { count(); return(RIGHT_ASSIGN); }
"<<="                   { count(); return(LEFT_ASSIGN); }
"+="                    { count(); return(ADD_ASSIGN); }
"-="                    { count(); return(SUB_ASSIGN); }
"*="                    { count(); return(MUL_ASSIGN); }
"/="                    { count(); return(DIV_ASSIGN); }
"%="                    { count(); return(MOD_ASSIGN); }
"&="                    { count(); return(AND_ASSIGN); }
"^="                    { count(); return(XOR_ASSIGN); }
"|="                    { count(); return(OR_ASSIGN); }
">>"                    { count(); return(RIGHT_OP); }
"<<"                    { count(); return(LEFT_OP); }
"++"                    { count(); return(INC_OP); }
"--"                    { count(); return(DEC_OP); }
"->"                    { count(); return(PTR_OP); }
"&&"                    { count(); return(AND_OP); }
"||"                    { count(); return(OR_OP); }
"<="                    { count(); return(LE_OP); }
">="                    { count(); return(GE_OP); }
"=="                    { count(); return(EQ_OP); }
"!="                    { count(); return(NE_OP); }
";"                     { count(); return(SEMI); }
"{"                     { count(); return(LBRACE); }
"}"                     { count(); return(RBRACE); }
","                     { count(); return(COMMA); }
":"                     { count(); return(COLON); }
"="                     { count(); return(EQUALS); }
"("                     { count(); return(LPAREN); }
")"                     { count(); return(RPAREN); }
"["                     { count(); return(LBRACKET); }
"]"                     { count(); return(RBRACKET); }
"."                     { count(); return(DOT); }
"&"                     { count(); return(BIT_AND); }
"!"                     { count(); return(NOT_OP); }
"~"                     { count(); return(BIT_NOT); }
"-"                     { count(); return(SUB_OP); }
"+"                     { count(); return(ADD_OP); }
"*"                     { count(); return(MULT_OP); }
"/"                     { count(); return(DIV_OP); }
"%"                     { count(); return(MOD_OP); }
"<"                     { count(); return(LT_OP); }
">"                     { count(); return(GT_OP); }
"^"                     { count(); return(BIT_XOR); }
"|"                     { count(); return(BIT_OR); }
"?"                     { count(); return(QUESTION); }

[ \t\v\n\r\f]+          { count(); }
.                       { count(); /* ignore bad characters */ }

%%

int yywrap (void)
{
   return(1);
}

#ifndef YYLMAX
/* GNU flex without -l option or %array directive */
#error - You current must use flex -l or the %array directive.
#endif

unsigned char last_yytext[YYLMAX] = "";
void count (void)
{
   int i;

   for (i = 0; yytext[i] != '\0'; i++)
      if (yytext[i] == '\n')
      {
         line++;
         column = 0;
      }
      else if (yytext[i] == '\t')
         column += 8 - (column % 8);
      else
         column++;

   printf ("%s", last_yytext);
   strcpy (last_yytext, yytext);
}

void flush_out (void)
{
   printf ("%s", last_yytext);
   last_yytext[0] = '\0';
}

