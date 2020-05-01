#include <Arduino.h>
#include <Cmd.h>
#include <ArduinoJson.h>
#include <IRremote.h>


#define IDLE_STATE 0
#define BUTTONS_STATE 1
#define RECORD_IR_STATE 2

const String state_array[3] = {
  "IDL_STATE",
  "BUT_STATE",
  "REC_STATE"
};

// The default start state.
int current_state = IDLE_STATE;

// Location of the Arduino pins on which the buttons are connected.
const int button1 = 6;
const int button2 = 7;

// Time when button was last clicked.
unsigned long lastTimeClicked1 = 0;
unsigned long lastTimeClicked2 = 0;
// The minimum time allowed between clicks.
unsigned long clickDelay = 500;

// IR reciever pin
int RECV_PIN = 2;
// Memory and flags used for the IRremote librairy
// and storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

//Initialize record and send objects
IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results results;
bool irrecv_enabled = false;

// Declaration of the functions executed by the IRremote class.
void storeCode(decode_results *results);
void sendCode(int repeat);

// Declaration of the functions executed by the cmd class.
void ping(int arg_cnt, char **args);
void set_state(int arg_cnt, char **args);
void send_ir(int arg_cnt, char **args);
