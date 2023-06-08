/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Initially written by Chris van Dongen, 2010.
 *
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate.  This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

/**
 * @file basic.c
 *
 * @brief BASIC interpreter functions implementation file.
 */

#include "basic.h"
#include "types.h"
#include "cli.h"
#include "cli_i.h"
#include "cli_commands.h"
#include "cli_vcp.h"
#include <furi_hal_version.h>
#include <loader/loader.h>

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"

#if BP_BASIC_PROGRAM_SPACE <= 0
#error "Invalid BASIC program space value"
#endif /* BP_BASIC_PROGRAM_SPACE <= 0 */

#if BP_BASIC_NESTED_FOR_LOOP_COUNT <= 1
#error "Invalid nested BASIC FOR-LOOP count"
#endif /* BP_BASIC_NESTED_FOR_LOOP_COUNT <= 1 */

#if BP_BASIC_STACK_FRAMES_DEPTH <= 1
#error "Invalid BASIC stack depth"
#endif /* BP_BASIC_STACK_FRAMES_DEPTH <= 1*/

/**
 * @brief How many variables the BASIC interpreter can handle.
 *
 * Currently set to 26 to handle variables identified from 'A' to 'Z'.
 */
#define BP_BASIC_VARIABLES_COUNT 26

// I am commenting these out to get it to compile and see if I can just run the interpreter at all and make a program that doesn't
// try to access any hardware. once that works then I can start adding in the hardware access functions
//#include "aux_pin.h"
//#include "base.h"
//#include "bitbang.h"
//#include "core.h"
//#include "proc_menu.h"

#define INVALID_TOKEN 0x00

#define TOKENS 0x80
#define TOK_LET 0x80
#define TOK_IF 0x81
#define TOK_THEN 0x82
#define TOK_ELSE 0x83
#define TOK_GOTO 0x84
#define TOK_GOSUB 0x85
#define TOK_RETURN 0x86
#define TOK_REM 0x87
#define TOK_PRINT 0x88
#define TOK_INPUT 0x89
#define TOK_FOR 0x8A
#define TOK_TO 0x8B
#define TOK_NEXT 0x8C
#define TOK_READ 0x8D
#define TOK_DATA 0x8E
#define TOK_STARTR 0x8F
#define TOK_START 0x90
#define TOK_STOPR 0x91
#define TOK_STOP 0x92
#define TOK_SEND 0x93
#define TOK_RECEIVE 0x94
#define TOK_CLK 0x95
#define TOK_DAT 0x96
#define TOK_BITREAD 0x97
#define TOK_ADC 0x98
#define TOK_AUXPIN 0x99
#define TOK_PSU 0x9A
#define TOK_PULLUP 0x9B
#define TOK_DELAY 0x9C

#define TOK_AUX 0x9D
#define TOK_FREQ 0x9E
#define TOK_DUTY 0x9F

#define TOK_MACRO 0xA0
#define TOK_END 0xA1

#define TOK_LEN 0xE0

#define NUMTOKEN (TOK_END - TOKENS) + 1

#define STAT_LET "LET"
#define STAT_IF "IF"
#define STAT_THEN "THEN"
#define STAT_ELSE "ELSE"
#define STAT_GOTO "GOTO"
#define STAT_GOSUB "GOSUB"
#define STAT_RETURN "RETURN"
#define STAT_REM "REM"
#define STAT_PRINT "PRINT"
#define STAT_INPUT "INPUT"
#define STAT_FOR "FOR"
#define STAT_TO "TO"
#define STAT_NEXT "NEXT"
#define STAT_END "END"
#define STAT_READ "READ"
#define STAT_DATA "DATA"
#define STAT_START "START"
#define STAT_STARTR "STARTR"
#define STAT_STOP "STOP"
#define STAT_STOPR "STOPR"
#define STAT_SEND "SEND"
#define STAT_RECEIVE "RECEIVE"
#define STAT_CLK "CLK"
#define STAT_DAT "DAT"
#define STAT_BITREAD "BITREAD"
#define STAT_ADC "ADC"
#define STAT_AUX "AUX"
#define STAT_PSU "PSU"
#define STAT_PULLUP "PULLUP"
#define STAT_DELAY "DELAY"

#define STAT_AUXPIN "AUXPIN"
#define STAT_FREQ "FREQ"
#define STAT_DUTY "DUTY"
#define STAT_MACRO "MACRO"

/**
 * @brief Status codes enumeration for the internal BASIC parser.
 */
