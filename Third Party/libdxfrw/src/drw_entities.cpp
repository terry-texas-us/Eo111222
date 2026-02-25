#include <cmath>
#include <list>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dxfreader.h"

//! Calculate arbitary axis
/*!
*   Calculate arbitary axis for apply extrusions
*/
void DRW_Entity::calculateAxis(DRW_Coord extPoint) {
  //Follow the arbitrary DXF definitions for extrusion axes.
  if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625) {
    //If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
    //The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
    //Factoring in the fixed values for Wy gives N.z,0,-N.x
    extAxisX.x = extPoint.z;
    extAxisX.y = 0;
    extAxisX.z = -extPoint.x;
  } else {
    //Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
    //The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
    //Factoring in the fixed values for Wz gives -N.y,N.x,0.
    extAxisX.x = -extPoint.y;
    extAxisX.y = extPoint.x;
    extAxisX.z = 0;
  }

  extAxisX.unitize();

  //Ay = N x Ax
  extAxisY.x = (extPoint.y * extAxisX.z) - (extAxisX.y * extPoint.z);
  extAxisY.y = (extPoint.z * extAxisX.x) - (extAxisX.z * extPoint.x);
  extAxisY.z = (extPoint.x * extAxisX.y) - (extAxisX.x * extPoint.y);

  extAxisY.unitize();
}

//! Extrude a point using arbitary axis
/*!
*   apply extrusion in a point using arbitary axis (previous calculated)
*/
void DRW_Entity::extrudePoint(DRW_Coord extPoint, DRW_Coord* point) const {
  double px, py, pz;
  px = (extAxisX.x * point->x) + (extAxisY.x * point->y) + (extPoint.x * point->z);
  py = (extAxisX.y * point->x) + (extAxisY.y * point->y) + (extPoint.y * point->z);
  pz = (extAxisX.z * point->x) + (extAxisY.z * point->y) + (extPoint.z * point->z);

  point->x = px;
  point->y = py;
  point->z = pz;
}

bool DRW_Entity::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 5:
      handle = reader->GetHandleString();
      break;
    case 330:
      parentHandle = reader->GetHandleString();
      break;
    case 8:
      layer = reader->GetUtf8String();
      break;
    case 6:
      lineType = reader->GetUtf8String();
      break;
    case 62:
      color = reader->GetInt32();
      break;
    case 370:
      lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->GetInt32());
      break;
    case 48:
      ltypeScale = reader->GetDouble();
      break;
    case 60:
      visible = reader->GetBool();
      break;
    case 420:
      color24 = reader->GetInt32();
      break;
    case 430:
      colorName = reader->GetString();
      break;
    case 67:
      space = static_cast<DRW::Space>(reader->GetInt32());
      break;
    case 102:
      parseDxfGroups(code, reader);
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      extData.push_back(new DRW_Variant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      curr = new DRW_Variant(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
      extData.push_back(curr);
      break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (curr) curr->setCoordY(reader->GetDouble());
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (curr) curr->setCoordZ(reader->GetDouble());
      curr = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      extData.push_back(new DRW_Variant(code, reader->GetDouble()));
      break;
    case 1070:
    case 1071:
      extData.push_back(new DRW_Variant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
  return true;
}

bool DRW_Entity::parseDxfGroups(int code, dxfReader* reader) {
  std::list<DRW_Variant> ls;
  DRW_Variant curr;
  int nc;
  std::string appName = reader->GetString();
  if (!appName.empty() && appName.at(0) == '{') {
    curr.addString(code, appName.substr(1, (int)appName.size() - 1));
    ls.push_back(curr);
    while (code != 102 && appName.at(0) == '}') {
      reader->ReadRec(&nc);
      if (code == 330 || code == 360) {
        curr.addInt(code, reader->GetHandleString());
      } else {
        switch (reader->GetType()) {
          case dxfReader::Type::String:
            curr.addString(code, reader->GetString());
            break;
          case dxfReader::Type::Int32:
          case dxfReader::Type::Int64:
            curr.addInt(code, reader->GetInt32());
            break;
          case dxfReader::Type::Double:
            curr.addDouble(code, reader->GetDouble());
            break;
          case dxfReader::Type::Bool:
            curr.addInt(code, reader->GetInt32());
            break;
          default:
            break;
        }
      }
      ls.push_back(curr);
    }
  }

  appData.push_back(ls);
  return true;
}

void DRW_Point::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 10:
      basePoint.x = reader->GetDouble();
      break;
    case 20:
      basePoint.y = reader->GetDouble();
      break;
    case 30:
      basePoint.z = reader->GetDouble();
      break;
    case 39:
      thickness = reader->GetDouble();
      break;
    case 210:
      haveExtrusion = true;
      extPoint.x = reader->GetDouble();
      break;
    case 220:
      extPoint.y = reader->GetDouble();
      break;
    case 230:
      extPoint.z = reader->GetDouble();
      break;
    default:
      DRW_Entity::parseCode(code, reader);
      break;
  }
}

