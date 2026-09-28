# OLED UI test notes

The renderer depends on the real Adafruit SSD1306 library and the scheduling
engine interface, so integration testing should be performed in the Wokwi/
PlatformIO firmware target after the scheduling branch is merged.

Recommended scenarios:

1. No active dose -> clock + next dose.
2. One `DOSE_DUE` -> due screen.
3. Two simultaneous `DOSE_DUE` events -> both medicines rotate.
4. One dose becomes `DOSE_MISSED` -> missed screen remains visible.
5. Due + missed simultaneously -> both notification groups rotate.
6. `DOSE_CONFIRMED` -> confirmation message when no other notification exists.
7. `DOSE_DISMISSED` -> reminder-cleared message.
8. `LOW_STOCK_WARNING` -> warning screen.
9. Change configuration medicine names/cell count -> UI changes without code changes.
10. 16-cell scheduler configuration -> UI still uses event data rather than a fixed cell list.