typedef enum {

  /**
   * No status has been obtained yet.
   */
  STATUS_CODE_UNKNOWN = 0,

  /**
   * An `END` statement was reached whilst parsing the program.
   */
  STATUS_CODE_FINISHED,

  /**
   * A partial instruction was found in the program listing.
   */
  STATUS_CODE_PARTIAL_INSTRUCTION_ERROR,

  /**
   * A syntax error was found.
   */
  STATUS_CODE_SYNTAX_ERROR,

  /**
   * An invalid `FOR` statement was parsed.
   */
  STATUS_CODE_FOR_ERROR,

  /**
   * An invalid `NEXT` statement was parsed.
   */
  STATUS_CODE_NEXT_ERROR,

  /**
   * An invalid `GOTO` statement was parsed.
   */
  STATUS_CODE_GOTO_ERROR,

  /**
   * A call stack overflow was observed.
   */
  STATUS_CODE_STACK_ERROR,

  /**
   * An invalid `RETURN` statement was parsed.
   */
  STATUS_CODE_RETURN_ERROR,

  /**
   * One or more invalid `DATA` statements were found.
   */
  STATUS_CODE_DATA_ERROR
} __attribute__((packed)) basic_status_code_t;

/**
 * @brief Flag indicating that the chosen line number was not found.
 */
#define LINE_NUMBER_NOT_FOUND NO

/**
 * @brief Flag indicating that the chosen line number was found.
 */
#define LINE_NUMBER_FOUND YES

#define BELL 0x07

#define BP_COMMAND_BUFFER_SIZE 256
#define BP_USER_MACROS_COUNT 5
#define BP_USER_MACRO_MAX_LENGTH 32
#define CMDLENMSK (BP_COMMAND_BUFFER_SIZE - 1)

char cmdbuf[BP_COMMAND_BUFFER_SIZE] = {0};
unsigned int cmdend;
unsigned int cmdstart;

static char user_macros[BP_USER_MACROS_COUNT][BP_USER_MACRO_MAX_LENGTH];
static int user_macro;

//extern bus_pirate_configuration_t bus_pirate_configuration;
//extern mode_configuration_t mode_configuration;

// copied from base.h originally
typedef struct {
  uint16_t num;
  uint16_t repeat;
  uint8_t cmd;
} __attribute__((packed)) command_t;

command_t last_command;

//extern bus_pirate_protocol_t enabled_protocols[ENABLED_PROTOCOLS_COUNT];

typedef struct {
  uint16_t from;
  uint16_t var;
  uint16_t to;
} __attribute__((packed)) basic_for_loop_t;

static int16_t basic_variables[BP_BASIC_VARIABLES_COUNT];
static int16_t basic_stack[BP_BASIC_STACK_FRAMES_DEPTH];
static basic_for_loop_t basic_nested_for_loops[BP_BASIC_NESTED_FOR_LOOP_COUNT];
static uint16_t basic_program_counter;
static uint16_t basic_current_nested_for_index;
static uint16_t basic_current_stack_frame;

/**
 * @brief Offset variable for `DATA` statements.
 */
static uint16_t basic_data_read_pointer;

static char *tokens[NUMTOKEN + 1] = {
    STAT_LET,     // 0x80
    STAT_IF,      // 0x81
    STAT_THEN,    // 0x82
    STAT_ELSE,    // 0x83
    STAT_GOTO,    // 0x84
    STAT_GOSUB,   // 0x85
    STAT_RETURN,  // 0x86
    STAT_REM,     // 0x87
    STAT_PRINT,   // 0x88
    STAT_INPUT,   // 0x89
    STAT_FOR,     // 0x8A
    STAT_TO,      // 0x8b
    STAT_NEXT,    // 0x8c
    STAT_READ,    // 0x8d
    STAT_DATA,    // 0x8e
    STAT_STARTR,  // 0x8f
    STAT_START,   // 0x90
    STAT_STOPR,   // 0x91
    STAT_STOP,    // 0x92
    STAT_SEND,    // 0x93
    STAT_RECEIVE, // 0x94
    STAT_CLK,     // 0x95
    STAT_DAT,     // 0x96
    STAT_BITREAD, // 0x97
    STAT_ADC,     // 0x98
    STAT_AUXPIN,  // 0x99
    STAT_PSU,     // 0x9a
    STAT_PULLUP,  // 0x9b
    STAT_DELAY,   // 0x9c
    STAT_AUX,     // 0x9d
    STAT_FREQ,    // 0x9e
    STAT_DUTY,    // 0x9f

    STAT_MACRO, // 0xA0
    STAT_END,   // 0xA1
};

/**
 * @brief BASIC program data area.
 */
static uint8_t basic_program_area[BP_BASIC_PROGRAM_SPACE] = {0};

/**
 * @brief Scans the currently loaded program to find the token index of the
 * given line number.
 *
 * @param[in] line the line number to obtain an offset for.
 * @param[out] result the offset in program memory for the line in question.
 *
 * @return LINE_NUMBER_FOUND if a matching line was found in program memory.
 * @return LINE_NUMBER_NOT_FOUND if no matching line was found in program
 * memory.
 *
 * @see LINE_NUMBER_FOUND
 * @see LINE_NUMBER_NOT_FOUND
 *
 */
static bool search_line_number(const uint16_t line, uint16_t *result);

