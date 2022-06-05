#include "TOTP.h"
#define LOCK_UP 2
#define LOCK_DOWN 3
#define JUMPPER 7 // unlock without phone.

#define LD_SWITCH 4 // left door
#define RD_SWITCH 5 // right door
#define LL_SWITCH 6 // left lock
#define RL_SWITCH 8 // right lock
#define RPM_PIN 18

#define LOCK 1
#define UNLOCK 0

#define REFRESH_TIME 5

//// CODE CLEANUP !!!!!!!!!!!!!!!!

char code[7] = {0};
bool bf_relief = false;
int relief_count = 0;

/////// LOCK
class Lock{
  private:
  int up_pin;
  int down_pin;
  int ld_switch;
  int rd_switch;
  int ll_switch;
  int rl_switch;

  void disconnect(){
    digitalWrite(this->up_pin, HIGH);
    digitalWrite(this->down_pin, HIGH);
  }

  public:
  // in binary form.
  // 0: both unlock
  // 1: right lock, left unlock
  // 10: left lock, right unlock
  // 11: both lock
  int get_door_status(){
    int ld = digitalRead(ld_switch);
    int rd = digitalRead(rd_switch);

    ld = ld << 1;

    return ld | rd;
  }

  int get_lock_status(){
    int ll = digitalRead(ll_switch);
    int rl = digitalRead(rl_switch);

    ll = ll << 1;

    return ll | rl;
  }

  Lock(int up_pin, int down_pin, int ld_switch, int rd_switch, int ll_switch, int rl_switch){
    this->up_pin = up_pin;
    this->down_pin = down_pin;
    this->ld_switch = ld_switch;
    this->rd_switch = rd_switch;
    this->ll_switch = ll_switch;
    this->rl_switch = rl_switch;

    pinMode(up_pin, OUTPUT);
    pinMode(down_pin, OUTPUT);
    pinMode(ld_switch, INPUT);
    pinMode(rd_switch, INPUT);
    pinMode(ll_switch, INPUT);
    pinMode(rl_switch, INPUT);
    pinMode(JUMPPER, INPUT);
    this->disconnect();

  }

  int get_jumpper_status(){
    return digitalRead(JUMPPER);
  }
  void unlock(){
    digitalWrite(this->up_pin, LOW);
    digitalWrite(this->down_pin, HIGH);
    delay(100);
    this->disconnect();
  }

  void lock(){
    digitalWrite(this->up_pin, HIGH);
    digitalWrite(this->down_pin, LOW);
    delay(100);
    this->disconnect();
  }
};
Lock* obj_lock = NULL;

/////// BlueTooth - BLE
class BlueTooth{
  private:
  class Command{
    public:
    uint8_t version;
    uint8_t command; // 0 = unlock, 1 = lock
    uint32_t totp;
  };

  class Package{
    public:
    const uint8_t* data;
    size_t len;

    private:
    bool is_valid(const uint8_t* data, size_t len){
      //////// Auth algo
      // Check length
      if(len == 5){
        // Decrypt
        Command* cmd = this->decrypt(data, len);
        if(cmd != NULL){
          if(cmd->totp == strtoul(code, NULL, 0)){
            free(cmd);
            return true;
          }
          free(cmd);
        }
        return false;
      }

      // Simulate after decryption
      return false;
    }

    Command* unpack(const uint8_t* data){
      Command* cmd = new Command();
      cmd->version = data[0]; // not use for now
      cmd->command = data[1];

      int t = 0;
      // get 24 bits
      for(int i = 2; i < 5; i++){
        t = t << 8;
        t += data[i];
      }
      cmd->totp = (uint32_t)t;

      return cmd;
    }

    // 0x0100123456FFFF len = 5
    // 0x01(version)0x00(command)0x123456(tbotp)
    Command* decrypt(const uint8_t* data, size_t len){
      // Not Yet have time to implement the Encrypt and Decrypt function.

      if (true)
        return this->unpack(data);
      
      // Fail
      return NULL;
    }

    public:
    Package(const uint8_t* data, size_t len){
      if(this->is_valid(data, len)){ // compare the OTP
        this->data = data;
        this->len = len;
      }else{
        this->data = NULL;
        this->len = 0;
      }
    }
  };

