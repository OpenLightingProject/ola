%{
#include "tools/ola_trigger/ConfigCommon.h"
#include "tools/ola_trigger/config.tab.h"

#define YY_NO_INPUT

void count();
%}

%option nounput
%option yylineno

alpha        [a-zA-Z_]
alphanumeric [a-zA-Z0-9_\-\.]
digit        [0-9]
word         {alpha}{alphanumeric}*
int          {digit}+
whitespace   [ \t]

%%

{int}             { count();
                    yylval.int_val = atoi(yytext);
                    return INTEGER_LITERAL;
                  }
{word}            { count();
                    yylval.str_val = new std::string(yytext);
                    return WORD_LITERAL;
                  }
\"(\\.|[^\\"])*\" { count();
                    std::string *s = new std::string(++yytext);
                    s->erase(s->size() - 1, 1);
                    yylval.str_val = s;
                    return QUOTED_VALUE; }
\'(\\.|[^\\'])*\' { count();
                    std::string *s = new std::string(++yytext);
                    s->erase(s->size() - 1, 1);
                    yylval.str_val = s;
                    return QUOTED_VALUE; }
^#[^\n]*             {}
{whitespace}+#[^\n]* {}
{whitespace}+     { count(); return WHITESPACE; }
\n                { count(); yylineno++; return NEW_LINE; }
[%=,`\-/+]         { count(); return (int) yytext[0]; }

%%

int column = 0;

void count() {
  for (int i = 0; yytext[i] != '\0'; i++)
    if (yytext[i] == '\n')
      column = 0;
    else if (yytext[i] == '\t')
      // tabs are evil, we should make it a syntax error
      column += 8 - (column % 8);
    else
      column++;
}

// only ever parse one file
int yywrap() {
  return 1;
}