void DRW_Line::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 11:
      secPoint.x = reader->GetDouble();
      break;
    case 21:
      secPoint.y = reader->GetDouble();
      break;
    case 31:
      secPoint.z = reader->GetDouble();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Circle::applyExtrusion() {
  if (haveExtrusion) {
    //NOTE: Commenting these out causes the the arcs being tested to be located
    //on the other side of the y axis (all x dimensions are negated).
    calculateAxis(extPoint);
    extrudePoint(extPoint, &basePoint);
  }
}

void DRW_Circle::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      radious = reader->GetDouble();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Arc::applyExtrusion() {
  DRW_Circle::applyExtrusion();

  if (haveExtrusion) {
    // If the extrusion vector has a z value less than 0, the angles for the arc
    // have to be mirrored since DXF files use the right hand rule.
    // Note that the following code only handles the special case where there is a 2D
    // drawing with the z axis heading into the paper (or rather screen). An arbitrary
    // extrusion axis (with x and y values greater than 1/64) may still have issues.
    if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625 && extPoint.z < 0.0) {
      staangle = DRW::Pi - staangle;
      endangle = DRW::Pi - endangle;

      double temp = staangle;
      staangle = endangle;
      endangle = temp;
    }
  }
}

void DRW_Arc::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 50:
      staangle = reader->GetDouble() / DRW::ARAD;
      break;
    case 51:
      endangle = reader->GetDouble() / DRW::ARAD;
      break;
    default:
      DRW_Circle::parseCode(code, reader);
      break;
  }
}

void DRW_Ellipse::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      ratio = reader->GetDouble();
      break;
    case 41:
      staparam = reader->GetDouble();
      break;
    case 42:
      endparam = reader->GetDouble();
      break;
    default:
      DRW_Line::parseCode(code, reader);
      break;
  }
}

void DRW_Ellipse::applyExtrusion() {
  if (haveExtrusion) {
    calculateAxis(extPoint);
    extrudePoint(extPoint, &secPoint);
    double intialparam = staparam;
    if (extPoint.z < 0.) {
      staparam = DRW::TwoPi - endparam;
      endparam = DRW::TwoPi - intialparam;
    }
  }
}

//if ratio > 1 minor axis are greather than major axis, correct it
void DRW_Ellipse::correctAxis() {
  bool complete = false;
  if (staparam == endparam) {
    staparam = 0.0;
    endparam = DRW::TwoPi;
    complete = true;
  }
  if (ratio > 1) {
    if (fabs(endparam - staparam - DRW::TwoPi) < 1.0e-10) complete = true;
    double incX = secPoint.x;
    secPoint.x = -(secPoint.y * ratio);
    secPoint.y = incX * ratio;
    ratio = 1 / ratio;
    if (!complete) {
      if (staparam < DRW::HalfPi) staparam += DRW::TwoPi;
      if (endparam < DRW::HalfPi) endparam += DRW::TwoPi;
      endparam -= DRW::HalfPi;
      staparam -= DRW::HalfPi;
    }
  }
}