  private:
  // Advertising data
  BleAdvertisingData advData;

  public:
  const char* service_uuid;
  const char* locking_command;

  BlueTooth(const char* service_uuid, const char* reciver){
    this->service_uuid = service_uuid;
    this->locking_command = reciver;

    BleUuid lockingService(service_uuid);
    BleCharacteristic commandCharacteristic("command", BleCharacteristicProperty::WRITE_WO_RSP, reciver, service_uuid, onDataReceived, (void*)reciver);

    // Add the COMMAND characteristics
    BLE.addCharacteristic(commandCharacteristic);

    // Add the locking service
    advData.appendServiceUUID(lockingService);

    // Start advertising!
    BLE.advertise(&advData);
  }

  int unpack(const uint8_t* data, size_t len){
    Package package = Package(data, len);
    if(package.len == 0){
      return -1; // invalid package.
    }
    else{
      // execute package content.
      switch(package.data[1]){
        case 0: // UNLOCK
          if(obj_lock->get_lock_status() != 0)
            obj_lock->unlock();
          break;
        case 1: // LOCK

          if(obj_lock->get_lock_status() != 3)
            obj_lock->lock();
          break;
      }
    }
    return 0;
  }
};
BlueTooth* BT = NULL;

// handling Bluetooth Low Energy callbacks
static void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  if(bf_relief == true || len == 0)
    return;

  relief_count += 1;

  // 2 commands per 1 sec
  if(relief_count > REFRESH_TIME * 2)
    bf_relief = true;
  

  BT->unpack(data, len);
}

// Variables for keeping state
typedef struct {
  uint8_t version;
  String pay_load; // use base64
} bt_data;

// Static level tracking
static bt_data m_bt_data;

void wifi_off(){
  // For saving power, since most of the power draw of the device is the Wi-Fi module.

  // call Particle.disconnect() before turning off the Wi-Fi manually, otherwise the cloud connection may turn it back on again.
  Particle.disconnect();
  WiFi.off();
}

void led_off(){
  RGB.control(true);
  RGB.color(0, 0, 0);
}

float get_rpm(){
  int val = analogRead(RPM_PIN);
  return val;
}

// TOTP* timeotp = NULL;
void setup() {
  wifi_off();
  led_off();
  obj_lock = new Lock(LOCK_UP, LOCK_DOWN, LD_SWITCH, RD_SWITCH, LL_SWITCH, RL_SWITCH);

  // uint8_t hmac_key[] = {0x68, 0x6e, 0x74, 0x36, 0x72, 0x6a, 0x6c, 0x73, 0x6b, 0x64, 0x6e, 0x67, 0x6c, 0x79, 0x7a, 0x63};
  // timeotp = new TOTP(hmac_key, 16);

  const char* service_uuid = "ad712cf0-c718-4c19-bef9-f1710c7ced9d"; //service
  const char* reciver = "ad712cf1-c718-4c19-bef9-f1710c7ced9d"; // command

  BlueTooth* BT = new BlueTooth(service_uuid, reciver);
}

void loop() {
  uint8_t key[] = {0x48, 0x61, 0x4e, 0x73, 0x4f, 0x6d, 0x50, 0x72, 0x4f, 0x4d, 0x65, 0x7a, 0x64, 0x61, 0x31, 0x32, 0x31, 0x4c, 0x53, 0x54, 0x39, 0x37};
  TOTP timeotp = TOTP(key, 22);
  char* new_code = timeotp.getCode(Time.now());
  if(strcmp(code, new_code) != 0){
    strcpy(code, new_code);
  }

  // unlock the car when the jumpper is connected.
  if(obj_lock->get_jumpper_status() == 1){
    obj_lock->unlock();
  }
  // lock the door when driving only if all door are closed.
  if(get_rpm() > 2000 && obj_lock->get_door_status() == 3 && obj_lock->get_lock_status() != 3)
    // ADD lock sensor so that it will not keep turning the motor.
    obj_lock->lock();

  bf_relief = false;
  relief_count = 0;
  delay(REFRESH_TIME * 1000);
}