/**
 * @brief Handles execution of particular tokens that are related to
 * the interface with the board hardware.
 *
 * Said tokens are `RECEIVE`, `SEND`, `AUX`, `DAT`, `BITREAD`, `PSU`,
 * `PULLUP`, and `ADC`.
 *
 * @param[in] token the token identifier to handle.
 *
 * @return the result value for the given execution, or 0 if the token was not
 * one of the special ones the function can handle.
 *
 * @see TOK_RECEIVE
 * @see TOK_SEND
 * @see TOK_AUX
 * @see TOK_DAT
 * @see TOK_BITREAD
 * @see TOK_PSU
 * @see TOK_PULLUP
 * @see TOK_ADC
 */
//static uint16_t handle_special_token(const uint8_t token);

/**
 * @brief Checks whether the string at the BASIC parser current position matches
 * a valid token, and returns it if one is found.
 *
 * @return the token identifier if a matching token is found.
 * @return INVALID_TOKEN if no valid token was found.
 *
 * @see INVALID_TOKEN
 */
static uint8_t get_token(void);


static void list(void);
static void interpreter(void);

/**
 * @brief Handles an else statement if one is detected.
 */
static void handle_else_statement(void);

static int16_t get_number_or_variable(void);
static int16_t get_multiplication_division_bitwise_ops(void);
static int16_t assign(void);
static void interpreter(void);

/**
 * @brief Compares the string pointed by the given pointer with the current
 * program space buffer pointer.
 *
 * @param[in] pointer the string to match.
 *
 * @return YES if the string matches, NO otherwise.
 */
static bool compare(char *pointer);

uint8_t get_token(void) {
  size_t index;

  for (index = 0; index < NUMTOKEN; index++) {
    if (compare(tokens[index])) {
      return TOKENS + index;
    }
  }

  return 0;
}

int getint(void) // get int from user (accept decimal, hex (0x) or binairy (0b)
{
  int i;
  int number;

  i = 0;
  number = 0;

  if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x31) &&
      (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // 1-9 is for sure decimal
  {
    number = cmdbuf[(cmdstart + i) & CMDLENMSK] - 0x30;
    i++;
    while ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) &&
           (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // consume all digits
    {
      number *= 10;
      number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
      i++;
    }
  } else if (cmdbuf[((cmdstart + i) & CMDLENMSK)] ==
             0x30) // 0 could be anything
  {
    i++;
    if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'b') ||
        (cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'B')) {
      i++;
      while ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == '1') ||
             (cmdbuf[((cmdstart + i) & CMDLENMSK)] == '0')) {
        number <<= 1;
        number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
        i++;
      }
    } else if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'x') ||
               (cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'X')) {
      i++;
      while (((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) &&
              (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) ||
             ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 'A') &&
              (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 'F')) ||
             ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 'a') &&
              (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 'f'))) {
        number <<= 4;
        if ((cmdbuf[(cmdstart + i) & CMDLENMSK] >= 0x30) &&
            (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) {
          number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
        } else {
          cmdbuf[((cmdstart + i) & CMDLENMSK)] |= 0x20; // make it lowercase
          number +=
              cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x57; // 'a'-0x61+0x0a
        }
        i++;
      }
    } else if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) &&
               (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) {
      number = cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
      while (
          (cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) &&
          (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // consume all digits
      {
        number *= 10;
        number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
        i++;
      }
    }
  } else // how did we come here??
  {
    //mode_configuration.command_error = YES;
    return 0;
  }

  cmdstart += i; // we used i chars
  cmdstart &= CMDLENMSK;
  return number;
} // getint(void)

void bp_basic_initialize(void) {
  basic_program_area[0] = TOK_LEN + 1;
  basic_program_area[1] = 0xFF;
  basic_program_area[2] = 0xFF;
  basic_program_area[3] = TOK_END;
  memset(&basic_program_area[4], 0x00, BP_BASIC_PROGRAM_SPACE - 4);
}

// copied from proc_menu.c and according to ChatGPT this is needed for assigning values to variables as it is used later.
// gets number from input
// -1 = abort (x)
// -2 = input to much
// 0-max return
// x=1 exit is enabled (we don't want that in the mode changes ;)

