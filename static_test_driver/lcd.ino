

void set_lcd_status(String state) {
  state.replace("_", " ");
  state.toLowerCase();
  state[0] = toupper(state[0]);
  lcd.setCursor(0, 0);
  lcd.print(state.substring(0, 20));
  for (unsigned i = state.length(); i < 20; i++) {
    lcd.print(' ');
  }
}

void set_lcd_errors(String errors) {
  lcd.setCursor(0, 1);
  lcd.print(errors.substring(0, 20));
  for (unsigned i = errors.length(); i < 20; i++) {
    lcd.print(' ');
  }
}
