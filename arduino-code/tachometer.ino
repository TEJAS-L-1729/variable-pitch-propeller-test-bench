const int receiverPin = 2;

// Define the exact number of blades on your propeller.
// This value is CRITICAL for accurate RPM calculation.
const int numberOfBlades = 2; // Example: Set this to the actual number of blades (e.g., 2, 3, 4)

// Variables for RPM calculation (declared as volatile for safe access from ISR)
volatile unsigned long lastInterruptionTime_micros = 0; // Stores the timestamp of the last blade detection
volatile unsigned long rotationPeriod_micros_sum = 0;   // Accumulator for multiple interruption intervals
volatile unsigned int interruptions_count = 0;          // Counts interruptions within a measurement cycle

// Define a smoothing factor for RPM calculation.
// This determines how many interruption intervals are averaged to calculate RPM.
// A higher value provides more stable readings but responds slower to RPM changes.
// A value equal to numberOfBlades means we average over one full rotation.
const int smoothingFactor = numberOfBlades * 1; // Average over two full rotations for stability


long rpm_sum = 0;
long rpm_reading[100];
long rpm_average = 0;
byte n_max = 0;
byte n = 0;


// Flag to control when RPM is updated (prevents frequent updates for stability)
unsigned long lastRPMUpdateTime_millis = 0;
const unsigned long rpmUpdateInterval_millis = 200; // Update RPM every 200 milliseconds

// Function called by the interrupt service routine (ISR)
// This function must be short and fast, avoiding delay(), Serial.print(), etc.
void detectBlade() { // IRAM_ATTR might be useful for some boards, not strictly necessary for Uno
  unsigned long currentInterruptionTime_micros = micros();

  // Only proceed if this is not the very first interruption or if time has passed
  if (lastInterruptionTime_micros != 0) {
    unsigned long interval = currentInterruptionTime_micros - lastInterruptionTime_micros;

    // Filter out very short or very long intervals (noise or extreme speed changes)
    // Adjust these thresholds based on expected RPM range.
    // Assuming min RPM ~60 (1s/rev) and max RPM ~12000 (0.005s/rev).
    // Min interval for 12000 RPM (3 blades) = (60,000,000us / 12000rpm) / 3 blades = 1666 us
    // Max interval for 60 RPM (3 blades) = (60,000,000us / 60rpm) / 3 blades = 333333 us
    if (interval > 500 && interval < 500000) { // Example thresholds: >0.5ms and <0.5s
      rotationPeriod_micros_sum += interval;
      interruptions_count++;
    }
  }
  lastInterruptionTime_micros = currentInterruptionTime_micros;
}

void setup() {
  // Initialize Serial communication for debugging and displaying output
  Serial.begin(2000000);
  Serial.println("-------------------------------------");
  Serial.println("Arduino Propeller RPM Detector");
  Serial.print("Number of Blades: ");
  Serial.println(numberOfBlades);
  Serial.println("Waiting for propeller rotations...");
  Serial.println("-------------------------------------");

  // Configure the receiver pin as an input.
  // INPUT_PULLUP can be used if your sensor module needs an internal pull-up resistor.
  pinMode(receiverPin, INPUT);

  // Attach an interrupt to the receiver pin.
  // digitalPinToInterrupt() maps the digital pin number to the interrupt number.
  // FALLING: The interrupt will trigger when the signal on receiverPin goes from HIGH to LOW.
  //          Choose FALLING, RISING, or CHANGE based on your sensor's behavior.
  attachInterrupt(digitalPinToInterrupt(receiverPin), detectBlade, FALLING);
}

void loop() {
  // Only calculate and print RPM at regular intervals for stable output
  unsigned long currentMillis = millis();
  if (currentMillis - lastRPMUpdateTime_millis >= rpmUpdateInterval_millis) {
    lastRPMUpdateTime_millis = currentMillis;

    // Check if we have collected enough interruption data for a reliable calculation
    if (interruptions_count >= smoothingFactor && rotationPeriod_micros_sum > 0) {
      // Disable interrupts temporarily while accessing volatile variables
      noInterrupts();

      // Calculate the average time between consecutive blade interruptions
      float averageInterruptionInterval_micros = (float)rotationPeriod_micros_sum / interruptions_count;

      // Calculate the time for one full rotation in microseconds
      // Time_per_rotation = Average_Interruption_Interval * Number_of_Blades
      unsigned long timePerRotation_micros = (unsigned long)(averageInterruptionInterval_micros * smoothingFactor);

      // Reset the accumulated sum and count for the next measurement cycle
      rotationPeriod_micros_sum = 0;
      interruptions_count = 0;

      // Re-enable interrupts
      interrupts();

      // Avoid division by zero
      if (timePerRotation_micros > 0) {
       rpm_reading[n] = (60000000 / timePerRotation_micros);
       n_max = constrain(map(rpm_reading[n], 60, 100000, 0, 100), 0, 100);
       n++;
       
       if(n > n_max){
       
         for (byte i = 0; i <= n_max; i++) {
           
           rpm_sum = rpm_sum + rpm_reading[i];
     
         };
         
         rpm_average = rpm_sum / (n_max + 1);     
         rpm_sum = 0;
         n = 0;
        Serial.print("Estimated RPM: ");
        Serial.println(rpm_average);
        delay(1500);
       }
      
    } else {
      // If not enough data collected yet, prompt the user
      // Only print this if we haven't received enough interruptions for calculation
      if (interruptions_count == 0) {
         Serial.println("No propeller detected or insufficient interruptions. Spinning?");
      } else {
         Serial.print("Collecting data... ");
         Serial.print(interruptions_count);
         Serial.print("/");
         Serial.print(smoothingFactor);
         Serial.println(" interruptions received.");
      }
    }
  }
}
}