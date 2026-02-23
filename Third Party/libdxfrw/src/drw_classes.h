#pragma once

#include "drw_base.h"

class dxfReader;
class dxfWriter;
class dwgBuffer;

//! Class to handle classes entries
/*!
*  Class to handle classes table entries
*  TODO: verify the dxf read/write part
*  @author Rallaz
*/
class DRW_Class {
public:
  DRW_Class() {
  }
  ~DRW_Class() {
  }

  void parseCode(int code, dxfReader* reader);
  void write(dxfWriter* writer, DRW::Version ver) const;

  UTF8STRING recName;      /*!< record name, code 1 */
  UTF8STRING className;    /*!< C++ class name, code 2 */
  UTF8STRING appName;      /*!< app name, code 3 */
  int proxyFlag{ 0 };           /*!< Proxy capabilities flag, code 90 */
  int instanceCount{ 0 };       /*!< number of instances for a custom class, code 91*/
  int wasaProxyFlag{ 0 };       /*!< proxy flag (app loaded on save), code 280 */
  int entityFlag{ 0 };          /*!< entity flag, code 281 (0 object, 1 entity)*/
};