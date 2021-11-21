%{
#include <string.h>
#include "y.h"
#include "ast.h"
%}

%option noyywrap
%option yylineno

%%
\/\/.*\n                            // Line comment.
"/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"   // Block comment.
if                                  { return IF; }
else                                { return ELSE; }
do                                  { return DO; }
while                               { return WHILE; }
for                                 { return FOR; }
return                              { return RETURN; }
\(                                  { return LP; }
\)                                  { return RP; }
\[                                  { return LB; }
\]                                  { return RB; }
\{                                  { return LCB; }
\}                                  { return RCB; }
,                                   { return COMMA; }
;                                   { return SEMICOLON; }
void                                { yylval.node = new_node("void", 0); return VOID; }
int                                 { yylval.node = new_node("int", 0); return INT; }
float                               { yylval.node = new_node("float", 0); return FLOAT; }
double                              { yylval.node = new_node("double", 0); return DOUBLE; }
char                                { yylval.node = new_node("char", 0); return CHAR; }
\+                                  { yylval.node = new_node("+", 0); return ADD; }
\-                                  { yylval.node = new_node("-", 0); return SUB; }
\*                                  { yylval.node = new_node("*", 0); return MUL; }
\/                                  { yylval.node = new_node("/", 0); return DIV; }
%                                   { yylval.node = new_node("%", 0); return MOD; }
\.                                  { yylval.node = new_node(".", 0); return DOT; }
\-\>                                { yylval.node = new_node("->", 0); return ARROW; }
\+\+                                { yylval.node = new_node("++", 0); return AUTO_INCR; }
\-\-                                { yylval.node = new_node("--", 0); return AUTO_DECR; }
==                                  { yylval.node = new_node("==", 0); return EQ; }
\>                                  { yylval.node = new_node(">", 0); return GT; }
\<                                  { yylval.node = new_node("<", 0); return LT; }
\>=                                 { yylval.node = new_node(">=", 0); return GE; }
\<=                                 { yylval.node = new_node("<=", 0); return LE; }
!=                                  { yylval.node = new_node("!=", 0); return NE; }
&&                                  { yylval.node = new_node("&&", 0); return AND; }
\|\|                                { yylval.node = new_node("||", 0); return OR; }
!                                   { yylval.node = new_node("!", 0); return NOT; }
&                                   { yylval.node = new_node("&", 0); return REF; }
=                                   { yylval.node = new_node("=", 0); return ASSIGN; }
\+=                                 { yylval.node = new_node("+=", 0); return ADD_ASSIGN; }
\-=                                 { yylval.node = new_node("-=", 0); return SUB_ASSIGN; }
\*=                                 { yylval.node = new_node("*=", 0); return MUL_ASSIGN; }
\/=                                 { yylval.node = new_node("/=", 0); return DIV_ASSIGN; }
\%=                                 { yylval.node = new_node("%=", 0); return MOD_ASSIGN; }
[0-9]+\.[0-9]+                      { yylval.node = new_value("const: double", strdup(yytext)); return DNUM; }
[0-9]+\.[0-9]+f                     { yylval.node = new_value("const: float", strdup(yytext)); return FNUM; }
[0-9]+                              { yylval.node = new_value("const: int", strdup(yytext)); return INUM; }
struct[ \t\n]+[a-zA-Z][a-zA-Z0-9_]* { yylval.node = new_node(strdup(yytext), 0); return STRUCT; }
[a-zA-Z][a-zA-Z0-9_]*               { yylval.node = new_value("id", strdup(yytext)); return ID; }
'.'                                 { yylval.node = new_value("const: character", strdup(yytext)); return CHARACTER;}
\"(\\.|[^"\\])*\"                   { yylval.node = new_value("const: string", strdup(yytext)); return STRING; }
[ \t\n]                             // Ignore whitespace.
%%
