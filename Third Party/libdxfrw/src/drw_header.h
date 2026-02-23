#pragma once

#include <map>
#include <string>

#include "drw_base.h"

class dxfReader;
class dxfWriter;
class dwgBuffer;

/** Class to handle header vars, to read iterate over "std::map vars"
 *  to write add a DRW_Variant* into "std::map vars" (do not delete it, are cleared in dtor)
 *  or use add* helper functions.
 */
class DRW_Header {
  friend class dxfRW;

 public:
  DRW_Header();
  ~DRW_Header() { clearVars(); }

  DRW_Header(const DRW_Header& h) {
    this->version = h.version;
    this->comments = h.comments;
    for (std::map<std::string, DRW_Variant*>::const_iterator it = h.vars.begin(); it != h.vars.end(); ++it) {
      this->vars[it->first] = new DRW_Variant(*(it->second));
    }
    this->curr = nullptr;
    this->vportCtrl = h.vportCtrl;
  }
  DRW_Header& operator=(const DRW_Header& h) {
    if (this != &h) {
      clearVars();
      this->version = h.version;
      this->comments = h.comments;
      for (std::map<std::string, DRW_Variant*>::const_iterator it = h.vars.begin(); it != h.vars.end(); ++it) {
        this->vars[it->first] = new DRW_Variant(*(it->second));
      }
    }
    return *this;
  }

  void addDouble(std::string key, double value, int code);
  void addInt(std::string key, int value, int code);
  void addStr(std::string key, std::string value, int code);
  void addCoord(std::string key, DRW_Coord value, int code);
  std::string getComments() const { return comments; }

  /** @brief Writes the header variables to the given dxfWriter object in the specified DXF version format.
   *  @param writer Pointer to dxfWriter object
   *  @param ver DXF version to write
   */
  void write(dxfWriter* writer, DRW::Version ver);
  void addComment(std::string c);

 protected:
  void parseCode(int code, dxfReader* reader);

 private:
  bool getDouble(std::string key, double* varDouble);
  bool getInt(std::string key, int* varInt);
  bool getStr(std::string key, std::string* varStr);
  bool getCoord(std::string key, DRW_Coord* varStr);
  void clearVars() {
    for (std::map<std::string, DRW_Variant*>::iterator it = vars.begin(); it != vars.end(); ++it) delete it->second;

    vars.clear();
  }

 public:
  std::map<std::string, DRW_Variant*> vars;

 private:
  std::string comments;
  std::string name;
  DRW_Variant* curr{};
  int version;  //to use on read

  duint32 linetypeCtrl{};
  duint32 layerCtrl{};
  duint32 styleCtrl{};
  duint32 dimstyleCtrl{};
  duint32 appidCtrl{};
  duint32 blockCtrl{};
  duint32 viewCtrl{};
  duint32 ucsCtrl{};
  duint32 vportCtrl;
  duint32 vpEntHeaderCtrl{};
};
