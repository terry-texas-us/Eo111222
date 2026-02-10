#pragma once

void WndProcPreviewClear(HWND previewWindow);
void WndProcPreviewUpdateBlock(HWND previewWindow, EoDbBlock* block);
void WndProcPreviewUpdateLayer(HWND previewWindow, EoDbGroupList* groups);
