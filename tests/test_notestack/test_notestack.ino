#include <MemoryUsage.h>

#include "note_stack.h"

const uint8_t kNumVoices = 2;
NoteStack<6> mono_allocator_[kNumVoices];

void setup()
{
  Serial.begin(115200);
  Serial.println(F("\n\n_____test notestack_____"));

//  uint8_t voice = 1;


//  uint8_t note = 20;
//  uint8_t velocity = 127;

  FREERAM_PRINT;
  
  test_init(1);

  Serial.println(F("\n** Add notes, then remove notes, in expected sequence ** "));

  test_add_note(1, 20, 127);
  test_add_note(1, 22, 127);
  test_add_note(1, 21, 127);
  test_add_note(1, 23, 127);

  test_remove_note(1, 23);
  test_remove_note(1, 22);
  test_remove_note(1, 21);
  test_remove_note(1, 20);

  test_clear(1);

  Serial.println(F("\n** Add redundant notes ** "));

  test_add_note(1, 22, 127);
  test_add_note(1, 22, 127);
  test_add_note(1, 21, 127);
  test_add_note(1, 21, 127);

  test_clear(1);

  Serial.println(F("\n** Add notes, then remove more notes than added ** "));

  test_add_note(1, 21, 127);
  test_add_note(1, 22, 127);
  test_add_note(1, 23, 127);

  test_remove_note(1, 22);
  test_remove_note(1, 23);
  test_remove_note(1, 20);
  test_remove_note(1, 21);
  test_remove_note(1, 23);

  test_clear(1);
  
}



void test_init(uint8_t voice)
{
  FREERAM_PRINT;
  Serial.print(F("\n---- init voice "));
  Serial.println(voice);

  mono_allocator_[voice].Init();

  Serial.print(F("max_size = "));
  Serial.println(mono_allocator_[voice].max_size());

  Serial.print(F("size = "));
  Serial.println(mono_allocator_[voice].size());
}


void test_add_note(uint8_t voice, uint8_t note, uint8_t velocity)
{
//  uint8_t size = 0;
//  uint8_t max_size = 0;

  Serial.print(F("\nadd note "));
  Serial.println(note);

  mono_allocator_[voice].NoteOn(note, velocity);

  uint8_t top_note = mono_allocator_[voice].most_recent_note().note;
  Serial.print(F("top note = "));
  Serial.println(top_note);

//  Serial.print(F("note played = "));
//  Serial.println(mono_allocator_[voice].played_note().note);

  Serial.print(F("size = "));
  Serial.println(mono_allocator_[voice].size());
  FREERAM_PRINT;
}


void test_remove_note(uint8_t voice, uint8_t note)
{
  Serial.print(F("\nremove note "));
  Serial.println(note);

  mono_allocator_[voice].NoteOff(note);

  Serial.print(F("top note = "));
  Serial.println(mono_allocator_[voice].most_recent_note().note);

//  Serial.print(F("note played = "));
//  Serial.println(mono_allocator_[voice].played_note().note);

  Serial.print(F("size = "));
  Serial.println(mono_allocator_[voice].size());
  FREERAM_PRINT;
}


void test_clear(uint8_t voice)
{
  Serial.print(F("\nclear voice"));
  Serial.println(voice);

  mono_allocator_[voice].Clear();

  Serial.print(F("top note = "));
  Serial.println(mono_allocator_[voice].most_recent_note().note);

//  Serial.print(F("note played = "));
//  Serial.println(mono_allocator_[voice].played_note().note);

  Serial.print(F("size = "));
  Serial.println(mono_allocator_[voice].size());
  FREERAM_PRINT;
}


void loop()
{
  
}
