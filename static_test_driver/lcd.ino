
#define WIDTH 16
#define SCROLL_PERIOD 700 // milliseconds

void set_lcd_status(String state) {
  state.replace("_", " ");
  state.toLowerCase();
  state[0] = toupper(state[0]);
  lcd.setCursor(0, 0);
  lcd.print(state.substring(0, WIDTH));
  for (unsigned i = state.length(); i < WIDTH; i++) {
    lcd.print(' ');
  }
}

void set_lcd_errors(String errors) {
  lcd.setCursor(0, 1);
  int max_scroll_index = max(0, (signed)errors.length() - WIDTH + 1) + 1; // Scroll one space past the end
  unsigned scroll_index = (millis() / SCROLL_PERIOD) % max_scroll_index;
  errors = errors.substring(scroll_index, scroll_index + WIDTH);
  lcd.print(errors);
  for (unsigned i = errors.length(); i < WIDTH; i++) {
    lcd.print(' ');
  }
}
