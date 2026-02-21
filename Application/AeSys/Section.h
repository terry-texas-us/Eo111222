#pragma once

class Section {
 public:
  static const long Round = 0x0001;
  static const long Oval = 0x0002;
  static const long Rectangular = 0x0004;
  static const long Fixed = 0x0010;

 public:
  Section();
  Section(double width, double depth, long properties);

  bool operator==(const Section& section);
  bool operator!=(const Section& section);
  void operator()(double width, double depth, long properties);
  void SetWidth(double width);
  void SetDepth(double depth);
  [[nodiscard]] double Width() const;
  [[nodiscard]] double Depth() const;
  bool Identical(const Section& section) const;
  bool IsRectangular() const;
  bool IsRound() const;

 private:
  double m_width;
  double m_depth;
  int m_properties;
};