//parts are the number of vertex to split polyline, default 128
void DRW_Ellipse::toPolyline(DRW_Polyline* pol, int parts) {
  double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
  double cosCurr, sinCurr;
  radMajor = sqrt(secPoint.x * secPoint.x + secPoint.y * secPoint.y);
  radMinor = radMajor * ratio;
  //calculate sin & cos of included angle
  incAngle = atan2(secPoint.y, secPoint.x);
  cosRot = cos(incAngle);
  sinRot = sin(incAngle);
  incAngle = DRW::TwoPi / parts;
  curAngle = staparam;
  int i = static_cast<int>(curAngle / incAngle);
  do {
    if (curAngle > endparam) {
      curAngle = endparam;
      i = parts + 2;
    }
    cosCurr = cos(curAngle);
    sinCurr = sin(curAngle);
    double x = basePoint.x + (cosCurr * cosRot * radMajor) - (sinCurr * sinRot * radMinor);
    double y = basePoint.y + (cosCurr * sinRot * radMajor) + (sinCurr * cosRot * radMinor);
    pol->addVertex(DRW_Vertex(x, y, 0.0, 0.0));
    curAngle = (++i) * incAngle;
  } while (i < parts);
  if (fabs(endparam - staparam - DRW::TwoPi) < 1.0e-10) { pol->flags = 1; }
  pol->layer = this->layer;
  pol->lineType = this->lineType;
  pol->color = this->color;
  pol->lWeight = this->lWeight;
  pol->extPoint = this->extPoint;
}

void DRW_Trace::applyExtrusion() {
  if (haveExtrusion) {
    calculateAxis(extPoint);
    extrudePoint(extPoint, &basePoint);
    extrudePoint(extPoint, &secPoint);
    extrudePoint(extPoint, &thirdPoint);
    extrudePoint(extPoint, &fourPoint);
  }
}

void DRW_Trace::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 12:
      thirdPoint.x = reader->GetDouble();
      break;
    case 22:
      thirdPoint.y = reader->GetDouble();
      break;
    case 32:
      thirdPoint.z = reader->GetDouble();
      break;
    case 13:
      fourPoint.x = reader->GetDouble();
      break;
    case 23:
      fourPoint.y = reader->GetDouble();
      break;
    case 33:
      fourPoint.z = reader->GetDouble();
      break;
    default:
      DRW_Line::parseCode(code, reader);
      break;
  }
}

void DRW_Solid::parseCode(int code, dxfReader* reader) { DRW_Trace::parseCode(code, reader); }

void DRW_3Dface::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      invisibleflag = reader->GetInt32();
      break;
    default:
      DRW_Trace::parseCode(code, reader);
      break;
  }
}

void DRW_Block::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Insert::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 41:
      xscale = reader->GetDouble();
      break;
    case 42:
      yscale = reader->GetDouble();
      break;
    case 43:
      zscale = reader->GetDouble();
      break;
    case 50:
      angle = reader->GetDouble();
      angle = angle / DRW::ARAD;  //convert to radian
      break;
    case 70:
      colcount = reader->GetInt32();
      break;
    case 71:
      rowcount = reader->GetInt32();
      break;
    case 44:
      colspace = reader->GetDouble();
      break;
    case 45:
      rowspace = reader->GetDouble();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_LWPolyline::applyExtrusion() {
  if (haveExtrusion) {
    calculateAxis(extPoint);
    for (unsigned int i = 0; i < vertlist.size(); i++) {
      auto* vert = vertlist.at(i);
      DRW_Coord v(vert->x, vert->y, elevation);
      extrudePoint(extPoint, &v);
      vert->x = v.x;
      vert->y = v.y;
    }
  }
}

void DRW_LWPolyline::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 10: {
      vertex = new DRW_Vertex2D();
      vertlist.push_back(vertex);
      vertex->x = reader->GetDouble();
      break;
    }
    case 20:
      if (vertex != nullptr) vertex->y = reader->GetDouble();
      break;
    case 40:
      if (vertex != nullptr) vertex->stawidth = reader->GetDouble();
      break;
    case 41:
      if (vertex != nullptr) vertex->endwidth = reader->GetDouble();
      break;
    case 42:
      if (vertex != nullptr) vertex->bulge = reader->GetDouble();
      break;
    case 38:
      elevation = reader->GetDouble();
      break;
    case 39:
      thickness = reader->GetDouble();
      break;
    case 43:
      width = reader->GetDouble();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    case 90:
      vertexnum = reader->GetInt32();
      vertlist.reserve(vertexnum);
      break;
    case 210:
      haveExtrusion = true;
      extPoint.x = reader->GetDouble();
      break;
    case 220:
      extPoint.y = reader->GetDouble();
      break;
    case 230:
      extPoint.z = reader->GetDouble();
      break;
    default:
      DRW_Entity::parseCode(code, reader);
      break;
  }
}

