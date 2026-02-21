%option noyywrap

sign    [+-]?
digit   [0-9]
integer  {digit}+
signed_integer {sign}{integer}
fraction {integer}\/{integer}
int_dot  {digit}+\.{digit}*
dot_int \.{digit}+
letter  [a-zA-Z_]
metric_units (mm|cm|dm|m|km)

%%

abs    { return 1; }
acos   { return 2; }
asin   { return 3; }
atan   { return 4; }
string { return 5; }
cos    { return 6; }
exp    { return 7; }
int    { return 8; }
ln     { return 9; }
log    { return 10; }
sin    { return 11; }
sqrt   { return 12; }
tan    { return 13; }
real   { return 14; }

/* Architectural units (priority 1) */
{signed_integer}\'{integer}-{fraction}\"? { return 19; }
{signed_integer}\'{integer}\"?            { return 19; }

/* Engineering units (priority 2) */
{signed_integer}\'{int_dot}\"?            { return 20; }
{signed_integer}\'{dot_int}\"?            { return 20; }

/* Simple units (priority 3) */
{signed_integer}-{fraction}(\'|\"|{metric_units}) { return 21; }
{sign}{fraction}(\'|\"|{metric_units})    { return 21; }
{signed_integer}(\'|\"|{metric_units})    { return 21; }
{sign}{int_dot}(\'|\"|{metric_units})     { return 21; }
{sign}{dot_int}(\'|\"|{metric_units})     { return 21; }

{sign}{digit}*"."{digit}+                 { return 18; }
{sign}{digit}+"."{digit}*                 { return 18; }
{digit}+                                  { return 17; }

\"([^\\\"]|\\.)*\"                        { return 22; }
{letter}({letter}|{digit})*               { return 23; }

"**" { return 24; }
"*"  { return 25; }
"/"  { return 26; }
"+"  { return 27; }
"-"  { return 28; }
"("  { return 29; }
")"  { return 30; }

[ \t\n\r]+ { /* ignore whitespace */ }
. { /* error or unexpected char */ }

%%