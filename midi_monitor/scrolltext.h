class ScrollText
{
  private:
    // the first and last lines that we allow to scroll
    // on 128x64 screen, with font size= 1, a character is 6px wide by 8px tall, (21 columns by 8 lines)
    byte _start_line = 0;
    byte _last_line = 7;

    byte _current_line = 0;

    byte _scroll_rate = 1;
    
    Adafruit_SSD1306* _display;

    void scroll_if_required(void);
  public:
    ScrollText(Adafruit_SSD1306* d, byte start_line, byte last_line);
    void show(void);

    void print(char* text);
    void print(const __FlashStringHelper *ifsh);

    void println(char* text);
    void println(int value);
    void println(const __FlashStringHelper *ifsh);
    
    void scroll(void);

    void initCursor(void);
};

ScrollText::ScrollText(Adafruit_SSD1306* d, byte start_line = 0, byte last_line = 7)
{
  _display = d;

  _start_line = start_line;
  _last_line = last_line;
  _current_line = _start_line;
}

void ScrollText::scroll_if_required(void)
{
  if (_current_line > _last_line)
  {
    _current_line = _last_line;
    scroll();
  }
}

void ScrollText::print(char* text)
{
  scroll_if_required();
  _display->print(text);
}

void ScrollText::print(const __FlashStringHelper *ifsh)
{
  scroll_if_required();
  _display->print(ifsh);
}

void ScrollText::println(char* text)
{
  scroll_if_required();
  _display->println(text);

  _current_line++;
}

void ScrollText::println(int value)
{
  scroll_if_required();
  _display->println(value);

  _current_line++;
}

void ScrollText::println(const __FlashStringHelper *ifsh)
{
  scroll_if_required();
  _display->println(ifsh);

  _current_line++;
}

void ScrollText::show(void)
{
  _display->display();                        // update display
}

void ScrollText::scroll(void)
{
  uint8_t *buffer = _display->getBuffer();

  memmove(buffer + (128 * 2), buffer + (128 * 3), 128 * 5);   // move line block up by one
  memset(buffer + (128 * 7), 0, 128);                         // erase last line

  _display->setCursor(0, 8 * _current_line);
}

void ScrollText::initCursor(void)
{
  _display->setCursor(0, 8 * _start_line);
}