void DRW_Text::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      height = reader->GetDouble();
      break;
    case 41:
      widthscale = reader->GetDouble();
      break;
    case 50:
      angle = reader->GetDouble();
      break;
    case 51:
      oblique = reader->GetDouble();
      break;
    case 71:
      textgen = reader->GetInt32();
      break;
    case 72:
      alignH = (HAlign)reader->GetInt32();
      break;
    case 73:
      alignV = (VAlign)reader->GetInt32();
      break;
    case 1:
      text = reader->GetUtf8String();
      break;
    case 7:
      style = reader->GetUtf8String();
      break;
    default:
      DRW_Line::parseCode(code, reader);
      break;
  }
}

void DRW_MText::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      text += reader->GetString();
      text = reader->ToUtf8String(text);
      break;
    case 11:
      haveXAxis = true;
      DRW_Text::parseCode(code, reader);
      break;
    case 3:
      text += reader->GetString();
      break;
    case 44:
      interlin = reader->GetDouble();
      break;
    default:
      DRW_Text::parseCode(code, reader);
      break;
  }
}

void DRW_MText::updateAngle() {
  if (haveXAxis) { angle = atan2(secPoint.y, secPoint.x) * 180 / DRW::Pi; }
}

void DRW_Polyline::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      flags = reader->GetInt32();
      break;
    case 40:
      defstawidth = reader->GetDouble();
      break;
    case 41:
      defendwidth = reader->GetDouble();
      break;
    case 71:
      vertexcount = reader->GetInt32();
      break;
    case 72:
      facecount = reader->GetInt32();
      break;
    case 73:
      smoothM = reader->GetInt32();
      break;
    case 74:
      smoothN = reader->GetInt32();
      break;
    case 75:
      curvetype = reader->GetInt32();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Vertex::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      flags = reader->GetInt32();
      break;
    case 40:
      stawidth = reader->GetDouble();
      break;
    case 41:
      endwidth = reader->GetDouble();
      break;
    case 42:
      bulge = reader->GetDouble();
      break;
    case 50:
      tgdir = reader->GetDouble();
      break;
    case 71:
      vindex1 = reader->GetInt32();
      break;
    case 72:
      vindex2 = reader->GetInt32();
      break;
    case 73:
      vindex3 = reader->GetInt32();
      break;
    case 74:
      vindex4 = reader->GetInt32();
      break;
    case 91:
      identifier = reader->GetInt32();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Hatch::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      solid = reader->GetInt32();
      break;
    case 71:
      associative = reader->GetInt32();
      break;
    case 72:        /*edge type*/
      if (ispol) {  //if is polyline is a as_bulge flag
        break;
      } else if (reader->GetInt32() == 1) {  //line
        addLine();
      } else if (reader->GetInt32() == 2) {  //arc
        addArc();
      } else if (reader->GetInt32() == 3) {  //elliptic arc
        addEllipse();
      } else if (reader->GetInt32() == 4) {  //spline
        addSpline();
      }
      break;
    case 10:
      if (pt)
        pt->basePoint.x = reader->GetDouble();
      else if (pline) {
        plvert = pline->addVertex();
        plvert->x = reader->GetDouble();
      }
      break;
    case 20:
      if (pt)
        pt->basePoint.y = reader->GetDouble();
      else if (plvert)
        plvert->y = reader->GetDouble();
      break;
    case 11:
      if (line)
        line->secPoint.x = reader->GetDouble();
      else if (ellipse)
        ellipse->secPoint.x = reader->GetDouble();
      break;
    case 21:
      if (line)
        line->secPoint.y = reader->GetDouble();
      else if (ellipse)
        ellipse->secPoint.y = reader->GetDouble();
      break;
    case 40:
      if (arc)
        arc->radious = reader->GetDouble();
      else if (ellipse)
        ellipse->ratio = reader->GetDouble();
      break;
    case 41:
      scale = reader->GetDouble();
      break;
    case 42:
      if (plvert) plvert->bulge = reader->GetDouble();
      break;
    case 50:
      if (arc)
        arc->staangle = reader->GetDouble() / DRW::ARAD;
      else if (ellipse)
        ellipse->staparam = reader->GetDouble() / DRW::ARAD;
      break;
    case 51:
      if (arc)
        arc->endangle = reader->GetDouble() / DRW::ARAD;
      else if (ellipse)
        ellipse->endparam = reader->GetDouble() / DRW::ARAD;
      break;
    case 52:
      angle = reader->GetDouble();
      break;
    case 73:
      if (arc)
        arc->isccw = reader->GetInt32();
      else if (pline)
        pline->flags = reader->GetInt32();
      break;
    case 75:
      hstyle = reader->GetInt32();
      break;
    case 76:
      hpattern = reader->GetInt32();
      break;
    case 77:
      doubleflag = reader->GetInt32();
      break;
    case 78:
      deflines = reader->GetInt32();
      break;
    case 91:
      loopsnum = reader->GetInt32();
      looplist.reserve(loopsnum);
      break;
    case 92:
      loop = new DRW_HatchLoop(reader->GetInt32());
      looplist.push_back(loop);
      if (reader->GetInt32() & 2) {
        ispol = true;
        clearEntities();
        pline = new DRW_LWPolyline;
        loop->objlist.push_back(pline);
      } else
        ispol = false;
      break;
    case 93:
      if (pline)
        pline->vertexnum = reader->GetInt32();
      else
        loop->numedges = reader->GetInt32();  //aqui reserve
      break;
    case 98:  //seed points ??
      clearEntities();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}

