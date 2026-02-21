#pragma once

class EoCtrlColorsButton : public CMFCButton {
  DECLARE_DYNAMIC(EoCtrlColorsButton)

  static COLORREF* m_palette;
  static std::uint16_t m_currentIndex;
  static std::uint16_t m_selectedIndex;

  enum Layouts { SimpleSingleRow, GridDown5RowsOddOnly, GridUp5RowsEvenOnly };
  Layouts m_layout;
  CSize m_cellSize;
  CSize m_cellSpacing;
  CSize m_margins;
  std::uint16_t m_beginIndex;
  std::uint16_t m_endIndex;
  std::uint16_t m_subItem;

  /**
   * Draws a single color cell at the specified index.
   *
   * @param deviceContext The device context to draw on.
   * @param index The index of the color cell to draw.
   * @param color The color to fill the cell with.
   */
  void DrawCell(CDC* deviceContext, std::uint16_t index, COLORREF color) const;
  
  /** @brief Determines the sub-item index at a given point.
   *
   * @param point The point to check.
   * @return The index of the sub-item at the specified point, or 0 if none.
   */
  std::uint16_t SubItemByPoint(const CPoint& point) const;

  /** @brief Calculates the rectangle of a sub-item (color cell) by its index.
   *
   * @param index The index of the sub-item.
   * @param rectangle The rectangle to be filled with the sub-item's dimensions.
   */
  void SubItemRectangleByIndex(std::uint16_t index, CRect& rectangle) const;

 public:
  EoCtrlColorsButton();
  EoCtrlColorsButton(const EoCtrlColorsButton&) = delete;
  EoCtrlColorsButton& operator=(const EoCtrlColorsButton&) = delete;

  virtual ~EoCtrlColorsButton();

  static void SetCurrentIndex(const std::uint16_t index) { m_currentIndex = index; }
  static void SetPalette(COLORREF* palette) { m_palette = palette; }
  void SetLayout(Layouts layout, const CSize& cellSize) {
    m_layout = layout;
    m_cellSize = cellSize;
  }
  void SetSequenceRange(const std::uint16_t beginIndex, const std::uint16_t endIndex) {
    m_beginIndex = beginIndex;
    m_endIndex = endIndex;
  }

  /** @brief Draws the color button control.
   *
   * This method is responsible for rendering the color button control by iterating
   * through the defined color palette and drawing each color cell based on the specified layout.
   *
   * @param deviceContext Pointer to the device context used for drawing.
   * @param rectangle The rectangle area to draw (not used in this implementation).
   * @param state The state of the button (not used in this implementation).
   */
  void OnDraw(CDC* deviceContext, const CRect& rectangle, UINT state) override;
  
  /**
   * Sizes the control to fit its content.
   * @param calculateOnly If TRUE, only calculates the size without resizing the control.
   * @return The calculated size of the control.
   */
  CSize SizeToContent(BOOL calculateOnly = FALSE) override;

  afx_msg UINT OnGetDlgCode();
  
  /** @brief Handles key down events for navigation within the color button control.
   *
   * This method processes arrow key inputs to navigate through the color cells
   * based on the current layout of the control. It updates the selected index
   * and redraws the affected cells accordingly.
   *
   * @param keyCode The virtual key code of the key that was pressed.
   * @param repeatCount The repeat count for the key press.
   * @param flags Additional flags associated with the key event.
   */
  afx_msg void OnKeyDown(UINT keyCode, UINT repeatCount, UINT flags);

  /** @brief Handles left mouse button release events.
   *
   * This method updates the selected sub-item based on the mouse click position
   * and calls the base class implementation to ensure proper event handling.
   *
   * @param flags Indicates whether various virtual keys are down.
   * @param point The x- and y-coordinates of the cursor relative to the upper-left corner of the control.
   */
  afx_msg void OnLButtonUp(UINT flags, CPoint point);

  /** @brief Handles mouse move events to update the selected color cell.
   *
   * This method updates the selected color cell based on the mouse position.
   * It redraws the previously selected cell and the newly selected cell,
   * and sends a notification to the parent window about the selection change.
   *
   * @param flags Indicates whether various virtual keys are down.
   * @param point The current position of the cursor.
   */
  afx_msg void OnMouseMove(UINT flags, CPoint point);
  afx_msg void OnPaint();

  /** @brief Handles the event when the control gains focus.
   *
   * This method is called when the color button control receives focus.
   * It updates the visual representation of the currently selected color cell
   * and notifies the parent window about the focus change.
   *
   * @param oldWindow Pointer to the previous window that had focus.
   */
  afx_msg void OnSetFocus(CWnd* oldWindow);

 protected:
  DECLARE_MESSAGE_MAP()
};
