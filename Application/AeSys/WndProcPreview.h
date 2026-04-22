#pragma once

/** Initializes the preview bitmap for the given preview window.
 *
 * @param previewWindow Handle to the preview window.
 */
void WndProcPreviewClear(HWND previewWindow);

/** Updates the preview window to display the given block.
 *
 * @param previewWindow Handle to the preview window.
 * @param block Pointer to the block to display.
 */
void WndProcPreviewUpdateBlock(HWND previewWindow, EoDbBlock* block);