void DRW_Spline::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 210:
      normalVec.x = reader->GetDouble();
      break;
    case 220:
      normalVec.y = reader->GetDouble();
      break;
    case 230:
      normalVec.z = reader->GetDouble();
      break;
    case 12:
      tgStart.x = reader->GetDouble();
      break;
    case 22:
      tgStart.y = reader->GetDouble();
      break;
    case 32:
      tgStart.z = reader->GetDouble();
      break;
    case 13:
      tgEnd.x = reader->GetDouble();
      break;
    case 23:
      tgEnd.y = reader->GetDouble();
      break;
    case 33:
      tgEnd.z = reader->GetDouble();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    case 71:
      degree = reader->GetInt32();
      break;
    case 72:
      nknots = reader->GetInt32();
      break;
    case 73:
      ncontrol = reader->GetInt32();
      break;
    case 74:
      nfit = reader->GetInt32();
      break;
    case 42:
      tolknot = reader->GetDouble();
      break;
    case 43:
      tolcontrol = reader->GetDouble();
      break;
    case 44:
      tolfit = reader->GetDouble();
      break;
    case 10: {
      controlpoint = new DRW_Coord();
      controllist.push_back(controlpoint);
      controlpoint->x = reader->GetDouble();
      break;
    }
    case 20:
      if (controlpoint != nullptr) controlpoint->y = reader->GetDouble();
      break;
    case 30:
      if (controlpoint != nullptr) controlpoint->z = reader->GetDouble();
      break;
    case 11: {
      fitpoint = new DRW_Coord();
      fitlist.push_back(fitpoint);
      fitpoint->x = reader->GetDouble();
      break;
    }
    case 21:
      if (fitpoint != nullptr) fitpoint->y = reader->GetDouble();
      break;
    case 31:
      if (fitpoint != nullptr) fitpoint->z = reader->GetDouble();
      break;
    case 40:
      knotslist.push_back(reader->GetDouble());
      break;
      //    case 41:
      //        break;
    default:
      DRW_Entity::parseCode(code, reader);
      break;
  }
}

void DRW_Image::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 12:
      vVector.x = reader->GetDouble();
      break;
    case 22:
      vVector.y = reader->GetDouble();
      break;
    case 32:
      vVector.z = reader->GetDouble();
      break;
    case 13:
      sizeu = reader->GetDouble();
      break;
    case 23:
      sizev = reader->GetDouble();
      break;
    case 340:
      ref = reader->GetHandleString();
      break;
    case 280:
      clip = reader->GetInt32();
      break;
    case 281:
      brightness = reader->GetInt32();
      break;
    case 282:
      contrast = reader->GetInt32();
      break;
    case 283:
      fade = reader->GetInt32();
      break;
    default:
      DRW_Line::parseCode(code, reader);
      break;
  }
}