int getnumber(int def, int min, int max,
              int x) // default, minimum, maximum, show exit option
{
  char c;
  char buf[6]; // max 4 digits;
  int i, j, stop, temp, neg;

again: // need to do it proper with whiles and ifs..

  i = 0;
  stop = 0;
  temp = 0;
  neg = 0;

  printf("\r\n(");
  if (def < 0) {
    printf("x");
  } else {
    printf("%i", def);
  }
  printf(")>");

  while (!stop) {
    //c = user_serial_read_byte();
    FuriString* user_input;
    furi_string_get_cstr(user_input);
    switch (user_input) {
    case 0x08:
      if (i) {
        i--;
        // MSG_DESTRUCTIVE_BACKSPACE;
        printf("Destructive Backspace");
      } else {
        if (neg) {
          neg = 0;
          // MSG_DESTRUCTIVE_BACKSPACE;
          printf("Destructive Backspace");
        } else {
          printf("\a");
        }
      }
      break;
    case 0x0A:
    case 0x0D:
      stop = 1;
      break;
    case '-':
      if (!i) // enable negative numbers
      {
        printf("-"); 
        neg = 1;
      } else {
        printf("\a");
      }
      break;
    case 'x':
      //if (x)
        return -1; // user wants to quit :( only if we enable it :D
    default:
      if ((user_input >= 0x30) && (user_input <= 0x39)) // we got a digit
      {
        if (i > 3) // 0-9999 should be enough??
        {
          printf("\a");
          i = 4;
        } else {
          //user_serial_transmit_character(c);
          printf("%c", user_input);
          buf[i] = user_input; // store user input
          i++;
        }
      } else // ignore input :)
      {
        printf("\a");
      }
    }
  }
  printf("\r\n");

  if (i == 0) {
    return def; // no user input so return default option
  } else {
    temp = 0;
    i--;
    j = i;

    for (; i >= 0; i--) {
      temp *= 10;
      temp += (buf[j - i] - 0x30);
    }

    if ((temp >= min) && (temp <= max)) {
      if (neg) {
        return -temp;
      } else {
        return temp;
      }
    } else { // bpWline("\r\nInvalid choice, try again\r\n");
      //BPMSG1211;
      printf("\r\nInvalid choice, try again\r\n");
      goto again;
    }
  }
  return temp; // we dont get here, but keep compiler happy
}
// end getnumber function


void handle_else_statement(void) {
  if (basic_program_area[basic_program_counter] != TOK_ELSE) {
    return;
  }

  basic_program_counter++;
  while (basic_program_area[basic_program_counter] <= TOK_LEN) {
    basic_program_counter++;
  }
}

bool search_line_number(const uint16_t line, uint16_t *result) {
  size_t index = 0;
  uint8_t token_length;
  uint16_t current_line_number;

  for (;;) {
    if (basic_program_area[index] <= TOK_LEN) {
      return LINE_NUMBER_NOT_FOUND;
    }
    token_length = basic_program_area[index] - TOK_LEN;
    current_line_number =
        (basic_program_area[index + 1] << 8) + basic_program_area[index + 2];
    if (line == current_line_number) {
      *result = index;
      return LINE_NUMBER_FOUND;
    }
    index += token_length + 3;
  }

  return LINE_NUMBER_NOT_FOUND;
}

// uint16_t handle_special_token(const uint8_t token) {
//   switch (token) {

//   case TOK_RECEIVE:
//     return enabled_protocols[bus_pirate_configuration.bus_mode].read();

//   case TOK_SEND:
//     return enabled_protocols[bus_pirate_configuration.bus_mode].send(assign());

//   case TOK_AUX:
//     return bp_aux_pin_read();

//   case TOK_DAT:
//     return enabled_protocols[bus_pirate_configuration.bus_mode].data_state();

//   case TOK_BITREAD:
//     return enabled_protocols[bus_pirate_configuration.bus_mode].read_bit();

//   case TOK_PSU:
//     return BP_VREGEN;

//   case TOK_PULLUP:
//     return ~BP_PULLUP;

//   case TOK_ADC: {
//     bp_enable_adc();
//     uint16_t adc_measurement = bp_read_adc(BP_ADC_PROBE);
//     bp_disable_adc();

//     return adc_measurement;
//   }

//   default:
//     return 0;
//   }
// }

int16_t get_number_or_variable(void) {
  int16_t temp = 0;

  if ((basic_program_area[basic_program_counter] == '(')) {
    basic_program_counter++;
    temp = assign();
    if ((basic_program_area[basic_program_counter] == ')')) {
      basic_program_counter++;
    }
  } else {
    if ((basic_program_area[basic_program_counter] >= 'A') &&
        (basic_program_area[basic_program_counter] <= 'Z')) {
      return basic_variables[basic_program_area[basic_program_counter++] - 'A'];
    }

    if (basic_program_area[basic_program_counter] > TOKENS) {
      //return handle_special_token(basic_program_area[basic_program_counter++]);
      printf("Special token handled here for hardware interaction\n");
    }

    while ((basic_program_area[basic_program_counter] >= '0') &&
           (basic_program_area[basic_program_counter] <= '9')) {
      temp *= 10;
      temp += basic_program_area[basic_program_counter] - '0';
      basic_program_counter++;
    }
  }

  return temp;
}

int16_t get_multiplication_division_bitwise_ops(void) {
  int16_t temp = get_number_or_variable();

  for (;;) {
    switch (basic_program_area[basic_program_counter++]) {
    case '*':
      temp *= get_number_or_variable();
      break;

    case '/':
      temp /= get_number_or_variable();
      break;

    case '&':
      temp &= get_number_or_variable();
      break;

    case '|':
      temp |= get_number_or_variable();
      break;

    default:
      return temp;
    }
  }
}

