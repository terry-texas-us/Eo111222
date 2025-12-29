%%
abs { return 1; }
acos { return 2; }
asin { return 3; }
atan { return 4; }
string { return 5; }
cos { return 6; }
exp { return 7; }
int { return 8; }
ln { return 9; }
log { return 10; }
sin { return 11; }
sqrt { return 12; }
tan { return 13; }
real { return 14; }
[0-9]+ { return 20; }
([0-9]+(\.[0-9]*)?|\.[0-9]+) { return 21; }
([0-9]+|([0-9]+\.[0-9]*|\.[0-9]+))(\'([0-9]+|([0-9]+\.[0-9]*|\.[0-9]+))*\'|\"|mm|cm|dm|m|km) { return 22; }
\"([\x20-\x21\x23-\x7e]|\"\")*\" { return 24; }
[a-zA-Z][a-zA-Z0-9_]* { return 25; }
"**" { return 26; }
"*" { return 27; }
"/" { return 28; }
"+" { return 29; }
"-" { return 30; }
"==" { return 31; }
"!=" { return 32; }
">" { return 33; }
">=" { return 34; }
"<" { return 35; }
"<=" { return 36; }
"&" { return 37; }
"|" { return 38; }
"!" { return 39; }
"(" { return 40; }
")" { return 41; }
"," { return 42; }
[ \t\n\r]+ { /* ignore whitespace */ }
. { /* error or unexpected char */ }
%%