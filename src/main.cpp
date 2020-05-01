#include <main.h>

void setup() {

  // Set button pins in read mode.
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  // Initialize serial communication.
  Serial.begin(115200);
  // Wait for serial port to connect. Needed for native USB port only.
  while (!Serial) {}

  // Initialize Command manager and supported commands
  cmdInit(&Serial);
  cmdAdd("SET_STATE", set_state);
  cmdAdd("SEND_IR", send_ir);
  cmdAdd("PING", ping);
}

// Main loop
void loop() {
  // Check if there is any available input command
  cmdPoll();

  // Main state machine
  switch (current_state) {
    //////////////////IDLE STATE//////////////////
    case IDLE_STATE:
    // Do nothing
    break;

    //////////////////BUTTON STATE//////////////////
    case BUTTONS_STATE:
      {
        int reading1 = digitalRead(button1);
        int reading2 = digitalRead(button2);

          if (reading1 == HIGH && ((millis() - lastTimeClicked1) > clickDelay)) {
            lastTimeClicked1 = millis();
            Serial.print("BTN_PRESSED {1}");
          }

          if (reading2 == HIGH && ((millis() - lastTimeClicked2) > clickDelay)) {
            lastTimeClicked2 = millis();
            Serial.print("BTN_PRESSED {2}");
          }
      }
    break;

    //////////////////RECORD IR STATE//////////////////
    case RECORD_IR_STATE:
    // Start the receiver
    if(!irrecv_enabled){
      irrecv.enableIRIn();
      irrecv_enabled = true;
    }
    if (irrecv.decode(&results)) {
      storeCode(&results);
      //Serial.println("RECORD {SUCC:RECORD}");
      // Go back to idle
      current_state = IDLE_STATE;
      irrecv_enabled = false;
    }
    break;

    default:
      current_state = IDLE_STATE;
  }
}

///////////////////////////////////////////////////////////////////////////
////////////////////////IR send/decode functions///////////////////////////
///////////////////////////////////////////////////////////////////////////
// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  int count = results->rawlen;
  if (codeType == UNKNOWN) {
    //Serial.println("unknown, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
    //    Serial.print(" m");
      }
      else {
        // Space
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
    //    Serial.print(" s");
      }
    //  Serial.print(rawCodes[i - 1], DEC);
    }
    //Serial.println("");
    Serial.print("RECORDED {codeLen:");
    Serial.print(codeLen);
    Serial.print(",rawbuf: ");

    for (int i = 0; i <= RAWBUF - 1; i++){
      int n = rawCodes[i]/50;
      if (n != 0){
        Serial.print(rawCodes[i]/50);
        if (rawCodes[i+1] != 0){
          Serial.print(" ");
        }
      }
    }
    Serial.print("}");
  }
  else {
    codeValue = results->value;
    codeLen = results->bits;
    Serial.print("RECORDED {codeLen:") ;
    Serial.print(codeLen);
    Serial.print(",codeType:");
    Serial.print(codeType);
    Serial.print(",codeValue:");
    Serial.print(codeValue);
    Serial.print("}");
  }
}

void sendCode(int repeat) {
  if (codeType == NEC) {
    if (repeat) {
    //  irsend.sendNEC(REPEAT, codeLen);
      Serial.print("Sent NEC repeat");
    }
    else {
    //  irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
    }
  }
  else if (codeType == SONY) {
  //  irsend.sendSony(codeValue, codeLen);
    Serial.print("Sent Sony ");
  }
  else if (codeType == PANASONIC) {
  //  irsend.sendPanasonic(codeValue, codeLen);
    Serial.print("Sent Panasonic ");
  }
  else if (codeType == SAMSUNG) {
  //  irsend.sendSAMSUNG(codeValue, codeLen);
    Serial.print("Sent Samsung ");
  }
  else if (codeType == JVC) {
  //  irsend.sendJVC(codeValue, codeLen, false);
    Serial.print("Sent JVC ");
  }
  else if (codeType == RC5 || codeType == RC6) {
    if (!repeat) {
      // Flip the toggle bit for a new button press
      toggle = 1 - toggle;
    }
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.print(codeValue, HEX);
    //  irsend.sendRC5(codeValue, codeLen);
    }
    else {
      //irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.print(codeValue, HEX);
    }
  }
  else if (codeType == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    Serial.print("Sent raw");
  }
}
///////////////////////////////////////////////////////////////////////////
///////////////////////////Command functions///////////////////////////////
///////////////////////////////////////////////////////////////////////////

// Send command
void send_ir(int arg_cnt, char **args){
  // SEND_IR {codeLen:32,codeType:3,codeValue:16689239}
  // If no data is given send last loaded code.
  if (arg_cnt > 0){
    //Serial.println(args[1]);
    String s = args[1];
    // If code is decoded.
    if (!(s.indexOf("rawbuf:") > 0)){

      int index_codeLen = s.indexOf("codeLen");
      int index_codeType = s.indexOf("codeType");
      int index_codeValue = s.indexOf("codeValue");

       // Serial.println(s);
       // Serial.println(s.substring(9,index_codeType - 1).toInt());
       // Serial.println(s.substring(index_codeType + 9 ,index_codeValue - 1).toInt());
       // Serial.println(s.substring(index_codeValue + 10 ,s.length()).toInt());

      // Load the received code in the IRremote.
      codeLen = s.substring(9,index_codeType - 1).toInt();
      codeType = s.substring(index_codeType + 9 ,index_codeValue - 1).toInt();
      codeValue = s.substring(index_codeValue + 10 ,s.length()).toInt();

       // Serial.println(codeLen);
       // Serial.println(codeType);
       // Serial.println(codeValue);

    }else{
      int index_codeLen = s.indexOf("codeLen");
      int index_rawbuf = s.indexOf("rawbuf");

      codeLen = s.substring(9,index_rawbuf - 1).toInt();
      codeType = UNKNOWN;

      for (int p = 2; p < codeLen + 2; p++){
        // Serial.print(p);
        // Serial.print(" : ");
        // Serial.print(args[p]);
        // Serial.print(" -> ");

        int stringToInt = atoi(args[p])*50;
        // Serial.println(stringToInt);
        rawCodes[p-2] = stringToInt;
      }
      // for(int i = 0; i < codeLen; i++){
      //   Serial.println(rawCodes[i]);
      // }
    }
  }
  // Make sure a code is loaded
  if(codeType != 0){
    // For now the repeat is forced to false. This means every time a
    // command is sent it is has the user released the button.
    sendCode(false);
    return;
  }
  Serial.print("ERROR {TYPE:no code}");
}

// Change the state of the main state machine.
void set_state(int arg_cnt, char **args){
  String s = args[1];

  // Check if recieved state is valid. If so update state.
  if (s.indexOf("STATE:") > 0){
    for (int i;i < sizeof(state_array);i++){
      if(s.indexOf(state_array[i]) > 0){
        current_state = i;
        // WTF cant print all caracters ... there seams to be a limit in
        // the length of the string ... dont get it. Will keep strings to
        // a minimum. Was probably because of the '"'
        String temp = "SET_STATE {SUCC:";
        temp += state_array[i];
        temp += "}";
        Serial.print(temp);
        return;
      }
    }
    Serial.print("ERROR {TYPE:unknown state}");
    return;
  }
  Serial.print("ERROR {TYPE:parsing}");
}

// Send the current state of the main state machine.
void ping(int arg_cnt, char **args){
  Serial.print("PING {current_state:" + state_array[current_state] + "}");
}