int16_t assign(void) {
  int16_t temp = get_multiplication_division_bitwise_ops();

  for (;;) {
    switch (basic_program_area[basic_program_counter++]) {
    case '-':
      temp -= get_multiplication_division_bitwise_ops();
      break;

    case '+':
      temp += get_multiplication_division_bitwise_ops();
      break;

    case '>':
      if (basic_program_area[basic_program_counter + 1] == '=') {
        temp = (temp >= get_multiplication_division_bitwise_ops());
        basic_program_counter++;
      } else {
        temp = (temp > get_multiplication_division_bitwise_ops());
      }
      break;

    case '<':
      if (basic_program_area[basic_program_counter + 1] == '>') {
        temp = (temp != get_multiplication_division_bitwise_ops());
        basic_program_counter++;
      } else if (basic_program_area[basic_program_counter + 1] == '=') {
        temp = (temp <= get_multiplication_division_bitwise_ops());
        basic_program_counter++;
      } else {
        temp = (temp < get_multiplication_division_bitwise_ops());
      }
      break;

    case '=':
      temp = (temp == get_multiplication_division_bitwise_ops());
      break;

    default:
      return temp;
    }
  }
}

void interpreter(void) {
  bool program_counter_updated = NO;
  basic_status_code_t status = STATUS_CODE_UNKNOWN;

  int len = 0;
  unsigned int lineno = 0;
  int i;
  int ifstat = 0;

  int temp;

  basic_program_counter = 0;
  basic_current_nested_for_index = 0;
  basic_current_stack_frame = 0;
  basic_data_read_pointer = 0;
  //bus_pirate_configuration.quiet = ON;

  memset((void *)&basic_variables, 0, sizeof(basic_variables));

  while (status == STATUS_CODE_UNKNOWN) {
    if (!ifstat) {
      if (basic_program_area[basic_program_counter] < TOK_LEN) {
        status = STATUS_CODE_PARTIAL_INSTRUCTION_ERROR;
        break;
      }

      len = basic_program_area[basic_program_counter] - TOK_LEN;
      lineno = ((basic_program_area[basic_program_counter + 1] << 8) |
                basic_program_area[basic_program_counter + 2]);
    }

    ifstat = 0;

    switch (basic_program_area[basic_program_counter + 3]) {
    case TOK_LET:
      program_counter_updated = YES;
      basic_program_counter += 6;

      basic_variables[basic_program_area[basic_program_counter - 2] - 0x41] =
          assign();
      handle_else_statement();
      break;

    case TOK_IF:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (assign()) {
        if (basic_program_area[basic_program_counter++] == TOK_THEN) {
          ifstat = 1;
          basic_program_counter -= 3; // simplest way (for now)
        }
      } else {
        while ((basic_program_area[basic_program_counter] != TOK_ELSE) &&
               (basic_program_area[basic_program_counter] <= TOK_LEN)) {
          basic_program_counter++;
        }
        if (basic_program_area[basic_program_counter] == TOK_ELSE) {
          ifstat = 1;
          basic_program_counter -= 2; // simplest way (for now)
        }
      }
      break;

    case TOK_GOTO: {
      uint16_t line_number;

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (search_line_number(assign(), &line_number)) {
        basic_program_counter = line_number;
      } else {
        status = STATUS_CODE_GOTO_ERROR;
      }
      break;
    }

    case TOK_GOSUB: {
      uint16_t line_number;

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (basic_current_stack_frame < BP_BASIC_STACK_FRAMES_DEPTH) {
        basic_stack[basic_current_stack_frame] =
            basic_program_counter + len - 1;
        basic_current_stack_frame++;
        if (search_line_number(assign(), &line_number)) {
          basic_program_counter = line_number;
        } else {
          status = STATUS_CODE_GOTO_ERROR;
        }
      } else {
        status = STATUS_CODE_STACK_ERROR;
      }
      break;
    }

    case TOK_RETURN:
      printf(STAT_RETURN); //replaced bp_write_string with printf

      program_counter_updated = YES;
      if (basic_current_stack_frame) {
        basic_program_counter = basic_stack[--basic_current_stack_frame];
      } else {
        status = STATUS_CODE_RETURN_ERROR;
      }
      break;

    case TOK_REM:
      program_counter_updated = YES;
      basic_program_counter += len + 3;
      break;

    case TOK_PRINT:
      program_counter_updated = YES;
      basic_program_counter += 4;
      while ((basic_program_area[basic_program_counter] < TOK_LEN) &&
             (basic_program_area[basic_program_counter] != TOK_ELSE)) {
        if (basic_program_area[basic_program_counter] == '\"') {
          basic_program_counter++;
          while (basic_program_area[basic_program_counter] != '\"') {
            printf("%d",
              // replaced user_serial_transmit_character with printf
                basic_program_area[basic_program_counter++]);
          }
          basic_program_counter++;
        } else if (((basic_program_area[basic_program_counter] >= 'A') &&
                    (basic_program_area[basic_program_counter] <= 'Z')) ||
                   ((basic_program_area[basic_program_counter] >= TOKENS) &&
                    (basic_program_area[basic_program_counter] < TOK_LEN))) {
          temp = assign();
          // bp_write_dec_word(temp);
          printf("%d", temp);
        } else if (basic_program_area[basic_program_counter] == ';') {
          basic_program_counter++;
        }
      }
      if (basic_program_area[basic_program_counter - 1] != ';') {
        printf("\r\n");
      }
      handle_else_statement();
      break;

    case TOK_INPUT:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (basic_program_area[basic_program_counter] == '\"') {
        basic_program_counter++;
        while (basic_program_area[basic_program_counter] != '\"') {
          printf("%d",
              basic_program_area[basic_program_counter++]);
        }
        basic_program_counter++;
      }
      if (basic_program_area[basic_program_counter] == ',') {
        basic_program_counter++;
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }

      basic_variables[basic_program_area[basic_program_counter] - 'A'] =
          getnumber(0, 0, 0x7FFF, 0);
      basic_program_counter++;
      handle_else_statement();
      break;

    case TOK_FOR:
      program_counter_updated = YES;
      basic_program_counter += 4;
      if (basic_current_nested_for_index < BP_BASIC_NESTED_FOR_LOOP_COUNT) {
        basic_current_nested_for_index++;
      } else {
        status = STATUS_CODE_FOR_ERROR;
      }
      basic_nested_for_loops[basic_current_nested_for_index].var =
          (basic_program_area[basic_program_counter++] - 'A') + 1;

      if (basic_program_area[basic_program_counter] == '=') {
        basic_program_counter++;
        basic_variables
            [(basic_nested_for_loops[basic_current_nested_for_index].var) - 1] =
                assign();
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      if (basic_program_area[basic_program_counter++] == TOK_TO) {
        basic_nested_for_loops[basic_current_nested_for_index].to = assign();
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      if (basic_program_area[basic_program_counter] >= TOK_LEN) {
        basic_nested_for_loops[basic_current_nested_for_index].from =
            basic_program_counter;
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      handle_else_statement();
      break;

    case TOK_NEXT:
      program_counter_updated = YES;
      basic_program_counter += 4;

      temp = (basic_program_area[basic_program_counter++] - 'A') + 1;
      status = STATUS_CODE_NEXT_ERROR;

      for (i = 0; i <= basic_current_nested_for_index; i++) {
        if (basic_nested_for_loops[i].var == temp) {
          if (basic_variables[temp - 1] < basic_nested_for_loops[i].to) {
            basic_variables[temp - 1]++;
            basic_program_counter = basic_nested_for_loops[i].from;
          } else {
            basic_current_nested_for_index--;
          }
          status = STATUS_CODE_UNKNOWN;
        }
      }
      handle_else_statement();
      break;

    case TOK_READ:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (basic_data_read_pointer == 0) {
        i = 0;
        while ((basic_program_area[i + 3] != TOK_DATA) &&
               (basic_program_area[i] != 0x00)) {
          i += (basic_program_area[i] - TOK_LEN) + 3;
        }

        if (basic_program_area[i]) {
          basic_data_read_pointer = i + 4;
        } else {
          status = STATUS_CODE_DATA_ERROR;
        }
      }
      temp = basic_program_counter;
      basic_program_counter = basic_data_read_pointer;
      basic_variables[basic_program_area[temp] - 'A'] =
          get_number_or_variable();
      basic_data_read_pointer = basic_program_counter;
      basic_program_counter = temp + 1;

      if (basic_program_area[basic_data_read_pointer] == ',') {
        basic_data_read_pointer++;
      }
      if (basic_program_area[basic_data_read_pointer] > TOK_LEN) {
        if (basic_program_area[basic_data_read_pointer + 3] != TOK_DATA) {
          basic_data_read_pointer = 0;
        } else {
          basic_data_read_pointer += 4;
        }
      }

      handle_else_statement();
      break;

    case TOK_DATA:
      program_counter_updated = YES;
      basic_program_counter += len + 3;
      break;

    case TOK_START:
      program_counter_updated = YES;
      basic_program_counter += 4;

      // enabled_protocols[bus_pirate_configuration.bus_mode].start();
      printf("Start something\n");
      handle_else_statement();
      break;

    case TOK_STARTR:
      program_counter_updated = YES;
      basic_program_counter += 4;

      // enabled_protocols[bus_pirate_configuration.bus_mode].start_with_read();
      printf("Start something with read\n");
      handle_else_statement();
      break;

    case TOK_STOP:
      program_counter_updated = YES;
      basic_program_counter += 4;

      // enabled_protocols[bus_pirate_configuration.bus_mode].stop();
      printf("Stop something\n");
      handle_else_statement();
      break;

    case TOK_STOPR:
      program_counter_updated = YES;
      basic_program_counter += 4;

      // enabled_protocols[bus_pirate_configuration.bus_mode].stop_from_read();
      printf("Stop something from read\n");
      handle_else_statement();
      break;

    case TOK_SEND:
      program_counter_updated = YES;
      basic_program_counter += 4;
      // enabled_protocols[bus_pirate_configuration.bus_mode].send((int)assign());
      printf("Send something\n");
      handle_else_statement();
      break;

    case TOK_AUX:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Put AUX stuff here\n");

      if (assign()) {
        //bp_aux_pin_set_high();
      } else {
        //bp_aux_pin_set_low();
      }
      handle_else_statement();
      break;

    case TOK_PSU:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Put PSU stuff here\n");
      //bp_set_voltage_regulator_state(assign() ? ON : OFF);
      handle_else_statement();
      break;

    case TOK_AUXPIN:
      program_counter_updated = YES;
      basic_program_counter += 4;

      printf("Put AUXPIN stuff here\n");

      // if (assign()) {
      //   mode_configuration.alternate_aux = ON;
      // } else {
      //   mode_configuration.alternate_aux = OFF;
      // }
      handle_else_statement();
      break;

    case TOK_FREQ: {
      //int16_t frequency = assign();
      //int16_t duty_cycle = assign();
      printf("PWM frequency stuff goes here\n");

      program_counter_updated = YES;
      basic_program_counter += 4;

      // if (frequency < PWM_MINIMUM_FREQUENCY) {
      //   frequency = PWM_MINIMUM_FREQUENCY;
      // }
      // if (frequency > PWM_MAXIMUM_FREQUENCY) {
      //   frequency = PWM_MAXIMUM_FREQUENCY;
      // }

      // if (duty_cycle < PWM_MINIMUM_DUTY_CYCLE) {
      //   duty_cycle = PWM_MINIMUM_DUTY_CYCLE;
      // }
      // if (duty_cycle > PWM_MAXIMUM_DUTY_CYCLE) {
      //   duty_cycle = PWM_MAXIMUM_DUTY_CYCLE;
      // }

      // bp_update_pwm(frequency, duty_cycle);

      handle_else_statement();
      break;
    }

    case TOK_DUTY: {
      //int16_t duty_cycle = assign();

      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Duty cycle stuff goes here\n");

      // if (duty_cycle < PWM_MINIMUM_DUTY_CYCLE) {
      //   duty_cycle = PWM_MINIMUM_DUTY_CYCLE;
      // }
      // if (duty_cycle > PWM_MAXIMUM_DUTY_CYCLE) {
      //   duty_cycle = PWM_MAXIMUM_DUTY_CYCLE;
      // }

      //bp_update_duty_cycle(duty_cycle);

      handle_else_statement();
      break;
    }

    case TOK_DAT:
      program_counter_updated = 1;
      basic_program_counter += 4;
      printf("DAT stuff goes here\n");

      // if (assign()) {
      //   enabled_protocols[bus_pirate_configuration.bus_mode].data_high();
      // } else {
      //   enabled_protocols[bus_pirate_configuration.bus_mode].data_low();
      // }
      handle_else_statement();
      break;

    case TOK_CLK:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("CLK stuff goes here\n");

      // switch (assign()) {
      // case 0:
      //   enabled_protocols[bus_pirate_configuration.bus_mode].clock_low();
      //   break;
      // case 1:
      //   enabled_protocols[bus_pirate_configuration.bus_mode].clock_high();
      //   break;
      // case 2:
      //   enabled_protocols[bus_pirate_configuration.bus_mode].clock_pulse();
      //   break;
      // }
      handle_else_statement();
      break;

    case TOK_PULLUP:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Pullup stuff goes here\n");
      // bp_set_pullup_state(assign() ? ON : OFF);
      handle_else_statement();
      break;

    case TOK_DELAY:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Delay stuff goes here\n");
      //temp = assign();
      // bp_delay_ms(temp);
      handle_else_statement();
      break;

    case TOK_MACRO:
      program_counter_updated = YES;
      basic_program_counter += 4;
      printf("Macro stuff goes here\n");
      //temp = assign();
      // enabled_protocols[bus_pirate_configuration.bus_mode].run_macro(temp);
      handle_else_statement();
      break;

    case TOK_END:
      status = STATUS_CODE_FINISHED;
      break;

    default:
      status = STATUS_CODE_SYNTAX_ERROR;
      break;
    }

    if (!program_counter_updated) {
      basic_program_counter += len + 3;
    }
    program_counter_updated = NO;
  }
} // end of basic_interpreter??? 



void list(void) {
  unsigned char c;
  unsigned int lineno;

  basic_program_counter = 0;

  while (basic_program_area[basic_program_counter]) {
    c = basic_program_area[basic_program_counter];
    if (c < TOK_LET) {
      printf("%d",c);
    } else if (c > TOK_LEN) {
      printf("\r\n");
      lineno = (basic_program_area[basic_program_counter + 1] << 8) +
               basic_program_area[basic_program_counter + 2];
      basic_program_counter += 2;
      //bp_write_dec_word(lineno);
      printf("%u",lineno);
      printf(" ");
    } else {
      printf(" ");
      printf(tokens[c - TOKENS]); //replaced bp_write_string with printf
      printf(" ");
    }
    basic_program_counter++;
  }
  printf("\r\n");
  printf("%i", (basic_program_counter - 1));
  //BPMSG1050;
}

bool compare(char *pointer) {
  int oldstart = cmdstart;

  while (*pointer) {
    if (*pointer != cmdbuf[cmdstart]) {
      cmdstart = oldstart;
      return NO;
    }

    cmdstart = (cmdstart + 1) & CMDLENMSK;
    pointer++;
  }

  return YES;
}



void consumewhitechars(void) {
  while (cmdbuf[cmdstart] == 0x20) {
    cmdstart = (cmdstart + 1) & CMDLENMSK;
  }
}

void bp_basic_enter_interactive_interpreter(void) {
  int i, j, temp;
  int end, len; //, string;
  bool string_found = NO;
  unsigned char line[35];
  uint16_t pos;
  size_t index = cmdstart;

  unsigned int lineno1, lineno2;

  /* Convert all text to upper case ASCII. */

  while (index != cmdend) {
    if ((cmdbuf[index] >= 'a') && (cmdbuf[index] <= 'z')) {
      cmdbuf[index] &= 0xDF;
    }
    index = (index + 1) & CMDLENMSK;
  }

  i = 0;
  // command or a new line?
  if ((cmdbuf[cmdstart] >= '0') &&
      (cmdbuf[cmdstart] <= '9')) { // bpWline("new line");

    for (i = 0; i < 35; i++) {
      line[i] = 0;
    }

    temp = getint();
    line[1] = temp >> 8;
    line[2] = temp & 0xFF;

    if (search_line_number(temp, &pos)) {
      len = (basic_program_area[pos] - TOK_LEN) + 3;
      /* @TODO: replace this with a memmove. */
      for (i = pos; i < BP_BASIC_PROGRAM_SPACE - len; i++) {
        basic_program_area[i] = basic_program_area[i + len];
      }
      memset(&basic_program_area[i], 0x00, BP_BASIC_PROGRAM_SPACE - i);
    }

    i = 3;

    consumewhitechars();
    while (cmdstart != cmdend) {
      if (!string_found) {
        consumewhitechars();
      }

      temp = string_found ? 0 : get_token();

      if (temp) {
        line[i] = temp;
        if (temp == TOK_REM) {
          string_found = YES;
        }
      } else {
        if (cmdbuf[cmdstart] == '"') {
          string_found = !string_found;
        }
        line[i] = cmdbuf[cmdstart];
        cmdstart = (cmdstart + 1) & CMDLENMSK;
      }
      i++;
      if (i > 35) {
        //BPMSG1051;
        return;
      }
    }

    /* No need to insert an empty line. */
    if ((i == 3) || (i == 4)) {
      return;
    }

    line[0] = TOK_LEN + (i - 4);

    i = 0;
    end = 0;
    pos = 0;

    while (!end) {
      if (basic_program_area[i] > TOK_LEN) {
        len = basic_program_area[i] - TOK_LEN;
        lineno1 = (basic_program_area[i + 1] << 8) + basic_program_area[i + 2];
        lineno2 = (line[1] << 8) + line[2];
        if (lineno1 < lineno2) {
          pos = i + len + 3;
        }
        i += (len + 3);
      } else {
        end = i; // we found the end! YaY!
      }

      temp = (basic_program_area[i + 1] << 8) + basic_program_area[i + 2];
    }

    temp = (line[0] - TOK_LEN) + 3;

    /* @TODO: Replace this with memmove. */
    for (i = end; i >= pos; i--) {
      basic_program_area[i + temp] = basic_program_area[i];
    }
    memcpy(&basic_program_area[pos + i], &line[i], temp);
  } else {
    if (compare("RUN")) {
      interpreter();
      printf("\r\n");
    } else if (compare("LIST")) {
      list();
    } else if (compare("EXIT")) {
      //bus_pirate_configuration.basic = NO;
    }


  //#ifdef BP_BASIC_DEBUG_COMMAND
  else if (compare("DEBUG")) {
    for (i = 0; i < BP_BASIC_PROGRAM_SPACE; i += 16) {
      for (j = 0; j < 16; j++) {
        printf("%X", (basic_program_area[i + j]));
        printf(" ");
      }
    }
  }
  //#endif /* BP_BASIC_DEBUG_COMMAND */

    else if (compare("NEW")) {
      bp_basic_initialize();
    } else {
      //BPMSG1052;
    }
  }
}