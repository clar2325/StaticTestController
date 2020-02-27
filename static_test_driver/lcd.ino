

void set_lcd_status(String state) {
  state.replace("_", " ");
  state.toLowerCase();
  state[0] = toupper(state[0]);
  lcd.setCursor(0, 0);
  lcd.print(state);
  for (unsigned i = state.length(); i < 20; i++) {
    lcd.print(' ');
  }
}