void DRW_Dimension::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      text = reader->GetUtf8String();
      break;
    case 2:
      name = reader->GetString();
      break;
    case 3:
      style = reader->GetUtf8String();
      break;
    case 70:
      type = reader->GetInt32();
      break;
    case 71:
      align = reader->GetInt32();
      break;
    case 72:
      linesty = reader->GetInt32();
      break;
    case 10:
      defPoint.x = reader->GetDouble();
      break;
    case 20:
      defPoint.y = reader->GetDouble();
      break;
    case 30:
      defPoint.z = reader->GetDouble();
      break;
    case 11:
      textPoint.x = reader->GetDouble();
      break;
    case 21:
      textPoint.y = reader->GetDouble();
      break;
    case 31:
      textPoint.z = reader->GetDouble();
      break;
    case 12:
      clonePoint.x = reader->GetDouble();
      break;
    case 22:
      clonePoint.y = reader->GetDouble();
      break;
    case 32:
      clonePoint.z = reader->GetDouble();
      break;
    case 13:
      def1.x = reader->GetDouble();
      break;
    case 23:
      def1.y = reader->GetDouble();
      break;
    case 33:
      def1.z = reader->GetDouble();
      break;
    case 14:
      def2.x = reader->GetDouble();
      break;
    case 24:
      def2.y = reader->GetDouble();
      break;
    case 34:
      def2.z = reader->GetDouble();
      break;
    case 15:
      circlePoint.x = reader->GetDouble();
      break;
    case 25:
      circlePoint.y = reader->GetDouble();
      break;
    case 35:
      circlePoint.z = reader->GetDouble();
      break;
    case 16:
      arcPoint.x = reader->GetDouble();
      break;
    case 26:
      arcPoint.y = reader->GetDouble();
      break;
    case 36:
      arcPoint.z = reader->GetDouble();
      break;
    case 41:
      linefactor = reader->GetDouble();
      break;
    case 53:
      rot = reader->GetDouble();
      break;
    case 50:
      angle = reader->GetDouble();
      break;
    case 52:
      oblique = reader->GetDouble();
      break;
    case 40:
      length = reader->GetDouble();
      break;
    case 51:
      hdir = reader->GetDouble();
      break;
    default:
      DRW_Entity::parseCode(code, reader);
      break;
  }
}

void DRW_Leader::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 3:
      style = reader->GetUtf8String();
      break;
    case 71:
      arrow = reader->GetInt32();
      break;
    case 72:
      leadertype = reader->GetInt32();
      break;
    case 73:
      flag = reader->GetInt32();
      break;
    case 74:
      hookline = reader->GetInt32();
      break;
    case 75:
      hookflag = reader->GetInt32();
      break;
    case 76:
      vertnum = reader->GetInt32();
      break;
    case 77:
      coloruse = reader->GetInt32();
      break;
    case 40:
      textheight = reader->GetDouble();
      break;
    case 41:
      textwidth = reader->GetDouble();
      break;
    case 10: {
      vertexpoint = new DRW_Coord();
      vertexlist.push_back(vertexpoint);
      vertexpoint->x = reader->GetDouble();
      break;
    }
    case 20:
      if (vertexpoint != nullptr) vertexpoint->y = reader->GetDouble();
      break;
    case 30:
      if (vertexpoint != nullptr) vertexpoint->z = reader->GetDouble();
      break;
    case 340:
      annotHandle = reader->GetHandleString();
      break;
    case 210:
      extrusionPoint.x = reader->GetDouble();
      break;
    case 220:
      extrusionPoint.y = reader->GetDouble();
      break;
    case 230:
      extrusionPoint.z = reader->GetDouble();
      break;
    case 211:
      horizdir.x = reader->GetDouble();
      break;
    case 221:
      horizdir.y = reader->GetDouble();
      break;
    case 231:
      horizdir.z = reader->GetDouble();
      break;
    case 212:
      offsetblock.x = reader->GetDouble();
      break;
    case 222:
      offsetblock.y = reader->GetDouble();
      break;
    case 232:
      offsetblock.z = reader->GetDouble();
      break;
    case 213:
      offsettext.x = reader->GetDouble();
      break;
    case 223:
      offsettext.y = reader->GetDouble();
      break;
    case 233:
      offsettext.z = reader->GetDouble();
      break;
    default:
      DRW_Entity::parseCode(code, reader);
      break;
  }
}

void DRW_Viewport::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      pswidth = reader->GetDouble();
      break;
    case 41:
      psheight = reader->GetDouble();
      break;
    case 68:
      vpstatus = reader->GetInt32();
      break;
    case 69:
      vpID = reader->GetInt32();
      break;
    case 12: {
      centerPX = reader->GetDouble();
      break;
    }
    case 22:
      centerPY = reader->GetDouble();
      break;
    default:
      DRW_Point::parseCode(code, reader);
      break;
  }
}
