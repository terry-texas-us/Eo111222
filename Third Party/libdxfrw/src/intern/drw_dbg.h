#pragma once

#include <ios>
#include <string>

#define DRW_DBGSL(a) DRW_dbg::getInstance()->setLevel(a)
#define DRW_DBGGL DRW_dbg::getInstance()->getLevel()
#define DRW_DBG(a) DRW_dbg::getInstance()->print(a)
#define DRW_DBGH(a) DRW_dbg::getInstance()->printH(a)
#define DRW_DBGB(a) DRW_dbg::getInstance()->printB(a)
#define DRW_DBGHL(a, b, c) DRW_dbg::getInstance()->printHL(a, b ,c)
#define DRW_DBGPT(a, b, c) DRW_dbg::getInstance()->printPT(a, b, c)

class print_none;

class DRW_dbg {
public:
  enum LEVEL {
    none,
    debug
  };
  void setLevel(LEVEL lvl);
  LEVEL getLevel() const;
  static DRW_dbg* getInstance();
  void print(std::string s);
  void print(int i);
  void print(unsigned int i);
  void print(long long int i);
  void print(long unsigned int i);
  void print(long long unsigned int i);
  void print(double d);
  void printH(long long int i);
  void printB(int i);
  void printHL(int c, int s, int h);
  void printPT(double x, double y, double z);

private:
  DRW_dbg();
  static DRW_dbg* instance;
  LEVEL level;
  std::ios_base::fmtflags flags;
  print_none* prClass;
};
